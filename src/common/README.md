# src/common — Platform-Independent Libraries

C11, no external dependencies.

| Library | Sources | Purpose |
|---|---|---|
| `gvcam_core` | `frame.c`, `color_convert.c` | Frame buffer sizing, BGR→NV12/YUY2/I420 conversion |
| `gvcam_codec` | `mjpeg_parser.c`, `jpeg_decoder.c` | MJPEG/JPEG decoding (requires libjpeg-turbo, optional) |
