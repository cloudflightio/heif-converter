// Import Node.js Dependencies
import { Readable } from "stream";

interface PngOptions {
  compression?: number;
}

declare const types: {
  toPng(input: Buffer | Readable, options?: PngOptions): Promise<Buffer>;
  version(): string;
};

export default types;
