#ifndef GVCAM_JPEG_DECODER_H
#define GVCAM_JPEG_DECODER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Thin wrapper around libjpeg-turbo for decoding JPEG -> BGR.
   Returns a malloc'd BGR buffer (caller frees).  *width/*height are filled. */
uint8_t *gvcam_jpeg_decode(const uint8_t *jpeg, size_t len,
                           int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif /* GVCAM_JPEG_DECODER_H */
