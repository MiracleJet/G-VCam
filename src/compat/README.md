# src/compat — Platform IPC

| File | Purpose |
|---|---|
| `ipc.h` | Shared IPC API (`gvcam_ipc_t`), included by both platform backends |
| `windows/ipc.c` | Windows shared-memory + mutex via `Global\` namespace |
| `posix/ipc.c` | POSIX stub (future: shm_open) |
