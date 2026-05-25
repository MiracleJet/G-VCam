#ifndef GVCAM_COLOR_CONVERT_H
#define GVCAM_COLOR_CONVERT_H

#include <stddef.h>
#include <stdint.h>
#include "gvcam/frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Convert BGR (OpenCV / turbojpeg output) to the target pixel format.
   src_w/src_h — BGR image dimensions
   dst_buf     — pre-allocated (use gvcam_frame_buffer_size())
   Returns 0 on success. */
int gvcam_bgr_to_nv12(const uint8_t *bgr, int w, int h, uint8_t *dst);
int gvcam_bgr_to_yuy2(const uint8_t *bgr, int w, int h, uint8_t *dst);
int gvcam_bgr_to_i420(const uint8_t *bgr, int w, int h, uint8_t *dst);

/* Generic: convert BGR to whatever fmt specifies. */
int gvcam_bgr_convert(const uint8_t *bgr, int w, int h,
                      gvcam_pixel_format_t fmt, uint8_t *dst);

#ifdef __cplusplus
}
#endif

#endif /* GVCAM_COLOR_CONVERT_H */
