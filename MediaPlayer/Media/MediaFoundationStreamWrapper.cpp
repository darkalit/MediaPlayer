#include "pch.h"
#include "MediaFoundationStreamWrapper.h"

#include "mfapi.h"
#include "Mferror.h"
#include "MediaFoundationSourceWrapper.h"

MediaFoundationStreamWrapper::MediaFoundationStreamWrapper(
    unsigned int sourceStreamId,
    unsigned int mediaStreamId,
    GUID streamType,
    IMFMediaSource* parentSource,
    IMFSourceReader* sourceReader)
    : m_SourceStreamId(sourceStreamId)
    , m_MediaStreamId(mediaStreamId)
    , m_StreamType(streamType)
{
    m_ParentSource.copy_from(parentSource);
    m_SourceReader.copy_from(sourceReader);
    winrt::check_hresult(GenerateStreamDescriptor());
    winrt::check_hresult(MFCreateEventQueue(m_MediaEventQueue.put()));
}

HRESULT MediaFoundationStreamWrapper::QueueStartedEvent(const PROPVARIANT* startPosition)
{
    m_State = State::STARTED;
    winrt::check_hresult(m_MediaEventQueue->QueueEventParamVar(MEStreamStarted, GUID_NULL, S_OK, startPosition));
    return S_OK;
}

HRESULT MediaFoundationStreamWrapper::QueueSeekedEvent(const PROPVARIANT* startPosition)
{
    m_State = State::STARTED;
    winrt::check_hresult(m_MediaEventQueue->QueueEventParamVar(MEStreamSeeked, GUID_NULL, S_OK, startPosition));
    return S_OK;
}

HRESULT MediaFoundationStreamWrapper::QueueStoppedEvent()
{
    m_State = State::STOPPED;
    winrt::check_hresult(m_MediaEventQueue->QueueEventParamVar(MEStreamStopped, GUID_NULL, S_OK, nullptr));
    return S_OK;
}

HRESULT MediaFoundationStreamWrapper::QueuePausedEvent()
{
    m_State = State::PAUSED;
    winrt::check_hresult(m_MediaEventQueue->QueueEventParamVar(MEStreamPaused, GUID_NULL, S_OK, nullptr));
    return S_OK;
}

void MediaFoundationStreamWrapper::SetSelected(bool selected)
{
    m_Selected = selected;
}

bool MediaFoundationStreamWrapper::IsSelected()
{
    return m_Selected;
}

bool MediaFoundationStreamWrapper::HasEnded()
{
    return m_StreamEnded;
}

STDMETHODIMP MediaFoundationStreamWrapper::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
{
    if (!m_MediaEventQueue) return E_FAIL;

    return m_MediaEventQueue->GetEvent(dwFlags, ppEvent);
}

STDMETHODIMP MediaFoundationStreamWrapper::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
    if (!m_MediaEventQueue) return E_FAIL;

    winrt::check_hresult(m_MediaEventQueue->BeginGetEvent(pCallback, punkState));
    return S_OK;
}

STDMETHODIMP MediaFoundationStreamWrapper::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
    if (!m_MediaEventQueue) return E_FAIL;

    winrt::check_hresult(m_MediaEventQueue->EndGetEvent(pResult, ppEvent));
    return S_OK;
}

STDMETHODIMP MediaFoundationStreamWrapper::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
    if (!m_MediaEventQueue) return E_FAIL;

    winrt::check_hresult(m_MediaEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue));
    return S_OK;
}

STDMETHODIMP MediaFoundationStreamWrapper::GetMediaSource(IMFMediaSource** ppMediaSource)
{
    if (!m_ParentSource) return MF_E_SHUTDOWN;
    m_ParentSource.copy_to(ppMediaSource);
    return S_OK;
}

STDMETHODIMP MediaFoundationStreamWrapper::GetStreamDescriptor(IMFStreamDescriptor** ppStreamDescriptor)
{
    if (!m_StreamDescriptor) return MF_E_NOT_INITIALIZED;
    m_StreamDescriptor.copy_to(ppStreamDescriptor);
    return S_OK;
}

STDMETHODIMP MediaFoundationStreamWrapper::RequestSample(IUnknown* pToken)
{
    winrt::com_ptr<IUnknown> token;
    token.copy_from(pToken);
    m_PendingSampleRequestTokens.push(token);

    winrt::com_ptr<IMFSample> sample;
    DWORD flags;
    LONGLONG timestamp;
    DWORD actualStreamIndex;

    winrt::check_hresult(m_SourceReader->ReadSample(m_SourceStreamId, 0, &actualStreamIndex, &flags, &timestamp, sample.put()));
    if (actualStreamIndex != m_SourceStreamId) return E_FAIL;

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
        winrt::check_hresult(m_MediaEventQueue->QueueEventParamUnk(MEEndOfStream, GUID_NULL, S_OK, nullptr));
        m_StreamEnded = true;
        static_cast<MediaFoundationSourceWrapper*>(m_ParentSource.get())->CheckForEndOfPresentation();
    }
    else
    {
        winrt::com_ptr<IUnknown> requestToken = m_PendingSampleRequestTokens.front();
        if (requestToken)
        {
            winrt::check_hresult(sample->SetUnknown(MFSampleExtension_Token, requestToken.get()));
        }

        winrt::check_hresult(m_MediaEventQueue->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, sample.get()));
        m_PendingSampleRequestTokens.pop();
    }

    return S_OK;
}

GUID MediaFoundationStreamWrapper::StreamType() const
{
    return m_StreamType;
}

HRESULT MediaFoundationStreamWrapper::GenerateStreamDescriptor()
{
    winrt::com_ptr<IMFMediaType> mediaType;
    IMFMediaType** mediaTypes = mediaType.put();

    winrt::check_hresult(GetMediaType(mediaType.put()));
    winrt::check_hresult(MFCreateStreamDescriptor(m_MediaStreamId, 1, mediaTypes, m_StreamDescriptor.put()));
    return S_OK;
}

HRESULT MediaFoundationStreamWrapper::GetMediaType(IMFMediaType** mediaTypeOut)
{
    winrt::check_hresult(m_SourceReader->GetCurrentMediaType(m_SourceStreamId, mediaTypeOut));
    return S_OK;
}
