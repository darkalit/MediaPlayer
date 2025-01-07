#pragma once

#include "MediaFoundationSourceWrapper.h"
#include "MediaEngineExtension.h"
#include "MediaEngineNotifyImpl.h"

class MediaEngineWrapper : public winrt::implements<MediaEngineWrapper, IUnknown>
{
public:
    MediaEngineWrapper(
        IMFDXGIDeviceManager* dxgiDeviceManager,
        std::function<void()> onLoadedCB,
        std::function<void()> onPlaybackEndedCB,
        std::function<void(MF_MEDIA_ENGINE_ERR, HRESULT)> onErrorCB,
        unsigned int width,
        unsigned int height);
    ~MediaEngineWrapper();

    void SetSource(IMFSourceReader* sourceReader);

    // Start from time stamp in seconds
    void Start(double timeStamp);
    void Stop();
    void Pause();
    void SetPlaybackSpeed(double speed);

    // Set time in seconds
    void SetCurrentTime(double timeStamp);
    double GetCurrentTime();

    void WindowUpdate(unsigned int width, unsigned int height);
    HANDLE GetSurfaceHandle();

private:
    HRESULT CreateMediaEngine();
    void OnLoaded();
    void OnPlaybackEnded();
    void OnError(MF_MEDIA_ENGINE_ERR error, HRESULT hr);

    std::function<void()> m_OnLoadedCB;
    std::function<void()> m_OnPlaybackEndedCB;
    std::function<void(MF_MEDIA_ENGINE_ERR, HRESULT)> m_OnErrorCB;

    winrt::com_ptr<IMFSourceReader> m_SourceReader;
    winrt::com_ptr<IMFMediaEngine> m_MediaEngine;
    winrt::com_ptr<IMFDXGIDeviceManager> m_DeviceManager;
    winrt::com_ptr<MediaEngineExtension> m_MediaEngineExtension;
    winrt::com_ptr<MediaFoundationSourceWrapper> m_Source;
    winrt::com_ptr<MediaEngineNotifyImpl> m_MediaEngineNotifier;
    winrt::handle m_DcompSurfaceHandle;

    unsigned int m_Width = 0;
    unsigned int m_Height = 0;
    bool m_HasSetSource = false;
};

