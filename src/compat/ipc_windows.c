/*
 * ipc_windows.c — Windows shared-memory IPC implementation.
 *
 * Uses Global\ namespace so the segment is visible across Session 0
 * (where mfpmp.exe loads the virtual camera DLL) and the user session
 * (where the companion process runs).
 */
#include "gvcam/ipc.h"

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#define MUTEX_SUFFIX L"_Mutex"

struct gvcam_ipc_s {
    HANDLE   hMapping;
    HANDLE   hMutex;
    uint8_t *view;              /* mapped view                            */
    size_t   total_size;        /* desc + payload                        */
    uint64_t last_frame_index;  /* for detecting new frames (consumer)    */
    int      is_producer;
};

/* ── helpers ────────────────────────────────────────────────────────── */

static SECURITY_ATTRIBUTES *null_dacl_sa(void)
{
    static SECURITY_ATTRIBUTES sa;
    static SECURITY_DESCRIPTOR sd;
    static int init = 0;
    if (!init) {
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
        sa.nLength              = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle       = FALSE;
        init = 1;
    }
    return &sa;
}

static HANDLE create_mutex(const wchar_t *name)
{
    return CreateMutexW(null_dacl_sa(), FALSE, name);
}

/* ── public API ─────────────────────────────────────────────────────── */

int gvcam_ipc_open(gvcam_ipc_t *ch, const char *name, gvcam_ipc_role_t role,
                   size_t frame_size)
{
    memset(ch, 0, sizeof(*ch));
    ch->is_producer = (role == GVCAM_IPC_PRODUCER);

    size_t total = GVCAM_FRAME_DESC_SIZE + frame_size;

    /* Build wide-char names */
    wchar_t wname[256], wmutex[256];
    int n = MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
    if (n == 0) return -1;
    n = MultiByteToWideChar(CP_UTF8, 0, name, -1, wmutex, 256);
    if (n == 0) return -1;
    wcsncat_s(wmutex, 256, MUTEX_SUFFIX, _TRUNCATE);

    if (ch->is_producer) {
        /* Create the shared memory segment */
        ch->hMapping = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            null_dacl_sa(),
            PAGE_READWRITE,
            0, (DWORD)total,
            wname);
        if (!ch->hMapping) return -1;
    } else {
        /* Open existing segment (poll up to 30 s) */
        for (int i = 0; i < 30; i++) {
            ch->hMapping = OpenFileMappingW(
                FILE_MAP_READ | FILE_MAP_WRITE, FALSE, wname);
            if (ch->hMapping) break;
            Sleep(1000);
        }
        if (!ch->hMapping) return -1;
    }

    ch->view = (uint8_t *)MapViewOfFile(
        ch->hMapping,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0, 0, 0);
    if (!ch->view) {
        CloseHandle(ch->hMapping);
        ch->hMapping = NULL;
        return -1;
    }
    ch->total_size = total;

    /* Mutex (non-fatal if creation fails — works lockless) */
    ch->hMutex = create_mutex(wmutex);

    /* Producer initialises the descriptor fields */
    if (ch->is_producer) {
        gvcam_frame_desc_t init_desc;
        memset(&init_desc, 0, sizeof(init_desc));
        memcpy(ch->view, &init_desc, sizeof(init_desc));
    }

    return 0;
}

void gvcam_ipc_close(gvcam_ipc_t *ch)
{
    if (ch->view) {
        UnmapViewOfFile(ch->view);
        ch->view = NULL;
    }
    if (ch->hMutex) {
        CloseHandle(ch->hMutex);
        ch->hMutex = NULL;
    }
    if (ch->hMapping) {
        CloseHandle(ch->hMapping);
        ch->hMapping = NULL;
    }
}

int gvcam_ipc_write(gvcam_ipc_t *ch, const gvcam_frame_desc_t *desc,
                    const void *data, size_t len)
{
    if (!ch->is_producer || !ch->view) return -1;

    if (ch->hMutex)
        WaitForSingleObject(ch->hMutex, 5);

    memcpy(ch->view, desc, GVCAM_FRAME_DESC_SIZE);
    memcpy(ch->view + GVCAM_FRAME_DESC_SIZE, data, len);

    if (ch->hMutex)
        ReleaseMutex(ch->hMutex);

    return 0;
}

int gvcam_ipc_read(gvcam_ipc_t *ch, gvcam_frame_desc_t *desc,
                   const void **data, size_t *len)
{
    if (ch->is_producer || !ch->view) return -1;

    if (ch->hMutex)
        WaitForSingleObject(ch->hMutex, 5);

    gvcam_frame_desc_t cur;
    memcpy(&cur, ch->view, sizeof(cur));

    /* Check for new frame */
    if (cur.frame_index == ch->last_frame_index) {
        if (ch->hMutex) ReleaseMutex(ch->hMutex);
        if (data) *data = NULL;
        if (len)  *len  = 0;
        return 1; /* no new frame */
    }

    ch->last_frame_index = cur.frame_index;

    /* Copy out descriptor and point into the shared mapping */
    if (desc) memcpy(desc, &cur, sizeof(cur));
    if (data) *data = ch->view + GVCAM_FRAME_DESC_SIZE;
    if (len)  *len  = cur.data_size;

    if (ch->hMutex)
        ReleaseMutex(ch->hMutex);

    return 0;
}
