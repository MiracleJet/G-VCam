#ifndef GVCAM_FRAME_H
#define GVCAM_FRAME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Resolution defaults ─────────────────────────────────── */
#define GVCAM_DEFAULT_WIDTH   1920
#define GVCAM_DEFAULT_HEIGHT  1080
#define GVCAM_DEFAULT_PORT    8080

/* ── Pixel formats ───────────────────────────────────────── */
typedef enum {
    GVCAM_FMT_NV12 = 0,   /* Windows native: Y + interleaved UV */
    GVCAM_FMT_YUY2,       /* Packed YUV 4:2:2 */
    GVCAM_FMT_I420,       /* Planar YUV 4:2:0 (Linux V4L2 common) */
    GVCAM_FMT_BGR,        /* OpenCV / intermediate */
    GVCAM_FMT_COUNT
} gvcam_pixel_format_t;

/* Returns bytes per pixel (for packed) or 0 for planar formats. */
int gvcam_pixel_format_bpp(gvcam_pixel_format_t fmt);

/* Returns total frame buffer size for a given format/resolution. */
int gvcam_frame_buffer_size(int width, int height, gvcam_pixel_format_t fmt);

/* ── Frame descriptor ────────────────────────────────────── */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;         /* gvcam_pixel_format_t */
    uint32_t data_size;      /* bytes of pixel data */
    uint64_t frame_index;    /* monotonic counter */
} gvcam_frame_desc_t;

#define GVCAM_FRAME_DESC_SIZE  sizeof(gvcam_frame_desc_t)

/* ── IPC frame layout (desc + payload) ───────────────────── */
/* The IPC segment layout: [gvcam_frame_desc_t][raw pixel data]
   Total IPC size = GVCAM_FRAME_DESC_SIZE + frame_buffer_size() */

#ifdef __cplusplus
}
#endif

#endif /* GVCAM_FRAME_H */
