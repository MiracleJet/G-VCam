#include "mjpeg_parser.h"

#include <stdlib.h>
#include <string.h>

/* The Android MJPEG server prepends "--boundary\r\n" before each frame.
   Between frames we scan for "\r\n--boundary\r\n".                      */

void gvcam_mjpeg_init(gvcam_mjpeg_parser_t *p, const char *boundary)
{
    memset(p, 0, sizeof(*p));
    p->boundary     = strdup(boundary);
    p->boundary_len = strlen(boundary);
}

void gvcam_mjpeg_free(gvcam_mjpeg_parser_t *p)
{
    free(p->boundary);
    free(p->buf);
    memset(p, 0, sizeof(*p));
}

/*
 * append raw bytes to the internal ring buffer.
 */
static int buf_append(gvcam_mjpeg_parser_t *p, const uint8_t *data, size_t len)
{
    size_t need = p->buf_len + len;
    if (need > p->buf_cap) {
        size_t cap = p->buf_cap ? p->buf_cap * 2 : 65536;
        while (cap < need) cap *= 2;
        uint8_t *n = (uint8_t *)realloc(p->buf, cap);
        if (!n) return -1;
        p->buf     = n;
        p->buf_cap = cap;
    }
    memcpy(p->buf + p->buf_len, data, len);
    p->buf_len = need;
    return 0;
}

int gvcam_mjpeg_feed(gvcam_mjpeg_parser_t *p,
                     const uint8_t *data, size_t len,
                     uint8_t **jpeg_out, size_t *len_out)
{
    if (buf_append(p, data, len) != 0) return -1;

    /* Look for boundary markers in the current buffer.
       Frame header format:  "--BOUNDARY\r\nContent-Type: image/jpeg\r\nContent-Length: N\r\n\r\n"
       Frame ends before the next boundary or end-of-stream marker.         */

    while (p->buf_len > p->boundary_len + 4) {

        /* Find boundary */
        const uint8_t *pos = (const uint8_t *)memmem(
            p->buf, p->buf_len,
            p->boundary, p->boundary_len);

        if (!pos) {
            /* Discard everything — we only have partial data before the first boundary */
            p->buf_len = 0;
            return 0;
        }

        size_t offset = (size_t)(pos - p->buf);

        /* Check for end-of-stream marker: boundary + "--" */
        if (offset + p->boundary_len + 2 <= p->buf_len &&
            memcmp(pos + p->boundary_len, "--", 2) == 0) {
            p->buf_len = 0;
            return -1; /* stream ended */
        }

        /* Skip past "boundary\r\n" to reach part headers */
        size_t hdr_start = offset + p->boundary_len;
        if (hdr_start + 2 <= p->buf_len &&
            p->buf[hdr_start] == '\r' && p->buf[hdr_start + 1] == '\n')
            hdr_start += 2;

        /* Find end of part headers: "\r\n\r\n" */
        const uint8_t *hdr_end = (const uint8_t *)memmem(
            p->buf + hdr_start, p->buf_len - hdr_start,
            "\r\n\r\n", 4);
        if (!hdr_end) return 0; /* need more data */

        size_t body_start = (size_t)(hdr_end - p->buf) + 4;

        /* Parse Content-Length from headers */
        size_t content_len = 0;
        {
            size_t hdr_bytes = (size_t)(hdr_end - (p->buf + hdr_start));
            char *hdr_text = (char *)malloc(hdr_bytes + 1);
            if (!hdr_text) return -1;
            memcpy(hdr_text, p->buf + hdr_start, hdr_bytes);
            hdr_text[hdr_bytes] = '\0';

            char *cl = strstr(hdr_text, "Content-Length:");
            if (!cl) cl = strstr(hdr_text, "content-length:");
            if (cl) {
                content_len = (size_t)strtoul(cl + 15, NULL, 10);
            }
            free(hdr_text);
        }

        if (content_len == 0 || body_start + content_len > p->buf_len)
            return 0; /* need more data */

        /* Extract JPEG frame */
        *jpeg_out = (uint8_t *)malloc(content_len);
        if (!*jpeg_out) return -1;
        memcpy(*jpeg_out, p->buf + body_start, content_len);
        *len_out = content_len;

        /* Advance buffer past this frame (skip trailing \r\n after JPEG body) */
        size_t next = body_start + content_len;
        if (next + 2 <= p->buf_len &&
            p->buf[next] == '\r' && p->buf[next + 1] == '\n')
            next += 2;

        size_t remain = p->buf_len - next;
        if (remain > 0)
            memmove(p->buf, p->buf + next, remain);
        p->buf_len = remain;

        return 1; /* frame ready */
    }

    return 0; /* need more data */
}
