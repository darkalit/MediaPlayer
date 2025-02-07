#include "pch.h"
#include "MediaEngineWrapper.h"

#include <iostream>
#include "mfapi.h"
#include "Audioclient.h"

MediaEngineWrapper::MediaEngineWrapper(
    IMFDXGIDeviceManager* dxgiDeviceManager,
    std::function<void()> onLoadedCB,
    std::function<void()> onPlaybackEndedCB,
    std::function<void(MF_MEDIA_ENGINE_ERR, HRESULT)> onErrorCB,
    unsigned int width,
    unsigned int height)
    : m_OnLoadedCB(onLoadedCB)
    , m_OnPlaybackEndedCB(onPlaybackEndedCB)
    , m_OnErrorCB(onErrorCB)
    , m_Width(width)
    , m_Height(height)
{
    m_DeviceManager.copy_from(dxgiDeviceManager);
    winrt::check_hresult(CreateMediaEngine());
}

MediaEngineWrapper::~MediaEngineWrapper()
{
    m_MediaEngineNotifier->Shutdown();
    m_MediaEngineExtension->Shutdown();
    m_MediaEngine->Shutdown();
}

void MediaEngineWrapper::SetSource(IMFSourceReader* sourceReader)
{
    m_Source = winrt::make_self<MediaFoundationSourceWrapper>(sourceReader);
    m_SourceReader.copy_from(sourceReader);

    winrt::com_ptr<IUnknown> sourceUnknown;
    m_Source.as(sourceUnknown);
    m_MediaEngineExtension->SetMediaSource(sourceUnknown.get());

    if (!m_HasSetSource)
    {
        BSTR source = SysAllocString(L"MFRendererSrc");
        m_MediaEngine->SetSource(source);
        SysFreeString(source);
        m_HasSetSource = true;
    }
    else
    {
        winrt::check_hresult(m_MediaEngine->Load());
    }
}

void MediaEngineWrapper::TransferVideoFrame(IUnknown* pDstSurf, const MFVideoNormalizedRect* pSrc, const RECT* pDst, const MFARGB* pBorderClr)
{
    winrt::check_hresult(m_MediaEngine->TransferVideoFrame(pDstSurf, pSrc, pDst, pBorderClr));
}

void MediaEngineWrapper::Start(double timeStamp)
{
    SetCurrentTime(timeStamp);
    winrt::check_hresult(m_MediaEngine->Play());
}

void MediaEngineWrapper::Stop()
{
    SetCurrentTime(0.0);
    Pause();
}

void MediaEngineWrapper::Pause()
{
    winrt::check_hresult(m_MediaEngine->Pause());
}

void MediaEngineWrapper::SetPlaybackSpeed(double speed)
{
    winrt::check_hresult(m_MediaEngine->SetPlaybackRate(speed));
}

void MediaEngineWrapper::SetVolume(double volume)
{
    volume = max(0.0, min(1.0, volume));
    winrt::check_hresult(m_MediaEngine->SetVolume(volume));
}

double MediaEngineWrapper::GetVolume()
{
    return m_MediaEngine->GetVolume();
}

void MediaEngineWrapper::SetCurrentTime(double timeStamp)
{
    PROPVARIANT startTime = {};
    startTime.vt = VT_I8;
    startTime.hVal.QuadPart = static_cast<LONGLONG>(timeStamp) * 10000000;
    winrt::check_hresult(m_SourceReader->SetCurrentPosition(GUID_NULL, startTime));
    winrt::check_hresult(m_MediaEngine->SetCurrentTime(timeStamp));
}

double MediaEngineWrapper::GetCurrentTime()
{
    return m_MediaEngine->GetCurrentTime();
}

void MediaEngineWrapper::WindowUpdate(unsigned int width, unsigned int height)
{
    if (width != m_Width || height != m_Height)
    {
        m_Width = width;
        m_Height = height;
    }

    if (m_MediaEngine)
    {
        RECT destRect{ 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
        winrt::com_ptr<IMFMediaEngineEx> mediaEngineEx;
        m_MediaEngine.as(mediaEngineEx);
        winrt::check_hresult(mediaEngineEx->UpdateVideoStream(nullptr, &destRect, nullptr));
    }
}

HANDLE MediaEngineWrapper::GetSurfaceHandle()
{
    HANDLE surfaceHandle = INVALID_HANDLE_VALUE;
    surfaceHandle = m_DcompSurfaceHandle.get();
    return surfaceHandle;
}

HRESULT MediaEngineWrapper::CreateMediaEngine()
{
    winrt::com_ptr<IMFMediaEngineClassFactory> classFactory;
    winrt::com_ptr<IMFAttributes> attributes;

    m_MediaEngineExtension = winrt::make_self<MediaEngineExtension>();
    winrt::check_hresult(MFCreateAttributes(attributes.put(), 6));

    auto onLoadedCB = std::bind(&MediaEngineWrapper::OnLoaded, this);
    auto onPlaybackEndedCB = std::bind(&MediaEngineWrapper::OnPlaybackEnded, this);
    auto onErrorCB = std::bind(&MediaEngineWrapper::OnError, this, std::placeholders::_1, std::placeholders::_2);

    m_MediaEngineNotifier = winrt::make_self<MediaEngineNotifyImpl>(onLoadedCB, onPlaybackEndedCB, onErrorCB);

    winrt::check_hresult(attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, m_MediaEngineNotifier.get()));
    winrt::check_hresult(attributes->SetUINT32(MF_MEDIA_ENGINE_CONTENT_PROTECTION_FLAGS, MF_MEDIA_ENGINE_ENABLE_PROTECTED_CONTENT));
    winrt::check_hresult(attributes->SetUINT32(MF_MEDIA_ENGINE_AUDIO_CATEGORY, AudioCategory_Media));

    if (m_DeviceManager)
    {
        winrt::check_hresult(attributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, m_DeviceManager.get()));
    }

    winrt::check_hresult(attributes->SetUnknown(MF_MEDIA_ENGINE_EXTENSION, m_MediaEngineExtension.get()));

    winrt::check_hresult(CoCreateInstance(
        CLSID_MFMediaEngineClassFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(classFactory.put())));
    winrt::check_hresult(classFactory->CreateInstance(0, attributes.get(), m_MediaEngine.put()));

    return S_OK;
}

void MediaEngineWrapper::OnLoaded()
{
    winrt::com_ptr<IMFMediaEngineEx> mediaEngineEx;
    m_MediaEngine.as(mediaEngineEx);
    winrt::check_hresult(mediaEngineEx->EnableWindowlessSwapchainMode(true));

    unsigned int width = m_Width != 0 ? m_Width : 640;
    unsigned int height = m_Height != 0 ? m_Height : 480;
    WindowUpdate(width, height);
    winrt::check_hresult(mediaEngineEx->GetVideoSwapchainHandle(m_DcompSurfaceHandle.put()));

    if (m_OnLoadedCB)
    {
        m_OnLoadedCB();
    }
}

void MediaEngineWrapper::OnPlaybackEnded()
{
    if (m_OnPlaybackEndedCB)
    {
        m_OnPlaybackEndedCB();
    }
}

void MediaEngineWrapper::OnError(MF_MEDIA_ENGINE_ERR error, HRESULT hr)
{
    if (m_OnErrorCB)
    {
        m_OnErrorCB(error, hr);
    }
}
