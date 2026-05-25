#include "color_convert.h"
#include <string.h>

/*
 * BGR → NV12
 * NV12 layout: Y plane (full res) + interleaved UV (quarter res, U first).
 *
 * Using standard BT.601 coefficients:
 *   Y  =  0.299 R + 0.587 G + 0.114 B
 *   U  = -0.169 R - 0.331 G + 0.500 B + 128
 *   V  =  0.500 R - 0.419 G - 0.081 B + 128
 */
int gvcam_bgr_to_nv12(const uint8_t *bgr, int w, int h, uint8_t *dst)
{
    const int y_size  = w * h;
    const int uv_size = w * h / 4;
    uint8_t *y_ptr  = dst;
    uint8_t *uv_ptr = dst + y_size;

    for (int row = 0; row < h; row++) {
        const uint8_t *src = bgr + (size_t)row * w * 3;
        uint8_t *y_row = y_ptr + (size_t)row * w;

        for (int col = 0; col < w; col++) {
            int b = src[col * 3];
            int g = src[col * 3 + 1];
            int r = src[col * 3 + 2];

            /* BT.601 full swing */
            y_row[col] = (uint8_t)(( 66 * r + 129 * g +  25 * b + 128) >> 8);

            if ((row & 1) == 0 && (col & 1) == 0) {
                size_t uv_idx = ((size_t)(row / 2) * (w / 2) + (col / 2)) * 2;
                uv_ptr[uv_idx]     = (uint8_t)((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
                uv_ptr[uv_idx + 1] = (uint8_t)((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
            }
        }
    }

    return 0;
}

int gvcam_bgr_to_yuy2(const uint8_t *bgr, int w, int h, uint8_t *dst)
{
    /* YUY2 packs 2 pixels into 4 bytes: Y0 U Y1 V */
    for (int row = 0; row < h; row++) {
        const uint8_t *src = bgr + (size_t)row * w * 3;
        uint8_t *d = dst + (size_t)row * w * 2;

        for (int col = 0; col < w; col += 2) {
            int b0 = src[col * 3];
            int g0 = src[col * 3 + 1];
            int r0 = src[col * 3 + 2];
            int b1 = src[(col + 1) * 3];
            int g1 = src[(col + 1) * 3 + 1];
            int r1 = src[(col + 1) * 3 + 2];

            int y0 = ( 66 * r0 + 129 * g0 +  25 * b0 + 128) >> 8;
            int y1 = ( 66 * r1 + 129 * g1 +  25 * b1 + 128) >> 8;

            /* Average chroma from both pixels */
            int ra = (r0 + r1) / 2, ga = (g0 + g1) / 2, ba = (b0 + b1) / 2;
            int u  = ((-38 * ra -  74 * ga + 112 * ba + 128) >> 8) + 128;
            int v  = ((112 * ra -  94 * ga -  18 * ba + 128) >> 8) + 128;

            d[col * 2]     = (uint8_t)y0;
            d[col * 2 + 1] = (uint8_t)u;
            d[col * 2 + 2] = (uint8_t)y1;
            d[col * 2 + 3] = (uint8_t)v;
        }
    }
    return 0;
}

int gvcam_bgr_to_i420(const uint8_t *bgr, int w, int h, uint8_t *dst)
{
    const int y_size  = w * h;
    const int u_size  = w * h / 4;
    uint8_t *y_ptr = dst;
    uint8_t *u_ptr = dst + y_size;
    uint8_t *v_ptr = dst + y_size + u_size;

    for (int row = 0; row < h; row++) {
        const uint8_t *src = bgr + (size_t)row * w * 3;
        uint8_t *y_row = y_ptr + (size_t)row * w;

        for (int col = 0; col < w; col++) {
            int b = src[col * 3];
            int g = src[col * 3 + 1];
            int r = src[col * 3 + 2];
            y_row[col] = (uint8_t)(( 66 * r + 129 * g +  25 * b + 128) >> 8);

            if ((row & 1) == 0 && (col & 1) == 0) {
                size_t uv_idx = (size_t)(row / 2) * (w / 2) + (col / 2);
                u_ptr[uv_idx] = (uint8_t)((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
                v_ptr[uv_idx] = (uint8_t)((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
            }
        }
    }
    return 0;
}

int gvcam_bgr_convert(const uint8_t *bgr, int w, int h,
                      gvcam_pixel_format_t fmt, uint8_t *dst)
{
    switch (fmt) {
    case GVCAM_FMT_NV12: return gvcam_bgr_to_nv12(bgr, w, h, dst);
    case GVCAM_FMT_YUY2: return gvcam_bgr_to_yuy2(bgr, w, h, dst);
    case GVCAM_FMT_I420: return gvcam_bgr_to_i420(bgr, w, h, dst);
    case GVCAM_FMT_BGR:
        memcpy(dst, bgr, (size_t)w * h * 3);
        return 0;
    default:
        return -1;
    }
}
