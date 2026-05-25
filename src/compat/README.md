# src/compat — Platform IPC Implementations

Internal headers and platform-specific IPC backends. Not installed.

| File | Platform | Purpose |
|---|---|---|
| `ipc_windows.c` / `ipc_windows.h` | Windows | Shared memory + mutex via `Global\` namespace |
| `ipc_posix.c` | Linux/macOS | (future) POSIX shared memory |
