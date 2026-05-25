// Simple thread-safe logging to C:\GVCam.log (Session-0 accessible path)
#pragma once
#include <windows.h>
#include <stdio.h>

static __inline void GVCamLog(const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    FILE *f = NULL;
    if (fopen_s(&f, "C:\\GVCam.log", "a") == 0) {
        fprintf(f, "[%lu] %s\n", GetCurrentThreadId(), buf);
        fclose(f);
    }
}
