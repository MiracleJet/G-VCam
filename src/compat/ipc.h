#ifndef GVCAM_IPC_H
#define GVCAM_IPC_H

#include <stddef.h>
#include <stdint.h>
#include "frame.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GVCAM_IPC_PRODUCER,
    GVCAM_IPC_CONSUMER
} gvcam_ipc_role_t;

typedef struct gvcam_ipc_s gvcam_ipc_t;

int  gvcam_ipc_open(gvcam_ipc_t *ch, const char *name, gvcam_ipc_role_t role,
                    size_t frame_size);
void gvcam_ipc_close(gvcam_ipc_t *ch);

int  gvcam_ipc_write(gvcam_ipc_t *ch, const gvcam_frame_desc_t *desc,
                     const void *data, size_t len);

int  gvcam_ipc_read(gvcam_ipc_t *ch, gvcam_frame_desc_t *desc,
                    const void **data, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* GVCAM_IPC_H */
