#include "../ipc.h"
#include <stdlib.h>
#include <string.h>

struct gvcam_ipc_s {
    int      fd;
    uint8_t *view;
    size_t   total_size;
    uint64_t last_frame_index;
    int      is_producer;
};

int gvcam_ipc_open(gvcam_ipc_t *ch, const char *name, gvcam_ipc_role_t role,
                   size_t frame_size)
{
    (void)name; (void)role; (void)frame_size;
    memset(ch, 0, sizeof(*ch));
    return -1;
}

void gvcam_ipc_close(gvcam_ipc_t *ch)
{
    (void)ch;
}

int gvcam_ipc_write(gvcam_ipc_t *ch, const gvcam_frame_desc_t *desc,
                    const void *data, size_t len)
{
    (void)ch; (void)desc; (void)data; (void)len;
    return -1;
}

int gvcam_ipc_read(gvcam_ipc_t *ch, gvcam_frame_desc_t *desc,
                   const void **data, size_t *len)
{
    (void)ch; (void)desc; (void)data; (void)len;
    return -1;
}
