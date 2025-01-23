#pragma once

#include "MediaFoundationStreamWrapper.h"

class MediaFoundationSourceWrapper
    : public winrt::implements<MediaFoundationSourceWrapper,
    IMFMediaSource,
    IMFGetService,
    IMFRateSupport,
    IMFRateControl>
{
public:
    MediaFoundationSourceWrapper(IMFSourceReader* sourceReader);
    ~MediaFoundationSourceWrapper() = default;

    STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent) override;
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState) override;
    STDMETHODIMP EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent) override;
    STDMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue) override;

    STDMETHODIMP GetCharacteristics(DWORD* pdwCharacteristics) override;
    STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor** ppPresentationDescriptor) override;
    STDMETHODIMP Start(IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition) override;
    STDMETHODIMP Stop(void) override;
    STDMETHODIMP Pause(void) override;
    STDMETHODIMP Shutdown(void) override;

    STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID* ppvObject) override;

    STDMETHODIMP GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float* pflRate) override;
    STDMETHODIMP GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float* pflRate) override;
    STDMETHODIMP IsRateSupported(BOOL fThin, float flRate, float* pflNearestSupportedRate) override;

    STDMETHODIMP SetRate(BOOL fThin, float flRate) override;
    STDMETHODIMP GetRate(BOOL* pfThin, float* pflRate) override;

    void CheckForEndOfPresentation();

private:
    enum class State
    {
        INITIALIZED,
        STARTED,
        STOPPED,
        PAUSED,
        SHUTDOWN,
    };

    HRESULT SelectDefaultStreams(const DWORD streamDescCount, IMFPresentationDescriptor* presentationDescriptor);

    std::vector<winrt::com_ptr<MediaFoundationStreamWrapper>> m_MediaStreams;
    winrt::com_ptr<IMFMediaEventQueue> m_MediaEventQueue;
    //winrt::com_ptr<IMFSourceReader> m_SourceReader;
    State m_State = State::INITIALIZED;
    float m_CurrentRate = 0.0f;
    bool m_PresentationEnded = false;
};

