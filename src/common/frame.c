#include "frame.h"

int gvcam_pixel_format_bpp(gvcam_pixel_format_t fmt)
{
    switch (fmt) {
    case GVCAM_FMT_NV12:  case GVCAM_FMT_I420:  return 0; /* planar */
    case GVCAM_FMT_YUY2:                         return 2; /* packed 4:2:2 */
    case GVCAM_FMT_BGR:                          return 3; /* packed 24-bit */
    default:                                     return 0;
    }
}

int gvcam_frame_buffer_size(int width, int height, gvcam_pixel_format_t fmt)
{
    switch (fmt) {
    case GVCAM_FMT_NV12:
    case GVCAM_FMT_I420:
        return width * height * 3 / 2;
    case GVCAM_FMT_YUY2:
        return width * height * 2;
    case GVCAM_FMT_BGR:
        return width * height * 3;
    default:
        return 0;
    }
}
