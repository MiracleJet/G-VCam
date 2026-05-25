#ifndef GVCAM_MJPEG_PARSER_H
#define GVCAM_MJPEG_PARSER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Streaming MJPEG (multipart/x-mixed-replace) parser.
   Feed raw TCP bytes; yields complete JPEG frames.  */

typedef struct {
    char    *boundary;
    size_t   boundary_len;
    uint8_t *buf;
    size_t   buf_len;
    size_t   buf_cap;
} gvcam_mjpeg_parser_t;

/* Initialize with the boundary string (e.g. "--boundary").
   The boundary and its CRLF variants are computed internally. */
void gvcam_mjpeg_init(gvcam_mjpeg_parser_t *p, const char *boundary);
void gvcam_mjpeg_free(gvcam_mjpeg_parser_t *p);

/* Feed raw bytes received from the TCP socket.
   If a complete JPEG frame was extracted, *jpeg_out is set to a malloc'd
   buffer (caller frees with free()) and *len_out gets its size.
   Returns 1 when a frame is ready, 0 when more data is needed, -1 on error. */
int  gvcam_mjpeg_feed(gvcam_mjpeg_parser_t *p,
                      const uint8_t *data, size_t len,
                      uint8_t **jpeg_out, size_t *len_out);

#ifdef __cplusplus
}
#endif

#endif /* GVCAM_MJPEG_PARSER_H */
