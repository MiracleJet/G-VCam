#pragma once
#include <windows.h>

// Starts the MJPEG→shared-memory bridge on a background thread.
// Connects to TCP :8080, parses the multipart MJPEG stream, decodes JPEG
// via WIC, converts BGR→NV12 via gvcam_core, and writes into
// Global\GVCam_FrameBuf so the virtual camera driver can serve it.
//
// Returns a Win32 thread HANDLE, or NULL on failure.
// Caller must CloseHandle when the thread exits.
HANDLE StartMjpegBridge();
