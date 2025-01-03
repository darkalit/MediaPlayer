#pragma once

#include "mfidl.h"
#include "mfreadwrite.h"
#include "queue"

class MediaFoundationStreamWrapper : public winrt::implements<MediaFoundationStreamWrapper, IMFMediaStream>
    {
public:
    MediaFoundationStreamWrapper(
        unsigned int sourceStreamId,
        unsigned int mediaStreamId,
        GUID streamType,
        IMFMediaSource* parentSource,
        IMFSourceReader* sourceReader);
    ~MediaFoundationStreamWrapper() = default;

    HRESULT QueueStartedEvent(const PROPVARIANT* startPosition);
    HRESULT QueueSeekedEvent(const PROPVARIANT* startPosition);
    HRESULT QueueStoppedEvent();
    HRESULT QueuePausedEvent();
    void SetSelected(bool selected);
    bool IsSelected();
    bool HasEnded();

    STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent) override;
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState) override;
    STDMETHODIMP EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent) override;
    STDMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue) override;

    STDMETHODIMP GetMediaSource(IMFMediaSource** ppMediaSource) override;
    STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor** ppStreamDescriptor) override;
    STDMETHODIMP RequestSample(IUnknown* pToken) override;

    GUID StreamType() const;

protected:
    enum class State
    {
        INITIALIZED,
        STARTED,
        STOPPED,
        PAUSED,
    };

    HRESULT GenerateStreamDescriptor();
    HRESULT GetMediaType(IMFMediaType** mediaTypeOut);

    State m_State = State::INITIALIZED;
    unsigned int m_SourceStreamId;
    unsigned int m_MediaStreamId;
    GUID m_StreamType;
    winrt::com_ptr<IMFMediaSource> m_ParentSource;
    winrt::com_ptr<IMFSourceReader> m_SourceReader;
    winrt::com_ptr<IMFMediaEventQueue> m_MediaEventQueue;
    winrt::com_ptr<IMFStreamDescriptor> m_StreamDescriptor;
    std::queue<winrt::com_ptr<IUnknown>> m_PendingSampleRequestTokens;
    bool m_Selected = false;
    bool m_StreamEnded = false;
};

