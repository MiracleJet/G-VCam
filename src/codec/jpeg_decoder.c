#include "jpeg_decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <turbojpeg.h>

uint8_t *gvcam_jpeg_decode(const uint8_t *jpeg, size_t len,
                           int *width, int *height)
{
    tjhandle tj = tjInitDecompress();
    if (!tj) return NULL;

    int w = 0, h = 0, subsamp = 0, cs = 0;
    if (tjDecompressHeader3(tj, jpeg, (unsigned long)len,
                            &w, &h, &subsamp, &cs) != 0) {
        tjDestroy(tj);
        return NULL;
    }

    /* BGR = TJPF_BGR (matches Windows / OpenCV conventions) */
    size_t bgr_size = (size_t)w * h * 3;
    uint8_t *bgr = (uint8_t *)malloc(bgr_size);
    if (!bgr) {
        tjDestroy(tj);
        return NULL;
    }

    if (tjDecompress2(tj, jpeg, (unsigned long)len,
                      bgr, w, 0/*pitch*/, h, TJPF_BGR,
                      TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0) {
        free(bgr);
        tjDestroy(tj);
        return NULL;
    }

    tjDestroy(tj);
    *width  = w;
    *height = h;
    return bgr;
}
