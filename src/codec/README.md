# src/codec — MJPEG/JPEG Decode

Optional. Requires libjpeg-turbo.

| File | Purpose |
|---|---|
| `jpeg_decoder.h` / `jpeg_decoder.c` | Thin wrapper around turbojpeg: JPEG → BGR |
| `mjpeg_parser.h` / `mjpeg_parser.c` | Streaming MJPEG parser (multipart/x-mixed-replace) |
