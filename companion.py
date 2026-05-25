"""
companion.py — GVCam Frame Producer
=====================================
Pushes NV12 test frames into the GVCam shared-memory segment so the
virtual camera driver has content to serve to apps.

Requires:  pip install numpy opencv-python
Platform:  Windows only (Win32 shared memory + mutex)
"""

import cv2
import numpy as np
import ctypes
import ctypes.wintypes as wintypes
import time
import struct
import math

SHARED_MEM_NAME = "Global\\GVCam_FrameBuf"
MUTEX_NAME      = "Global\\GVCam_FrameBuf_Mutex"

TARGET_WIDTH  = 1920
TARGET_HEIGHT = 1080
FRAME_SIZE    = TARGET_WIDTH * TARGET_HEIGHT * 3 // 2   # NV12 = 3,110,400 bytes

# GVCamSharedHeader (packed, 24 bytes — matching BestCam protocol):
#   UINT32 width       (offset  0)
#   UINT32 height      (offset  4)
#   UINT32 stride      (offset  8)
#   UINT32 frameSize   (offset 12)
#   UINT64 frameIndex  (offset 16)
HEADER_SIZE   = 24
HEADER_STATIC_FMT = struct.Struct('<4I')   # width, height, stride, frameSize = 16 bytes
TOTAL_SIZE    = HEADER_SIZE + FRAME_SIZE


def main():
    k32 = ctypes.WinDLL('kernel32', use_last_error=True)

    k32.OpenFileMappingW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
    k32.OpenFileMappingW.restype = wintypes.HANDLE

    k32.MapViewOfFile.argtypes = [
        wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD,
        wintypes.DWORD, ctypes.c_size_t]
    k32.MapViewOfFile.restype = ctypes.c_void_p

    k32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
    k32.UnmapViewOfFile.restype = wintypes.BOOL

    k32.CloseHandle.argtypes = [wintypes.HANDLE]
    k32.CloseHandle.restype = wintypes.BOOL

    k32.CreateMutexW.argtypes = [ctypes.c_void_p, wintypes.BOOL, wintypes.LPCWSTR]
    k32.CreateMutexW.restype = wintypes.HANDLE

    k32.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    k32.WaitForSingleObject.restype = wintypes.DWORD

    k32.ReleaseMutex.argtypes = [wintypes.HANDLE]
    k32.ReleaseMutex.restype = wintypes.BOOL

    print(f"Opening shared memory: {SHARED_MEM_NAME} ...")
    hMap = None
    for _ in range(60):
        hMap = k32.OpenFileMappingW(0x0002 | 0x0004, False, SHARED_MEM_NAME)
        if hMap:
            break
        time.sleep(1)
    if not hMap:
        print("Shared memory not found. Is the camera active in an app? "
              "(Open Zoom/OBS and select GVCam Virtual Camera first.)")
        return

    ptr = k32.MapViewOfFile(hMap, 0x0002 | 0x0004, 0, 0, TOTAL_SIZE)
    if not ptr:
        print("MapViewOfFile failed.")
        k32.CloseHandle(hMap)
        return

    hMutex = k32.CreateMutexW(None, False, MUTEX_NAME)
    print("Shared memory connected. Generating test frames...")

    frame_index = 0
    nv12 = np.empty(FRAME_SIZE, dtype=np.uint8)

    try:
        while True:
            t = time.time()
            x = int((math.sin(t * 2) + 1) / 2 * (TARGET_WIDTH - 200))
            y = int((math.cos(t * 3) + 1) / 2 * (TARGET_HEIGHT - 200))

            bgr = np.zeros((TARGET_HEIGHT, TARGET_WIDTH, 3), dtype=np.uint8)
            bgr[:, :] = (30, 30, 30)
            cv2.rectangle(bgr, (x, y), (x + 200, y + 200), (0, 255, 0), -1)
            cv2.putText(bgr, f"GVCam Virtual Camera - Frame {frame_index}",
                        (50, 100), cv2.FONT_HERSHEY_SIMPLEX, 2, (255, 255, 255), 3)

            # BGR → NV12 (correctly slice U/V planes to w/2, h/4 each)
            yuv = cv2.cvtColor(bgr, cv2.COLOR_BGR2YUV_I420)
            uv_off = TARGET_WIDTH * TARGET_HEIGHT
            h_quarter = TARGET_HEIGHT // 4
            w_half = TARGET_WIDTH // 2
            nv12[:uv_off] = yuv[:TARGET_HEIGHT, :].ravel()
            u = yuv[TARGET_HEIGHT       :TARGET_HEIGHT + h_quarter,       :w_half].ravel()
            v = yuv[TARGET_HEIGHT + h_quarter:TARGET_HEIGHT + h_quarter * 2, :w_half].ravel()
            nv12[uv_off::2] = u
            nv12[uv_off + 1::2] = v

            desc = HEADER_STATIC_FMT.pack(TARGET_WIDTH, TARGET_HEIGHT,
                                          TARGET_WIDTH, FRAME_SIZE)
            desc += struct.pack('<Q', frame_index)

            if hMutex:
                k32.WaitForSingleObject(hMutex, 5)
            ctypes.memmove(ptr, desc, HEADER_SIZE)
            ctypes.memmove(ptr + HEADER_SIZE, nv12.ctypes.data, FRAME_SIZE)
            if hMutex:
                k32.ReleaseMutex(hMutex)
            print(f"Generate {frame_index}\n")
            frame_index += 1
            time.sleep(1 / 30.0)

    except KeyboardInterrupt:
        print("Stopping...")
    finally:
        k32.UnmapViewOfFile(ctypes.c_void_p(ptr))
        if hMutex:
            k32.CloseHandle(hMutex)
        k32.CloseHandle(hMap)


if __name__ == '__main__':
    main()
