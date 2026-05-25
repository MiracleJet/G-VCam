#ifndef GVCAM_IPC_H
#define GVCAM_IPC_H

#include <stdint.h>
#include "gvcam/frame.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Platform-independent IPC channel ────────────────────── */

/* Direction: producer writes frames, consumer reads them. */
typedef enum {
    GVCAM_IPC_PRODUCER,
    GVCAM_IPC_CONSUMER
} gvcam_ipc_role_t;

typedef struct gvcam_ipc_s gvcam_ipc_t;

/*
 * Open (or create) a named IPC channel.
 *   name      — platform-specific name (Windows: "Global\\Name", POSIX: "/name")
 *   role      — PRODUCER creates and writes; CONSUMER opens and reads
 *   frame_size— expected payload size (width*height*bpp for packed format)
 * Returns 0 on success, -1 on error.
 */
int  gvcam_ipc_open(gvcam_ipc_t *ch, const char *name, gvcam_ipc_role_t role,
                    size_t frame_size);

/* Close the channel, release all resources. */
void gvcam_ipc_close(gvcam_ipc_t *ch);

/*
 * Write a frame into the channel (PRODUCER).
 *   desc — metadata (width, height, format, frame_index, etc.)
 *   data — raw pixel buffer
 *   len  — data length (must match desc->data_size)
 * Returns 0 on success.
 */
int  gvcam_ipc_write(gvcam_ipc_t *ch, const gvcam_frame_desc_t *desc,
                     const void *data, size_t len);

/*
 * Read latest frame from the channel (CONSUMER).
 *   desc — [out] frame metadata
 *   data — [out] pointer directly into the shared mapping (zero-copy).
 *          Valid until next gvcam_ipc_read() or gvcam_ipc_close().
 *   len  — [out] actual payload length
 * Returns  0 on success (new frame available).
 * Returns  1 if no new frame since last read.
 * Returns -1 on error.
 */
int  gvcam_ipc_read(gvcam_ipc_t *ch, gvcam_frame_desc_t *desc,
                    const void **data, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* GVCAM_IPC_H */
