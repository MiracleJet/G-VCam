/* Internal header — not installed. Full IPC struct definition for Windows. */
#ifndef GVCAM_IPC_WINDOWS_H
#define GVCAM_IPC_WINDOWS_H

#include "gvcam/ipc.h"
#include <windows.h>

struct gvcam_ipc_s {
    HANDLE   hMapping;
    HANDLE   hMutex;
    uint8_t *view;
    size_t   total_size;
    uint64_t last_frame_index;
    int      is_producer;
};

#endif
