#include "mediastream.h"
#include "mediasource.h"
#include <mfapi.h>
#include <mferror.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")

GVCamMediaStream::GVCamMediaStream() : _active(false), _lastFrameIndex(0) {}
GVCamMediaStream::~GVCamMediaStream() {}

HRESULT GVCamMediaStream::RuntimeClassInitialize(GVCamMediaSource *pSource)
{
    _source = pSource;

    HRESULT hr = MFCreateEventQueue(&_eventQueue);
    if (FAILED(hr)) return hr;

    ComPtr<IMFMediaType> mediaType;
    hr = CreateDefaultMediaType(&mediaType);
    if (FAILED(hr)) return hr;

    IMFMediaType* typeArr[] = { mediaType.Get() };
    hr = MFCreateStreamDescriptor(0, 1, typeArr, &_streamDescriptor);
    if (FAILED(hr)) return hr;

    hr = SetCurrentMediaTypeOnHandler();
    if (FAILED(hr)) return hr;

    _frameServer = std::make_unique<FrameServer>();
    _frameServer->Initialize();

    return S_OK;
}

HRESULT GVCamMediaStream::CreateDefaultMediaType(IMFMediaType** ppMediaType)
{
    const UINT32 WIDTH  = 1920;
    const UINT32 HEIGHT = 1080;
    const UINT32 STRIDE = WIDTH;
    const UINT32 SAMPLE_SIZE = WIDTH * HEIGHT * 3 / 2;

    ComPtr<IMFMediaType> mediaType;
    HRESULT hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr)) return hr;

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);                      if (FAILED(hr)) return hr;
    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);                        if (FAILED(hr)) return hr;
    hr = MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, WIDTH, HEIGHT);         if (FAILED(hr)) return hr;
    hr = MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, 30, 1);                if (FAILED(hr)) return hr;
    hr = MFSetAttributeRatio(mediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);         if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);     if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);                    if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE);                         if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_SAMPLE_SIZE, SAMPLE_SIZE);                         if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, STRIDE);                           if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Normal);       if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_BT709);          if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_YUV_MATRIX, MFVideoTransferMatrix_BT709);          if (FAILED(hr)) return hr;
    hr = mediaType->SetUINT32(MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_709);          if (FAILED(hr)) return hr;

    *ppMediaType = mediaType.Detach();
    return S_OK;
}

HRESULT GVCamMediaStream::SetCurrentMediaTypeOnHandler()
{
    ComPtr<IMFMediaTypeHandler> handler;
    HRESULT hr = _streamDescriptor->GetMediaTypeHandler(&handler);
    if (FAILED(hr)) return hr;

    ComPtr<IMFMediaType> mediaType;
    hr = handler->GetMediaTypeByIndex(0, &mediaType);
    if (FAILED(hr)) return hr;

    return handler->SetCurrentMediaType(mediaType.Get());
}

void GVCamMediaStream::SetActive(bool active) { _active = active; }

HRESULT GVCamMediaStream::FireStreamStarted(const PROPVARIANT* pvarStartPosition)
{
    return _eventQueue->QueueEventParamVar(MEStreamStarted, GUID_NULL, S_OK, pvarStartPosition);
}

STDMETHODIMP GVCamMediaStream::GetMediaSource(IMFMediaSource** ppMediaSource)
{
    return _source.CopyTo(ppMediaSource);
}

STDMETHODIMP GVCamMediaStream::GetStreamDescriptor(IMFStreamDescriptor** ppStreamDescriptor)
{
    if (!ppStreamDescriptor) return E_POINTER;
    *ppStreamDescriptor = _streamDescriptor.Get();
    if (_streamDescriptor) _streamDescriptor->AddRef();
    return S_OK;
}

STDMETHODIMP GVCamMediaStream::RequestSample(IUnknown* pToken)
{
    if (!_active) return MF_E_MEDIA_SOURCE_WRONGSTATE;

    BYTE  *srcData   = nullptr;
    DWORD  srcLength  = 0;
    UINT64 frameIdx   = 0;

    HRESULT hr = _frameServer->GetLatestFrame(&srcData, &srcLength, &frameIdx);

    if (SUCCEEDED(hr) && srcData && srcLength > 0) {
        _lastFrame.assign(srcData, srcData + srcLength);
        _lastFrameIndex = frameIdx;
    }

    if (_lastFrame.empty()) {
        QueueBlankSample(pToken);
        return S_OK;
    }

    ComPtr<IMFSample> sample;
    hr = MFCreateSample(&sample);
    if (FAILED(hr)) return hr;

    ComPtr<IMFMediaBuffer> buffer;
    hr = MFCreateMemoryBuffer((DWORD)_lastFrame.size(), &buffer);
    if (FAILED(hr)) return hr;

    BYTE* dst = nullptr;
    hr = buffer->Lock(&dst, nullptr, nullptr);
    if (FAILED(hr)) return hr;

    memcpy(dst, _lastFrame.data(), _lastFrame.size());
    buffer->Unlock();
    buffer->SetCurrentLength((DWORD)_lastFrame.size());

    sample->AddBuffer(buffer.Get());
    sample->SetSampleTime(MFGetSystemTime());
    sample->SetSampleDuration(333333);

    if (pToken)
        sample->SetUnknown(MFSampleExtension_Token, pToken);

    _eventQueue->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, sample.Get());
    return S_OK;
}

void GVCamMediaStream::QueueBlankSample(IUnknown* pToken)
{
    ComPtr<IMFSample> sample;
    if (SUCCEEDED(MFCreateSample(&sample))) {
        if (pToken)
            sample->SetUnknown(MFSampleExtension_Token, pToken);
        _eventQueue->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, sample.Get());
    }
}

void GVCamMediaStream::Shutdown()
{
    _eventQueue->Shutdown();
}

STDMETHODIMP GVCamMediaStream::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
    { return _eventQueue->GetEvent(dwFlags, ppEvent); }
STDMETHODIMP GVCamMediaStream::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
    { return _eventQueue->BeginGetEvent(pCallback, punkState); }
STDMETHODIMP GVCamMediaStream::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
    { return _eventQueue->EndGetEvent(pResult, ppEvent); }
STDMETHODIMP GVCamMediaStream::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
    { return _eventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue); }
