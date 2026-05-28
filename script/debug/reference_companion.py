"""
reference_companion.py — BestCam Minimal Frame Producer
======================================================
This is a minimal Python reference script to demonstrate pushing frames
into the BestCam shared memory segment. 
Requires: pip install numpy opencv-python
"""

import cv2
import numpy as np
import ctypes
import ctypes.wintypes as wintypes
import time
import struct
import math

# SHARED_MEM_NAME = "Global\\BestCam_SharedMem"
# MUTEX_NAME      = "Global\\BestCam_Mutex"
SHARED_MEM_NAME = "Global\\GVCam_FrameBuf"
MUTEX_NAME      = "Global\\GVCam_FrameBuf_Mutex"

TARGET_WIDTH  = 1920
TARGET_HEIGHT = 1080
FRAME_SIZE    = TARGET_WIDTH * TARGET_HEIGHT * 3 // 2
HEADER_SIZE   = 24
TOTAL_SIZE    = HEADER_SIZE + FRAME_SIZE

HEADER_STATIC = struct.pack('<4I', TARGET_WIDTH, TARGET_HEIGHT, TARGET_WIDTH, FRAME_SIZE)
DESC_FMT = struct.Struct('<5IQ')   # 5 uint32 + pad + 1 uint64 = 32 bytes on x64


def build_desc(width, height, stride, fmt, data_size, frame_index):
    """Pack a gvcam_frame_desc_t into 32 bytes (x86-64 ABI)."""
    return DESC_FMT.pack(width, height, stride, fmt, data_size, frame_index)

def main():
    print(f"Opening shared memory: {SHARED_MEM_NAME} ...")
    # k32 = ctypes.WinDLL('kernel32', use_last_error=True)

    k32 = ctypes.WinDLL('kernel32', use_last_error=True)

    # 声明函数原型
    k32.OpenFileMappingW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
    k32.OpenFileMappingW.restype = wintypes.HANDLE
    k32.MapViewOfFile.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, ctypes.c_size_t]
    k32.MapViewOfFile.restype = ctypes.c_void_p
    k32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
    k32.UnmapViewOfFile.restype = wintypes.BOOL
    k32.CloseHandle.argtypes = [wintypes.HANDLE]
    k32.CloseHandle.restype = wintypes.BOOL

    # Wait for the host to create the shared memory
    hMap = None
    for _ in range(30):
        hMap = k32.OpenFileMappingW(0x0002 | 0x0004, False, SHARED_MEM_NAME)
        if hMap:
            break
        time.sleep(1)
        
    if not hMap:
        print("Could not find shared memory. Is BestCamHost.exe running?")
        return

    ptr = k32.MapViewOfFile(hMap, 0x0002 | 0x0004, 0, 0, TOTAL_SIZE)
    if not ptr:
        print("Failed to map view of file.")
        return

    print("Shared memory connected. Generating test frames...")
    
    frame_index = 0
    nv12 = np.empty(FRAME_SIZE, dtype=np.uint8)
    
    try:
        while True:
            # Generate a test pattern (moving color bars or similar)
            # To keep it simple, let's just make a blank frame with a moving square
            t = time.time()
            x = int((math.sin(t * 2) + 1) / 2 * (TARGET_WIDTH - 200))
            y = int((math.cos(t * 3) + 1) / 2 * (TARGET_HEIGHT - 200))
            
            bgr = np.zeros((TARGET_HEIGHT, TARGET_WIDTH, 3), dtype=np.uint8)
            bgr[:, :] = (30, 30, 30) # Dark gray background
            cv2.rectangle(bgr, (x, y), (x+200, y+200), (0, 255, 0), -1)
            cv2.putText(bgr, f"BestCam Virtual Camera - Frame {frame_index}", 
                        (50, 100), cv2.FONT_HERSHEY_SIMPLEX, 2, (255, 255, 255), 3)

            # Convert to NV12
            yuv = cv2.cvtColor(bgr, cv2.COLOR_BGR2YUV_I420)
            uv_off = TARGET_WIDTH * TARGET_HEIGHT
            nv12[:uv_off] = yuv[:TARGET_HEIGHT].ravel()
            
            # Interleave U and V
            u = yuv[TARGET_HEIGHT : TARGET_HEIGHT + TARGET_HEIGHT // 4].ravel()
            v = yuv[TARGET_HEIGHT + TARGET_HEIGHT // 4 : TARGET_HEIGHT + TARGET_HEIGHT // 2].ravel()
            nv12[uv_off   ::2] = u
            nv12[uv_off + 1::2] = v

            # Write to shared memory
            hdr = HEADER_STATIC + struct.pack('<Q', frame_index)
            # hdr = build_desc(TARGET_WIDTH, TARGET_HEIGHT,
            #                   TARGET_WIDTH,       # stride
            #                   0,                  # GVCAM_FMT_NV12
            #                   FRAME_SIZE,
            #                   frame_index)
            ctypes.memmove(ptr, hdr, HEADER_SIZE)
            ctypes.memmove(ptr + HEADER_SIZE, nv12.ctypes.data, FRAME_SIZE)
            
            frame_index += 1
            time.sleep(1/30.0) # ~30 fps
            
    except KeyboardInterrupt:
        print("Stopping...")
    finally:
        k32.UnmapViewOfFile(ctypes.c_void_p(ptr))
        k32.CloseHandle(hMap)

if __name__ == '__main__':
    main()
