#ifndef GVCAM_WIN_FRAME_SERVER_H
#define GVCAM_WIN_FRAME_SERVER_H

#include <windows.h>

#define GVCAM_IPC_NAME   L"Global\\GVCam_FrameBuf"
#define GVCAM_MUTEX_NAME L"Global\\GVCam_FrameBuf_Mutex"

#pragma pack(push, 1)
struct GVCamSharedHeader {
    UINT32  width;
    UINT32  height;
    UINT32  stride;
    UINT32  frameSize;
    UINT64  frameIndex;
    UINT8   data[1];
};
#pragma pack(pop)

class FrameServer {
public:
    FrameServer();
    ~FrameServer();

    HRESULT Initialize();
    HRESULT GetLatestFrame(BYTE **data, DWORD *length, UINT64 *frameIndex);

    UINT32 GetWidth()  const;
    UINT32 GetHeight() const;

private:
    HANDLE             _hMapFile;
    HANDLE             _hMutex;
    GVCamSharedHeader *_header;
    UINT64             _lastIndex;
};

#endif
