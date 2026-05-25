#include "mediasource.h"
#include "mediastream.h"

GVCamMediaSource::GVCamMediaSource() : _state(MFMEDIASOURCE_STOPPED) {}
GVCamMediaSource::~GVCamMediaSource() {}

HRESULT GVCamMediaSource::RuntimeClassInitialize()
{
    GVCamLog("GVCamMediaSource::RuntimeClassInitialize");

    HRESULT hr = MFCreateEventQueue(&_eventQueue);
    if (FAILED(hr)) return hr;

    hr = MFCreateAttributes(&_sourceAttributes, 8);
    if (FAILED(hr)) return hr;

    _sourceAttributes->SetUINT32(MF_SA_D3D_AWARE, FALSE);
    _sourceAttributes->SetUINT32(MF_SA_D3D11_AWARE, FALSE);

    _stream = Make<GVCamMediaStream>();
    if (!_stream) return E_OUTOFMEMORY;

    hr = _stream->RuntimeClassInitialize(this);
    if (FAILED(hr)) { GVCamLog("Stream init failed: 0x%08X", hr); return hr; }

    return S_OK;
}

STDMETHODIMP GVCamMediaSource::QueryInterface(REFIID riid, void** ppv)
{
    return RuntimeClass::QueryInterface(riid, ppv);
}

STDMETHODIMP GVCamMediaSource::GetService(REFGUID, REFIID, LPVOID*)
{
    return E_NOINTERFACE;
}

STDMETHODIMP GVCamMediaSource::GetSourceAttributes(IMFAttributes** ppAttributes)
{
    if (!ppAttributes) return E_POINTER;
    _sourceAttributes.CopyTo(ppAttributes);
    return S_OK;
}

STDMETHODIMP GVCamMediaSource::GetStreamAttributes(DWORD dwStreamIdentifier, IMFAttributes** ppAttributes)
{
    if (!ppAttributes) return E_POINTER;

    ComPtr<IMFAttributes> streamAttrs;
    HRESULT hr = MFCreateAttributes(&streamAttrs, 8);
    if (FAILED(hr)) return hr;

    static const GUID PINNAME_VIDEO_CAPTURE =
        { 0xFB6C4281, 0x0353, 0x11d1, { 0x90, 0x5F, 0x00, 0x00, 0xC0, 0xCC, 0x16, 0xBA } };

    streamAttrs->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, PINNAME_VIDEO_CAPTURE);
    streamAttrs->SetUINT32(MF_DEVICESTREAM_STREAM_ID, dwStreamIdentifier);
    streamAttrs->SetUINT32(MF_DEVICESTREAM_FRAMESERVER_SHARED, 1);
    streamAttrs->SetUINT32(MF_SA_D3D_AWARE, FALSE);
    streamAttrs->SetUINT32(MF_SA_D3D11_AWARE, FALSE);

    *ppAttributes = streamAttrs.Detach();
    return S_OK;
}

STDMETHODIMP GVCamMediaSource::SetD3DManager(IUnknown*) { return S_OK; }

STDMETHODIMP GVCamMediaSource::GetCharacteristics(DWORD* characteristics)
{
    if (!characteristics) return E_POINTER;
    *characteristics = MFMEDIASOURCE_IS_LIVE;
    return S_OK;
}

STDMETHODIMP GVCamMediaSource::CreatePresentationDescriptor(IMFPresentationDescriptor** ppPD)
{
    if (!ppPD) return E_POINTER;

    ComPtr<IMFStreamDescriptor> sd;
    HRESULT hr = _stream->GetStreamDescriptor(&sd);
    if (FAILED(hr)) return hr;

    IMFStreamDescriptor* sdArr[] = { sd.Get() };
    ComPtr<IMFPresentationDescriptor> pd;

    hr = MFCreatePresentationDescriptor(1, sdArr, &pd);
    if (FAILED(hr)) return hr;

    pd->SelectStream(0);
    *ppPD = pd.Detach();
    return S_OK;
}

STDMETHODIMP GVCamMediaSource::Start(IMFPresentationDescriptor*, const GUID*, const PROPVARIANT* pvarStartPosition)
{
    GVCamLog("GVCamMediaSource::Start");
    _state = MFMEDIASOURCE_RUNNING;
    _stream->SetActive(true);

    ComPtr<IUnknown> streamUnk;
    _stream.As(&streamUnk);

    HRESULT hr = _eventQueue->QueueEventParamUnk(MENewStream, GUID_NULL, S_OK, streamUnk.Get());
    if (FAILED(hr)) return hr;

    hr = _eventQueue->QueueEventParamVar(MESourceStarted, GUID_NULL, S_OK, pvarStartPosition);
    if (FAILED(hr)) return hr;

    return _stream->FireStreamStarted(pvarStartPosition);
}

STDMETHODIMP GVCamMediaSource::Stop()
{
    GVCamLog("GVCamMediaSource::Stop");
    _state = MFMEDIASOURCE_STOPPED;
    _stream->SetActive(false);
    _eventQueue->QueueEventParamVar(MESourceStopped, GUID_NULL, S_OK, nullptr);
    return S_OK;
}

STDMETHODIMP GVCamMediaSource::Pause()
{
    _state = MFMEDIASOURCE_PAUSED;
    _stream->SetActive(false);
    _eventQueue->QueueEventParamVar(MESourcePaused, GUID_NULL, S_OK, nullptr);
    return S_OK;
}

STDMETHODIMP GVCamMediaSource::Shutdown()
{
    GVCamLog("GVCamMediaSource::Shutdown");
    if (_eventQueue) _eventQueue->Shutdown();
    if (_stream)     _stream->Shutdown();
    return S_OK;
}

STDMETHODIMP GVCamMediaSource::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
{
    return _eventQueue->GetEvent(dwFlags, ppEvent);
}
STDMETHODIMP GVCamMediaSource::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
    return _eventQueue->BeginGetEvent(pCallback, punkState);
}
STDMETHODIMP GVCamMediaSource::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
    return _eventQueue->EndGetEvent(pResult, ppEvent);
}
STDMETHODIMP GVCamMediaSource::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
    return _eventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);
}
