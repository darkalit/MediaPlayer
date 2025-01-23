#include "pch.h"
#include "MediaFoundationSourceWrapper.h"

#include "mfapi.h"
#include "Mferror.h"

MediaFoundationSourceWrapper::MediaFoundationSourceWrapper(IMFSourceReader* sourceReader)
{
    //m_SourceReader.attach(sourceReader);
    BOOL validStream = false;
    DWORD streamIndexIn = 0;
    DWORD mediaStreamIndex = 0;
    while (true)
    {
        validStream = false;
        HRESULT hr = sourceReader->GetStreamSelection(streamIndexIn, &validStream);
        if (hr == MF_E_INVALIDSTREAMNUMBER)
        {
            hr = S_OK;
            break;
        }

        if (validStream)
        {
            winrt::com_ptr<IMFMediaType> mediaType;
            winrt::check_hresult(sourceReader->GetCurrentMediaType(streamIndexIn, mediaType.put()));
            GUID guidMajorType;
            winrt::check_hresult(mediaType->GetMajorType(&guidMajorType));
            if (guidMajorType == MFMediaType_Audio || guidMajorType == MFMediaType_Video)
            {
                auto stream = winrt::make_self<MediaFoundationStreamWrapper>(streamIndexIn, mediaStreamIndex, guidMajorType, this, sourceReader);
                m_MediaStreams.push_back(stream);
                mediaStreamIndex++;
            }
        }
        streamIndexIn++;
    }
    winrt::check_hresult(MFCreateEventQueue(m_MediaEventQueue.put()));
}

STDMETHODIMP MediaFoundationSourceWrapper::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
{
    return m_MediaEventQueue->GetEvent(dwFlags, ppEvent);
}

STDMETHODIMP MediaFoundationSourceWrapper::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
    winrt::check_hresult(m_MediaEventQueue->BeginGetEvent(pCallback, punkState));
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
    winrt::check_hresult(m_MediaEventQueue->EndGetEvent(pResult, ppEvent));
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT* pvValue)
{
    winrt::check_hresult(m_MediaEventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue));
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::GetCharacteristics(DWORD* pdwCharacteristics)
{
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    *pdwCharacteristics = MFMEDIASOURCE_CAN_SEEK;
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::CreatePresentationDescriptor(IMFPresentationDescriptor** ppPresentationDescriptor)
{
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    winrt::com_ptr<IMFPresentationDescriptor> presentationDescriptor;
    std::vector<winrt::com_ptr<IMFStreamDescriptor>> streamDescriptors;
    for (auto& stream : m_MediaStreams)
    {
        winrt::com_ptr<IMFStreamDescriptor> streamDescriptor;
        winrt::check_hresult(stream->GetStreamDescriptor(streamDescriptor.put()));
        streamDescriptors.push_back(streamDescriptor);
    }

    const DWORD streamDescCount = static_cast<DWORD>(streamDescriptors.size());
    winrt::check_hresult(MFCreatePresentationDescriptor(streamDescCount, reinterpret_cast<IMFStreamDescriptor**>(streamDescriptors.data()), presentationDescriptor.put()));
    winrt::check_hresult(SelectDefaultStreams(streamDescCount, presentationDescriptor.get()));
    *ppPresentationDescriptor = presentationDescriptor.detach();
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::Start(IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition)
{
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    bool seeked = false;
    bool started = false;
    if (m_State == State::INITIALIZED || m_State == State::STOPPED)
    {
        started = true;
    }
    else if (m_State == State::PAUSED || m_State == State::STARTED)
    {
        if (pvarStartPosition->vt == VT_EMPTY)
        {
            started = true;
        }
        else
        {
            seeked = true;
        }
    }

    DWORD streamDescCount = 0;
    winrt::check_hresult(pPresentationDescriptor->GetStreamDescriptorCount(&streamDescCount));
    for (DWORD i = 0; i < streamDescCount; ++i)
    {
        winrt::com_ptr<IMFStreamDescriptor> streamDescriptor;
        BOOL selected;
        winrt::check_hresult(pPresentationDescriptor->GetStreamDescriptorByIndex(i, &selected, streamDescriptor.put()));

        DWORD streamId;
        winrt::check_hresult(streamDescriptor->GetStreamIdentifier(&streamId));

        if (streamId >= m_MediaStreams.size()) continue;

        winrt::com_ptr<MediaFoundationStreamWrapper> stream = m_MediaStreams[streamId];

        if (selected)
        {
            MediaEventType eventType = MENewStream;
            if (stream->IsSelected())
            {
                eventType = MEUpdatedStream;
            }

            winrt::com_ptr<IUnknown> unknownStream;
            stream.as(unknownStream);
            winrt::check_hresult(m_MediaEventQueue->QueueEventParamUnk(eventType, GUID_NULL, S_OK, unknownStream.get()));

            if (started)
            {
                if (seeked) return E_FAIL;

                winrt::check_hresult(stream->QueueStartedEvent(pvarStartPosition));
            }
            else if (seeked)
            {
                winrt::check_hresult(stream->QueueSeekedEvent(pvarStartPosition));
            }
        }
        stream->SetSelected(selected);
        m_State = State::STARTED;
    }

    if (started)
    {
        if (seeked) return E_FAIL;

        winrt::com_ptr<IMFMediaEvent> mediaEvent;
        winrt::check_hresult(MFCreateMediaEvent(MESourceStarted, GUID_NULL, S_OK, pvarStartPosition, mediaEvent.put()));
        winrt::check_hresult(m_MediaEventQueue->QueueEvent(mediaEvent.get()));
    }
    else if (seeked)
    {
        winrt::check_hresult(QueueEvent(MESourceSeeked, GUID_NULL, S_OK, pvarStartPosition));
    }
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::Stop(void)
{
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    winrt::check_hresult(QueueEvent(MESourceStopped, GUID_NULL, S_OK, nullptr));

    for (auto& stream : m_MediaStreams)
    {
        if (stream->IsSelected())
        {
            winrt::check_hresult(stream->QueueStoppedEvent());
        }
    }

    m_State = State::STOPPED;
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::Pause(void)
{
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    winrt::check_hresult(QueueEvent(MESourcePaused, GUID_NULL, S_OK, nullptr));

    for (auto& stream : m_MediaStreams)
    {
        if (stream->IsSelected())
        {
            winrt::check_hresult(stream->QueuePausedEvent());
        }
    }

    m_State = State::PAUSED;
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::Shutdown(void)
{
    m_State = State::SHUTDOWN;
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::GetService(REFGUID guidService, REFIID riid, LPVOID* ppvObject)
{
    if (!IsEqualGUID(guidService, MF_RATE_CONTROL_SERVICE)) return MF_E_UNSUPPORTED_SERVICE;

    return static_cast<IMFMediaSource*>(this)->QueryInterface(riid, ppvObject);
}

STDMETHODIMP MediaFoundationSourceWrapper::GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float* pflRate)
{
    if (eDirection == MFRATE_REVERSE) return MF_E_REVERSE_UNSUPPORTED;

    *pflRate = 0.0f;
    return m_State == State::SHUTDOWN ? MF_E_SHUTDOWN : S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::GetFastestRate(MFRATE_DIRECTION eDirection, BOOL fThin, float* pflRate)
{
    if (eDirection == MFRATE_REVERSE) return MF_E_REVERSE_UNSUPPORTED;

    *pflRate = 0.0f;
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    *pflRate = 3.0f;
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::IsRateSupported(BOOL fThin, float flRate, float* pflNearestSupportedRate)
{
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    if (pflNearestSupportedRate)
    {
        *pflNearestSupportedRate = 0.0f;
    }

    MFRATE_DIRECTION direction = (flRate >= 0) ? MFRATE_FORWARD : MFRATE_REVERSE;
    float fastestRate = 0.0f;
    float slowestRate = 0.0f;
    winrt::check_hresult(GetFastestRate(direction, fThin, &fastestRate));
    winrt::check_hresult(GetSlowestRate(direction, fThin, &slowestRate));

    if (fThin) return MF_E_THINNING_UNSUPPORTED;
    if (flRate < slowestRate) return MF_E_REVERSE_UNSUPPORTED;
    if (flRate > fastestRate) return MF_E_UNSUPPORTED_RATE;

    if (pflNearestSupportedRate)
    {
        *pflNearestSupportedRate = flRate;
    }
    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::SetRate(BOOL fThin, float flRate)
{
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    winrt::check_hresult(IsRateSupported(fThin, flRate, &m_CurrentRate));

    PROPVARIANT varRate;
    varRate.vt = VT_R4;
    varRate.fltVal = m_CurrentRate;
    winrt::check_hresult(QueueEvent(MESourceRateChanged, GUID_NULL, S_OK, &varRate));

    return S_OK;
}

STDMETHODIMP MediaFoundationSourceWrapper::GetRate(BOOL* pfThin, float* pflRate)
{
    *pfThin = false;
    *pflRate = 0.0f;
    if (m_State == State::SHUTDOWN) return MF_E_SHUTDOWN;

    *pflRate = m_CurrentRate;
    return S_OK;
}

void MediaFoundationSourceWrapper::CheckForEndOfPresentation()
{
    if (m_PresentationEnded) return;

    bool presentationEnded = true;
    for (auto& stream : m_MediaStreams)
    {
        if (!stream->HasEnded())
        {
            presentationEnded = false;
            break;
        }
    }

    m_PresentationEnded = presentationEnded;
    if (m_PresentationEnded)
    {
        winrt::check_hresult(QueueEvent(MEEndOfPresentation, GUID_NULL, S_OK, nullptr));
    }
}

HRESULT MediaFoundationSourceWrapper::SelectDefaultStreams(const DWORD streamDescCount, IMFPresentationDescriptor* presentationDescriptor)
{
    bool audioStreamSelected = false;
    bool videoStreamSelected = false;

    for (DWORD idx = 0; idx < streamDescCount; ++idx)
    {
        winrt::com_ptr<IMFStreamDescriptor> streamDescriptor;
        BOOL selected;
        winrt::check_hresult(presentationDescriptor->GetStreamDescriptorByIndex(idx, &selected, streamDescriptor.put()));
        if (selected) continue;

        DWORD streamId;
        winrt::check_hresult(streamDescriptor->GetStreamIdentifier(&streamId));
        if (IsEqualGUID(m_MediaStreams[streamId]->StreamType(), MFMediaType_Audio) && !audioStreamSelected)
        {
            audioStreamSelected = true;
            winrt::check_hresult(presentationDescriptor->SelectStream(idx));
        }
        if (IsEqualGUID(m_MediaStreams[streamId]->StreamType(), MFMediaType_Video) && !videoStreamSelected)
        {
            videoStreamSelected = true;
            winrt::check_hresult(presentationDescriptor->SelectStream(idx));
        }
    }

    return S_OK;
}
