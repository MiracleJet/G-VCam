#include "mjpeg_bridge.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>
#include <string>

#include "frame_server.h"
#include "color_convert.h"
#include "logger.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

namespace {

// --- WIC JPEG → BGR --------------------------------------------------

bool WicDecodeJpeg(const uint8_t *jpeg, size_t len,
                   std::vector<uint8_t> &bgr, UINT &w, UINT &h,
                   UINT targetW = 1920, UINT targetH = 1080)
{
    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return false;

    ComPtr<IWICStream> stream;
    hr = factory->CreateStream(&stream);
    if (FAILED(hr)) return false;
    hr = stream->InitializeFromMemory(const_cast<uint8_t *>(jpeg),
                                      static_cast<DWORD>(len));
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromStream(
        stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return false;

    hr = frame->GetSize(&w, &h);
    if (FAILED(hr)) return false;

    IWICBitmapSource *source = frame.Get();

    // Scale to target resolution if needed
    ComPtr<IWICBitmapScaler> scaler;
    if (w != targetW || h != targetH) {
        hr = factory->CreateBitmapScaler(&scaler);
        if (FAILED(hr)) return false;
        hr = scaler->Initialize(source, targetW, targetH,
                                WICBitmapInterpolationModeFant);
        if (FAILED(hr)) return false;
        source = scaler.Get();
        w = targetW;
        h = targetH;
    }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return false;

    hr = converter->Initialize(
        source,
        GUID_WICPixelFormat24bppBGR,
        WICBitmapDitherTypeNone,
        nullptr, 0.f,
        WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return false;

    UINT stride = w * 3;
    size_t size = stride * h;
    bgr.resize(size);
    hr = converter->CopyPixels(nullptr, stride, static_cast<UINT>(size),
                               bgr.data());
    return SUCCEEDED(hr);
}

// --- Winsock TCP client -----------------------------------------------

SOCKET ConnectToAndroid(const char *host)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(8080);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        return INVALID_SOCKET;
    }
    return sock;
}

bool SendHttpRequest(SOCKET sock, const char *host)
{
    char req[256];
    snprintf(req, sizeof(req),
             "GET / HTTP/1.0\r\nHost: %s\r\n\r\n", host);
    return send(sock, req, (int)strlen(req), 0) != SOCKET_ERROR;
}

bool ReadLine(SOCKET sock, std::string &line)
{
    line.clear();
    char ch;
    while (recv(sock, &ch, 1, 0) > 0) {
        line.push_back(ch);
        if (line.size() >= 2 &&
            line[line.size() - 2] == '\r' && line[line.size() - 1] == '\n')
            return true;
    }
    return false;
}

std::string ParseBoundary(const std::string &contentType)
{
    for (size_t p = 0; p < contentType.size(); ) {
        size_t semi = contentType.find(';', p);
        if (semi == std::string::npos) semi = contentType.size();
        std::string part = contentType.substr(p, semi - p);
        // trim
        while (!part.empty() && part.front() == ' ') part.erase(0, 1);
        while (!part.empty() && part.back() == ' ') part.pop_back();
        // match
        const char *pfx = "boundary=";
        if (part.size() > strlen(pfx) &&
            _strnicmp(part.c_str(), pfx, strlen(pfx)) == 0) {
            return part.substr(strlen(pfx));
        }
        p = semi + 1;
    }
    return "--boundary";  // fallback
}

bool SkipResponseHeaders(SOCKET sock, std::string &boundary)
{
    std::string line, contentType;
    int status = 0;
    for (;;) {
        if (!ReadLine(sock, line)) return false;
        if (line == "\r\n") break;  // end of headers
        if (_strnicmp(line.c_str(), "HTTP/", 5) == 0) {
            sscanf_s(line.c_str(), "HTTP/%*d.%*d %d", &status);
        }
        if (_strnicmp(line.c_str(), "Content-Type:", 13) == 0) {
            contentType = line.substr(14);
            contentType.erase(contentType.find_last_not_of(" \r\n") + 1);
        }
    }
    boundary = ParseBoundary(contentType);
    return status == 200;
}

// --- Shared memory ---------------------------------------------------

struct SharedMemGuard {
    HANDLE hMap   = nullptr;
    uint8_t *view = nullptr;

    ~SharedMemGuard() {
        if (view) UnmapViewOfFile(view);
        if (hMap) CloseHandle(hMap);
    }
};

bool OpenSharedMemory(SharedMemGuard &g, DWORD totalSize)
{
    for (int i = 0; i < 60; i++) {
        g.hMap = OpenFileMappingW(
            FILE_MAP_READ | FILE_MAP_WRITE, FALSE, GVCAM_IPC_NAME);
        if (g.hMap) break;
        Sleep(1000);
    }
    if (!g.hMap) return false;

    g.view = (uint8_t *)MapViewOfFile(
        g.hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    return g.view != nullptr;
}

} // anonymous namespace

// --- Public API -------------------------------------------------------

HANDLE StartMjpegBridge(const char *host)
{
    GVCamLog("StartMjpegBridge: initializing, host=%s", host);

    char *hostParam = _strdup(host);

    HANDLE hThread = CreateThread(nullptr, 0, [](LPVOID p) -> DWORD {
        char *host = static_cast<char *>(p);
        printf("[BRIDGE] Thread started\n");
        fflush(stdout);

        HRESULT coHR = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        printf("[BRIDGE] CoInitializeEx: 0x%08lX\n", (long)coHR);
        fflush(stdout);

        // 1. WinSock
        WSADATA wsa{};
        int wsaRc = WSAStartup(MAKEWORD(2, 2), &wsa);
        printf("[BRIDGE] WSAStartup: %d\n", wsaRc);
        fflush(stdout);
        if (wsaRc != 0) {
            CoUninitialize();
            return 1;
        }

        // 2. Shared memory — driver must be active first
        const DWORD TOTAL = 24 + 1920 * 1080 * 3 / 2;
        SharedMemGuard shm;
        printf("[BRIDGE] Opening shared memory...\n");
        fflush(stdout);
        if (!OpenSharedMemory(shm, TOTAL)) {
            printf("[BRIDGE] Shared memory NOT found (open any app using GVCam)\n");
            fflush(stdout);
            WSACleanup();
            CoUninitialize();
            return 1;
        }
        printf("[BRIDGE] Shared memory OK\n");
        fflush(stdout);

        // 3. TCP connect with retry
        SOCKET sock = INVALID_SOCKET;
        std::string boundary;
        printf("[BRIDGE] Connecting to TCP 8080...\n");
        fflush(stdout);
        while (true) {
            sock = ConnectToAndroid(host);
            if (sock != INVALID_SOCKET && SendHttpRequest(sock, host)) {
                if (SkipResponseHeaders(sock, boundary)) break;
                closesocket(sock);
                sock = INVALID_SOCKET;
            }
            printf("[BRIDGE] TCP retry in 2s...\n");
            fflush(stdout);
            Sleep(2000);
        }
        printf("[BRIDGE] MJPEG connected, boundary: %s\n", boundary.c_str());
        fflush(stdout);

        // 4. Parse loop
        // boundary already includes "--" prefix (e.g. "--boundary")
        std::string marker = boundary;
        std::vector<uint8_t> buf;
        char chunk[65536];
        uint64_t frameIndex  = 0;
        int      frameSizeNV = 1920 * 1080 * 3 / 2;
        std::vector<uint8_t> nv12(frameSizeNV);
        // sizeof(GVCamSharedHeader) includes the data[1] tail → 25 bytes,
        // but the shared memory segment is sized for 24-byte header.
        // Use offsetof(data) which gives exactly 24.
        const int HEADER_SZ = offsetof(GVCamSharedHeader, data);

        while (true) {
            int n = recv(sock, chunk, sizeof(chunk), 0);
            if (n <= 0) break;
            buf.insert(buf.end(), chunk, chunk + n);
            if (buf.size() > 4 * 1024 * 1024) {  // 4 MB limit
                GVCamLog("Buffer overflow, resetting");
                buf.clear();
            }

            for (;;) {
                // find boundary
                auto it = std::search(
                    buf.begin(), buf.end(), marker.begin(), marker.end());
                if (it == buf.end()) break;

                size_t pos = it - buf.begin();
                size_t endPos = pos + marker.size();

                // "--" after boundary → end of stream
                if (endPos + 2 <= buf.size() &&
                    buf[endPos] == '-' && buf[endPos + 1] == '-') {
                    buf.clear();
                    goto exit_loop;
                }

                // skip past boundary + \r\n
                size_t hdrStart = endPos;
                if (hdrStart + 2 <= buf.size() &&
                    buf[hdrStart] == '\r' && buf[hdrStart + 1] == '\n')
                    hdrStart += 2;

                // find \r\n\r\n (end of part headers)
                const char needle[] = "\r\n\r\n";
                auto hdrEnd = std::search(
                    buf.begin() + hdrStart, buf.end(),
                    needle, needle + 4);
                if (hdrEnd == buf.end()) break;

                size_t bodyStart = (hdrEnd - buf.begin()) + 4;

                // parse Content-Length
                size_t cl = 0;
                {
                    std::string hdrBlock(buf.begin() + hdrStart,
                                         buf.begin() + bodyStart - 4);
                    const char *clPfx = "Content-Length:";
                    const char *clPfx2 = "content-length:";
                    const char *found = strstr(hdrBlock.c_str(), clPfx);
                    if (!found) found = strstr(hdrBlock.c_str(), clPfx2);
                    if (found)
                        cl = strtoul(found + strlen(clPfx), nullptr, 10);
                }

                if (cl == 0 || bodyStart + cl > buf.size()) break;

                // extract JPEG
                const uint8_t *jpeg = buf.data() + bodyStart;

                // decode
                static int bridgeFrames = 0;
                if (bridgeFrames == 0) {
                    printf("[BRIDGE] First frame received, decoding...\n");
                    fflush(stdout);
                }
                std::vector<uint8_t> bgr;
                UINT w = 0, h = 0;
                if (WicDecodeJpeg(jpeg, cl, bgr, w, h) && !bgr.empty()) {
                    if (bridgeFrames == 0) {
                        printf("[BRIDGE] First frame decoded: %ux%u\n", w, h);
                        fflush(stdout);
                    }
                    bridgeFrames++;

                    // WicDecodeJpeg already scales to 1920x1080
                    gvcam_bgr_to_nv12(bgr.data(), (int)w, (int)h,
                                      nv12.data());

                    // write to shared memory
                    GVCamSharedHeader hdr{};
                    hdr.width     = 1920;
                    hdr.height    = 1080;
                    hdr.stride    = 1920;
                    hdr.frameSize = (UINT32)frameSizeNV;
                    hdr.frameIndex = frameIndex++;

                    memcpy(shm.view, &hdr, HEADER_SZ);
                    memcpy(shm.view + HEADER_SZ, nv12.data(), frameSizeNV);
                }
                // advance past this frame
                size_t next = bodyStart + cl;
                if (next + 2 <= buf.size() &&
                    buf[next] == '\r' && buf[next + 1] == '\n')
                    next += 2;
                buf.erase(buf.begin(), buf.begin() + next);
            }
        }
    exit_loop:

        printf("[BRIDGE] Exiting, %llu frames\n", frameIndex);
        fflush(stdout);
        closesocket(sock);
        WSACleanup();
        CoUninitialize();
        free(host);
        return 0;
    }, hostParam, 0, nullptr);

    return hThread;
}
