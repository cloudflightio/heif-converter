{
  "targets": [
    {
      "target_name": "converter",
      "sources": [
        "lib.cc"
      ],
      "include_dirs": [
        "<!(node -p \"require('node-addon-api').include_dir\")",
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_CPP_EXCEPTIONS" ],
      "conditions": [
        [
          'OS=="linux"', 
          {
            "variables": {
              "VCPKG_LIB_ROOT%": "<!(echo $VCPKG_LIB_ROOT)",
              "conditions": [
                [
                  '"<!(echo $VCPKG_LIB_ROOT)" == ""', 
                  {
                    "VCPKG_LIB_ROOT%": "<(module_root_dir)/../platform/linux<!(node ../libc.js)-x64"
                  }
                ]
              ]
            },
            "libraries": [
              "<(VCPKG_LIB_ROOT)/lib/libheif.a",
              "<(VCPKG_LIB_ROOT)/lib/libturbojpeg.a",
              "<(VCPKG_LIB_ROOT)/lib/libpng16.a",
              "<(VCPKG_LIB_ROOT)/lib/libde265.a",
              "<(VCPKG_LIB_ROOT)/lib/libx265.a"
            ],
            "include_dirs": [
              "<(VCPKG_LIB_ROOT)/include",
              "<(VCPKG_LIB_ROOT)/include/libheif",
              "<(VCPKG_LIB_ROOT)/include/libpng16"
            ]
          }
        ],
        [
          'OS=="mac"', 
          {
            "variables": {
              "VCPKG_LIB_ROOT%": "<!(echo $VCPKG_LIB_ROOT)",
              "conditions": [
                [
                  '"<!(echo $VCPKG_LIB_ROOT)" == ""', 
                  {
                    "VCPKG_LIB_ROOT%": "<(module_root_dir)/../platform/darwin-x64"
                  }
                ]
              ]
            },
            "libraries": [
              "<(VCPKG_LIB_ROOT)/lib/libheif.a",
              "<(VCPKG_LIB_ROOT)/lib/libturbojpeg.a",
              "<(VCPKG_LIB_ROOT)/lib/libpng16.a",
              "<(VCPKG_LIB_ROOT)/lib/libde265.a",
              "<(VCPKG_LIB_ROOT)/lib/libx265.a"
            ],
            "include_dirs": [
              "<(VCPKG_LIB_ROOT)/include",
              "<(VCPKG_LIB_ROOT)/include/libheif"
            ],
            'xcode_settings': {
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
              'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES'
            },
          }
        ],
        [
          'OS=="win"',
          {
            "conditions": [
              [
                'target_arch=="x64"',
                {
                  "variables": {
                    "VCPKG_LIB_ROOT%": '<!(node -e "console.log(process.env.VCPKG_LIB_ROOT)")',
                    "conditions": [
                      [
                        '"<!(node -e "console.log(process.env.VCPKG_LIB_ROOT)")" == "undefined"',
                        {
                          "VCPKG_LIB_ROOT%": "<(module_root_dir)/../platform/win32-x64"
                        }
                      ]
                    ]
                  }
                }
              ],
              [
                'target_arch=="ia32"',
                {
                  "variables": {
                    "VCPKG_LIB_ROOT%": '<!(node -e "console.log(process.env.VCPKG_LIB_ROOT)")',
                    "conditions": [
                      [
                        '"<!(node -e "console.log(process.env.VCPKG_LIB_ROOT)")" == "undefined"',
                        {
                          "VCPKG_LIB_ROOT%": "<(module_root_dir)/../platform/win32-ia32"
                        }
                      ]
                    ]
                  }
                }
              ]
            ],
            "msvs_settings": {
              "VCCLCompilerTool": {
                "ExceptionHandling": 1
              }
            },
            "libraries": [
              "<(VCPKG_LIB_ROOT)/lib/heif.lib",
              "<(VCPKG_LIB_ROOT)/lib/turbojpeg.lib",
              "<(VCPKG_LIB_ROOT)/lib/libpng16.lib"
            ],
            "include_dirs": [
              "<(VCPKG_LIB_ROOT)/include",
              "<(VCPKG_LIB_ROOT)/include/libheif"
            ],
            "copies": [
              {
                "destination": "<(module_root_dir)/build/Release",
                "files": [
                  "<(VCPKG_LIB_ROOT)/bin/heif.dll",
                  "<(VCPKG_LIB_ROOT)/bin/turbojpeg.dll",
                  "<(VCPKG_LIB_ROOT)/bin/libde265.dll",
                  "<(VCPKG_LIB_ROOT)/bin/libx265.dll",
                  "<(VCPKG_LIB_ROOT)/bin/libpng16.dll",
                  "<(VCPKG_LIB_ROOT)/bin/zlib1.dll"
                ]
              }
            ]
          }
        ]
      ]
    }
  ]
}
