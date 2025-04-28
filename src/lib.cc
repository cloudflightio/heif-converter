#include <napi.h>
#include <heif.h>
#include <turbojpeg.h>
#include <png.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>

class ToPngWorker : public Napi::AsyncWorker {
public:
    ToPngWorker(const Napi::Buffer<uint8_t> buffer, heif_item_id image_id, const Napi::Object& options, Napi::Function& callback)
        : Napi::AsyncWorker(callback), imageId(image_id) {
        
        data = buffer.Data();
        size = buffer.Length();
        bufferRef = Napi::Persistent(buffer);

        compression = options.Get("compression").As<Napi::Number>().Int32Value();
    }

    ~ToPngWorker() {}

    void Execute() override {
        struct heif_context* ctx = heif_context_alloc();
        struct heif_error err;

        err = heif_context_read_from_memory(ctx, data, size, nullptr);

        if (err.code != heif_error_Ok) {
            heif_context_free(ctx);
            SetError("Failed to read HEIF data: " + std::string(err.message));
            return;
        }

        struct heif_image_handle* handle;
        if (imageId == -1) {
            err = heif_context_get_primary_image_handle(ctx, &handle);
        } else {
            err = heif_context_get_image_handle(ctx, imageId, &handle);
        }

        if (err.code != heif_error_Ok) {
            heif_context_free(ctx);
            SetError("Failed to get image handle: " + std::string(err.message));
            return;
        }

        struct heif_image* img;
        err = heif_decode_image(handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGB, nullptr);
        if (err.code != heif_error_Ok) {
            heif_image_handle_release(handle);
            heif_context_free(ctx);
            SetError("Failed to decode HEIF image: " + std::string(err.message));
            return;
        }

        int width = heif_image_get_width(img, heif_channel_interleaved);
        int height = heif_image_get_height(img, heif_channel_interleaved);
        int stride;
        const uint8_t* data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);

        std::vector<uint8_t> row(width * 3);

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png_ptr) {
            heif_image_release(img);
            heif_image_handle_release(handle);
            heif_context_free(ctx);
            SetError("Failed to create PNG write struct");
            return;
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_write_struct(&png_ptr, nullptr);
            heif_image_release(img);
            heif_image_handle_release(handle);
            heif_context_free(ctx);
            SetError("Failed to create PNG info struct");
            return;
        }

        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            heif_image_release(img);
            heif_image_handle_release(handle);
            heif_context_free(ctx);
            SetError("Error during PNG creation");
            return;
        }

        enum heif_color_profile_type cp_type =
        heif_image_handle_get_color_profile_type(handle);
        void* icc_buf = NULL;
        size_t icc_len = 0;
        if (cp_type == heif_color_profile_type_prof || cp_type == heif_color_profile_type_rICC) {
            icc_len = heif_image_handle_get_raw_color_profile_size(handle);
            if (icc_len) {
                icc_buf = malloc(icc_len);
                err = heif_image_handle_get_raw_color_profile(handle, icc_buf);
                if (err.code != heif_error_Ok) {
                    free(icc_buf);
                    icc_buf = NULL;
                    icc_len = 0;
                }
            }
        }

        png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        png_set_compression_level(png_ptr, compression);
        png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
        if (icc_buf && icc_len) {
            png_set_iCCP(png_ptr, info_ptr, "ICC Profile", PNG_COMPRESSION_TYPE_BASE, static_cast<png_const_bytep>(icc_buf), icc_len);
        }

        std::vector<uint8_t> pngData;
        png_set_write_fn(png_ptr, &pngData, [](png_structp png_ptr, png_bytep data, png_size_t length) {
            std::vector<uint8_t>* p = static_cast<std::vector<uint8_t>*>(png_get_io_ptr(png_ptr));
            p->insert(p->end(), data, data + length);
        }, nullptr);

        png_write_info(png_ptr, info_ptr);

        for (int y = 0; y < height; ++y) {
            memcpy(row.data(), data + y * stride, width * 3);
            png_write_row(png_ptr, row.data());
        }

        png_write_end(png_ptr, nullptr);
        png_destroy_write_struct(&png_ptr, &info_ptr);

        pngBuffer.assign(pngData.begin(), pngData.end());

        heif_image_release(img);
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        free(icc_buf);
    }

    void OnOK() override {
        Napi::HandleScope scope(Env());
        Napi::Buffer<uint8_t> result = Napi::Buffer<uint8_t>::Copy(Env(), pngBuffer.data(), pngBuffer.size());
        Callback().Call({Env().Null(), result});
    }

private:
    Napi::Reference<Napi::Buffer<uint8_t>> bufferRef;
    const uint8_t* data = nullptr;
    size_t size = 0;
    heif_item_id imageId;
    std::vector<uint8_t> pngBuffer;
    int compression;
};

Napi::Value ToPng(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::Buffer<uint8_t> input = info[0].As<Napi::Buffer<uint8_t>>();
    
    heif_item_id imageId = -1;
    if (info[1].IsNumber()) {
        imageId = info[1].As<Napi::Number>().Uint32Value();
    }

    Napi::Object options = info[2].As<Napi::Object>();
    Napi::Function callback = info[3].As<Napi::Function>();

    ToPngWorker* worker = new ToPngWorker(input, imageId, options, callback);
    worker->Queue();
    return env.Undefined();
}

Napi::String GetVersion(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), heif_get_version());
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "version"), Napi::Function::New(env, GetVersion));
    exports.Set(Napi::String::New(env, "toPng"), Napi::Function::New(env, ToPng));

    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAll)