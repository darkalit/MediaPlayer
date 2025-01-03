#pragma once

#include "mfmediaengine.h"
#include "functional"

class MediaEngineNotifyImpl : public winrt::implements<MediaEngineNotifyImpl, IMFMediaEngineNotify>
{
public:
    MediaEngineNotifyImpl(
        std::function<void()> onLoadedCB,
        std::function<void()> onPlaybackEndedCB,
        std::function<void(MF_MEDIA_ENGINE_ERR, HRESULT)> onErrorCB);
    ~MediaEngineNotifyImpl() = default;

    STDMETHODIMP EventNotify(DWORD event, DWORD_PTR param1, DWORD param2) override;
    void Shutdown();
    
private:
    std::function<void()> m_OnLoadedCB;
    std::function<void()> m_OnPlaybackEndedCB;
    std::function<void(MF_MEDIA_ENGINE_ERR, HRESULT)> m_OnErrorCB;
    bool m_Detached = false;
};

