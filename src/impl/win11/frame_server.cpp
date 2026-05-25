#include "frame_server.h"

static const DWORD TOTAL_MEM_SIZE = 24 + 1920 * 1080 * 3 / 2;

FrameServer::FrameServer()
    : _hMapFile(nullptr), _hMutex(nullptr), _header(nullptr), _lastIndex(0)
{}

FrameServer::~FrameServer()
{
    if (_header)   UnmapViewOfFile(_header);
    if (_hMapFile) CloseHandle(_hMapFile);
    if (_hMutex)   CloseHandle(_hMutex);
}

HRESULT FrameServer::Initialize()
{
    SECURITY_DESCRIPTOR sd;
    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
        return HRESULT_FROM_WIN32(GetLastError());

    SECURITY_ATTRIBUTES sa;
    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle       = FALSE;

    _hMapFile = CreateFileMappingW(
        INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
        0, TOTAL_MEM_SIZE, GVCAM_IPC_NAME);

    if (!_hMapFile)
        return HRESULT_FROM_WIN32(GetLastError());

    _header = (GVCamSharedHeader *)MapViewOfFile(
        _hMapFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

    if (!_header)
        return HRESULT_FROM_WIN32(GetLastError());

    ZeroMemory(_header, sizeof(GVCamSharedHeader));
    _header->width     = 1920;
    _header->height    = 1080;
    _header->stride    = 1920;
    _header->frameSize = 1920 * 1080 * 3 / 2;

    _hMutex = CreateMutexW(&sa, FALSE, GVCAM_MUTEX_NAME);

    return S_OK;
}

HRESULT FrameServer::GetLatestFrame(BYTE **data, DWORD *length, UINT64 *frameIndex)
{
    if (!_header)
        return E_FAIL;

    if (_hMutex)
        WaitForSingleObject(_hMutex, 5);

    if (_header->frameIndex == _lastIndex) {
        if (_hMutex) ReleaseMutex(_hMutex);
        return E_PENDING;
    }

    *data       = _header->data;
    *length     = _header->frameSize;
    *frameIndex = _header->frameIndex;
    _lastIndex  = _header->frameIndex;

    if (_hMutex) ReleaseMutex(_hMutex);
    return S_OK;
}

UINT32 FrameServer::GetWidth()  const { return _header ? _header->width  : 0; }
UINT32 FrameServer::GetHeight() const { return _header ? _header->height : 0; }
