#include "pch.h"
#include "PlayerService.h"

#include "PlayerService.g.cpp"

#include "mfapi.h"
#include "mfplay.h"
#include "mfobjects.h"
#include "mfidl.h"
#include "propvarutil.h"
#include "propkeydef.h"
#include "propkey.h"
#include "Shlwapi.h"
#include "wincodec.h"

#include "string"
#include "iostream"

#include "Audioclient.h"
#include "microsoft.ui.xaml.media.dxinterop.h"
#include "windows.system.threading.core.h"
#include "windows.system.threading.h"
#include "d3d11.h"
#include "dxgi1_3.h"
#include "d3d11_3.h"
#include "DirectXColors.h"
#include "App.xaml.h"
#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.Storage.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

using namespace winrt::Windows::Foundation;

namespace winrt::MediaPlayer::implementation
{
    PlayerService::PlayerService()
    {
        try
        {
            check_hresult(MFStartup(MF_VERSION));
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::PlayerService: Failed to init Media Foundation;\n" << e.message() << '\n';
        }
    }

    PlayerService::~PlayerService()
    {
        try
        {
            m_State = PlayerServiceState::CLOSED;
            m_VideoThread.join();

            check_hresult(MFShutdown());
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::~PlayerService: Failed to shutdown Media Foundation;\n" << e.message() << '\n';
        }
    }

    void PlayerService::Init()
    {
        m_DeviceResources = App::GetDeviceResources();
        m_TexturePlaneRenderer = std::make_shared<TexturePlaneRenderer>(m_DeviceResources);
        m_TextRenderer = std::make_shared<TextRenderer>(m_DeviceResources);

        m_DeviceManager = nullptr;
        UINT resetToken = 0;
        check_hresult(MFLockDXGIDeviceManager(&resetToken, m_DeviceManager.put()));

        com_ptr<ID3D10Multithread> multithreadedDevice;
        m_DeviceResources->GetD3DDevice()->QueryInterface(IID_PPV_ARGS(multithreadedDevice.put()));
        multithreadedDevice->SetMultithreadProtected(true);

        check_hresult(m_DeviceManager->ResetDevice(m_DeviceResources->GetD3DDevice(), resetToken));

        auto onLoadedCB = std::bind(&PlayerService::OnLoaded, this);
        auto onPlaybackEndedCB = std::bind(&PlayerService::OnPlaybackEnded, this);
        auto onErrorCB = std::bind(&PlayerService::OnError, this, std::placeholders::_1, std::placeholders::_2);

        m_MediaEngineWrapper = make_self<MediaEngineWrapper>(
            m_DeviceManager.get(),
            onLoadedCB,
            onPlaybackEndedCB,
            onErrorCB,
            0,
            0);

        State(PlayerServiceState::READY);
    }

    void PlayerService::AddSource(hstring const& path, hstring const& displayName)
    {
        auto res = GetMetadataInternal(path);

        if (res.Title.empty())
        {
            res.Title = displayName;
        }

        m_Playlist.Append(res);

        if (CurrentMediaIndex() == -1)
        {
            CurrentMediaIndex(0);
        }

        if (!HasSource())
        {
            SetSource(path);
        }
    }

    void PlayerService::SetSource(hstring const& path)
    {
        State(PlayerServiceState::SOURCE_SWITCH);

        // memory leak temporarily fix
        // TODO: find better way to control m_SourceReader references
        while (m_SourceReader)
        {
            if (m_SourceReader->Release() <= 2)
            {
                break;
            }
        }

        m_IsMFSupported = false;
        if (m_Mode == PlayerServiceMode::AUTO || m_Mode == PlayerServiceMode::MEDIA_FOUNDATION)
        {
            auto hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, m_SourceReader.put());
            m_IsMFSupported = SUCCEEDED(hr);
        }

        if (m_IsMFSupported && (m_Mode == PlayerServiceMode::AUTO || m_Mode == PlayerServiceMode::MEDIA_FOUNDATION))
        {
            m_MediaEngineWrapper->SetSource(m_SourceReader.get());

            if (SwapChainPanel())
            {
                m_UIDispatcherQueue.TryEnqueue([&]()
                {
                    auto size = SwapChainPanel().ActualSize();
                    ResizeVideo(size.x, size.y);
                });
            }

            Position(0);
            State(PlayerServiceState::STOPPED);

            return;
        }

        if (!(!m_IsMFSupported || m_Mode != PlayerServiceMode::FFMPEG))
        {
            return;
        }

        m_FfmpegDecoder.OpenFile(path);
        auto& buffer = m_FfmpegDecoder.GetWavBuffer();

        com_ptr<IStream> stream;
        stream.attach(SHCreateMemStream(buffer.data(), buffer.size()));

        com_ptr<IMFByteStream> byteStream;
        check_hresult(MFCreateMFByteStreamOnStreamEx(stream.get(), byteStream.put()));

        MFCreateSourceReaderFromByteStream(byteStream.get(), nullptr, m_SourceReader.put());
        m_MediaEngineWrapper->SetSource(m_SourceReader.get());


        m_SubTracks.Clear();
        for (auto& s : m_FfmpegDecoder.GetSubtitleStreams())
        {
            m_SubTracks.Append(s);
        }

        Position(0);
        State(PlayerServiceState::STOPPED);

        if (SwapChainPanel())
        {
            auto size = SwapChainPanel().ActualSize();
            ResizeVideo(size.x, size.y);

            if (m_VideoThread.joinable())
            {
                m_VideoThread.join();
            }

            m_VideoThread = std::thread(&PlayerService::VideoRender, this);
        }
    }

    void PlayerService::SetSubtitleIndex(int32_t index)
    {
        m_FfmpegDecoder.OpenSubtitle(index);
    }

    bool PlayerService::HasSource()
    {
        return !!m_SourceReader && m_MediaEngineWrapper && m_Playlist.Size() > 0 && CurrentMediaIndex() > -1;
    }

    int32_t PlayerService::GetMediaIndexById(guid const& id)
    {
        for (int32_t i = 0; i < m_Playlist.Size(); ++i)
        {
            if (m_Playlist.GetAt(i).Id == id)
            {
                return i;
            }
        }

        return -1;
    }

    void PlayerService::CreateSnapshot()
    {
        uint32_t width = Metadata().VideoWidth, height = Metadata().VideoHeight, rowPitch = 0;

        auto wicFactory = create_instance<IWICImagingFactory>(CLSID_WICImagingFactory);

        com_ptr<IWICBitmapEncoder> encoder;
        check_hresult(wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.put()));

        com_ptr<IWICStream> stream;
        check_hresult(wicFactory->CreateStream(stream.put()));

        Windows::Storage::StorageFolder folder = Windows::ApplicationModel::Package::Current().InstalledLocation();

        hstring pos = to_hstring(round(static_cast<double>(Position()) / 10.0) / 100.0);
        hstring time = to_hstring(clock::now().time_since_epoch().count());
        hstring path = folder.Path() + L"\\snapshot_" + Metadata().Title + L"_" + pos + L"_" + time + L".png";

        check_hresult(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE));
        check_hresult(encoder->Initialize(stream.get(), WICBitmapEncoderNoCache));

        com_ptr<IWICBitmapFrameEncode> frame;
        com_ptr<IPropertyBag2> props;
        check_hresult(encoder->CreateNewFrame(frame.put(), props.put()));
        check_hresult(frame->Initialize(props.get()));
        check_hresult(frame->SetSize(width, height));

        WICPixelFormatGUID format = GUID_WICPixelFormat32bppRGBA;
        check_hresult(frame->SetPixelFormat(&format));

        if (m_IsMFSupported && (m_Mode == PlayerServiceMode::AUTO || m_Mode == PlayerServiceMode::MEDIA_FOUNDATION))
        {
            m_MediaEngineWrapper->SetCurrentTime(Position() / 1000.0);

            auto device = m_DeviceResources->GetD3DDevice();
            auto context = m_DeviceResources->GetD3DDeviceContext();

            com_ptr<ID3D11Texture2D> texture;
            D3D11_TEXTURE2D_DESC desc {
                .Width = width,
                .Height = height,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                .SampleDesc = {
                    .Count = 1,
                    .Quality = 0,
                },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
            };
            check_hresult(device->CreateTexture2D(&desc, nullptr, texture.put()));

            MFVideoNormalizedRect srcRect = { 0, 0, 1, 1 };
            RECT dstRect = { 0, 0, width, height };
            MFARGB bgColor = { 0, 0, 0, 0 };

            m_MediaEngineWrapper->TransferVideoFrame(texture.get(), &srcRect, &dstRect, &bgColor);

            com_ptr<ID3D11Texture2D> stagingTexture;

            desc.BindFlags = 0;
            desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.Usage = D3D11_USAGE_STAGING;

            check_hresult(device->CreateTexture2D(&desc, nullptr, stagingTexture.put()));

            context->CopyResource(stagingTexture.get(), texture.get());

            D3D11_MAPPED_SUBRESOURCE mapped;
            check_hresult(context->Map(stagingTexture.get(), 0, D3D11_MAP_READ, 0, &mapped));
            check_hresult(frame->WritePixels(
                height,
                mapped.RowPitch,
                height * mapped.RowPitch,
                static_cast<BYTE*>(mapped.pData)
            ));
            context->Unmap(stagingTexture.get(), 0);
        }
        else
        {
            auto buffer = m_CurFrame.Buffer;
            for (size_t i = 0; i < buffer.size(); i += 4)
            {
                std::swap(buffer[i], buffer[i + 2]);
            }

            check_hresult(frame->WritePixels(
                height,
                m_CurFrame.RowPitch,
                height * m_CurFrame.RowPitch,
                buffer.data()
            ));
        }

        check_hresult(frame->Commit());
        check_hresult(encoder->Commit());
    }

    void PlayerService::Next()
    {
        if (!HasSource()) return;

        Stop();
        CurrentMediaIndex(CurrentMediaIndex() + 1);
        CurrentMediaIndex(CurrentMediaIndex() % m_Playlist.Size());
        SetSource(Metadata().Path);
        Start();
    }

    void PlayerService::Prev()
    {
        if (!HasSource()) return;

        Stop();
        if (CurrentMediaIndex() == 0)
        {
            CurrentMediaIndex(m_Playlist.Size());
        }
        CurrentMediaIndex(CurrentMediaIndex() - 1);
        SetSource(Metadata().Path);
        Start();
    }

    void PlayerService::StartByIndex(int32_t index)
    {
        if (!HasSource()) return;

        CurrentMediaIndex(max(0, min(m_Playlist.Size() - 1, index)));
        SetSource(Metadata().Path);
        Start();
    }

    void PlayerService::DeleteByIndex(int32_t index)
    {
        if (!HasSource()) return;
        if (index >= m_Playlist.Size() || index < 0) return;

        m_Playlist.RemoveAt(index);

        if (m_Playlist.Size() == 0)
        {
            Stop();
            return;
        }

        if (index < CurrentMediaIndex())
        {
            CurrentMediaIndex(CurrentMediaIndex() - 1);
            return;
        }

        index %= m_Playlist.Size();
        int newCurrentMedia = CurrentMediaIndex() % m_Playlist.Size();

        if (newCurrentMedia == index || newCurrentMedia != CurrentMediaIndex())
        {
            CurrentMediaIndex(newCurrentMedia);
            Stop();
            SetSource(Metadata().Path);
            Start();
        }
    }

    void PlayerService::Clear()
    {
        if (!HasSource()) return;

        Stop();
        m_Playlist.Clear();
        CurrentMediaIndex(-1);
    }

    void PlayerService::Start()
    {
        if (!HasSource()) return;

        try
        {
            if (CurrentMediaIndex() > -1 && Position() >= Metadata().Duration)
            {
                Stop();
                return;
            }

            double position = static_cast<double>(Position()) / 1000.0;
            if (!m_IsMFSupported || !(m_Mode == PlayerServiceMode::AUTO || m_Mode == PlayerServiceMode::MEDIA_FOUNDATION))
            {
                m_FfmpegDecoder.Seek(Position());
            }
            m_Seeked = true;
            m_MediaEngineWrapper->Start(position);
            PlaybackSpeed(m_PlaybackSpeed);
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Start: Failed to start media;\n" << e.message() << '\n';
        }

        State(PlayerServiceState::PLAYING);

        auto md = Metadata();
        md.IsSelected = true;
        Metadata(md);
    }

    void PlayerService::Start(uint64_t timePos)
    {
        if (!HasSource()) return;

        try
        {
            Position(timePos);

            if (CurrentMediaIndex() > -1 && timePos >= Metadata().Duration)
            {
                Stop();
                return;
            }

            double position = static_cast<double>(timePos) / 1000.0;
            if (!m_IsMFSupported || !(m_Mode == PlayerServiceMode::AUTO || m_Mode == PlayerServiceMode::MEDIA_FOUNDATION))
            {
                m_FfmpegDecoder.Seek(timePos);
            }
            m_Seeked = true;
            m_MediaEngineWrapper->Start(position);
            PlaybackSpeed(m_PlaybackSpeed);
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Start: Failed to start media;\n" << e.message() << '\n';
        }

        State(PlayerServiceState::PLAYING);

        auto md = Metadata();
        md.IsSelected = true;
        Metadata(md);
    }

    void PlayerService::Stop()
    {
        State(PlayerServiceState::STOPPED);
        if (!HasSource()) return;

        try
        {
            m_Position = 0;
            m_MediaEngineWrapper->Stop();
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Stop: Failed to stop media;\n" << e.message() << '\n';
        }
    }

    void PlayerService::Pause()
    {
        State(PlayerServiceState::PAUSED);
        if (!HasSource()) return;

        try
        {
            m_MediaEngineWrapper->Pause();
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Pause: Failed to stop media;\n" << e.message() << '\n';
        }
    }

    void PlayerService::ResizeVideo(float width, float height)
    {
        if (!m_MediaEngineWrapper) return;
        if (m_IsMFSupported && (m_Mode == PlayerServiceMode::MEDIA_FOUNDATION || m_Mode == PlayerServiceMode::AUTO))
        {
            m_MediaEngineWrapper->WindowUpdate(width, height);
            return;
        }

        m_ResizeNeeded = true;
        m_DesiredSize = { width, height };
    }

    uint64_t PlayerService::Position()
    {
        m_Position = m_MediaEngineWrapper->GetCurrentTime() * 1000.0;
        return m_Position;
    }

    void PlayerService::Position(uint64_t value)
    {
        if (m_Position != value)
        {
            m_Position = value;
        }
    }

    uint64_t PlayerService::RemainingTime()
    {
        auto metadata = Metadata();
        return metadata.Duration ? metadata.Duration - m_Position : 0;
    }

    double PlayerService::PlaybackSpeed()
    {
        return m_PlaybackSpeed;
    }

    void PlayerService::PlaybackSpeed(double value)
    {
        if (m_PlaybackSpeed != value)
        {
            m_PlaybackSpeed = value;
            m_MediaEngineWrapper->SetPlaybackSpeed(value);
        }
    }

    int32_t PlayerService::CurrentMediaIndex()
    {
        return m_CurrentMediaIndex;
    }

    void PlayerService::CurrentMediaIndex(int32_t value)
    {
        if (m_CurrentMediaIndex != value)
        {
            m_CurrentMediaIndex = value;
        }
    }

    double PlayerService::Volume()
    {
        return m_MediaEngineWrapper->GetVolume();
    }

    void PlayerService::Volume(double value)
    {
        if (m_MediaEngineWrapper->GetVolume() != value)
        {
            m_MediaEngineWrapper->SetVolume(value);
        }        
    }

    PlayerServiceMode PlayerService::Mode()
    {
        return m_Mode;
    }

    void PlayerService::Mode(PlayerServiceMode const& value)
    {
        if (m_Mode != value)
        {
            m_Mode = value;
        }
    }

    PlayerServiceState PlayerService::State()
    {
        return m_State;
    }

    void PlayerService::State(PlayerServiceState value)
    {
        if (m_State != value)
        {
            m_State = value;
        }
    }

    MediaMetadata PlayerService::Metadata()
    {
        if (!HasSource()) return {};

        return m_Playlist.GetAt(CurrentMediaIndex());
    }

    void PlayerService::Metadata(MediaMetadata const& value)
    {
        if (m_Playlist.GetAt(CurrentMediaIndex()) != value)
        {
            m_Playlist.GetAt(CurrentMediaIndex()) = value;
        }
    }

    Collections::IVector<MediaMetadata> PlayerService::Playlist()
    {
        return m_Playlist;
    }

    Windows::Foundation::Collections::IObservableVector<SubtitleStream> PlayerService::SubTracks()
    {
        return m_SubTracks;
    }

    Microsoft::UI::Xaml::Controls::SwapChainPanel PlayerService::SwapChainPanel()
    {
        return m_SwapChainPanel;
    }

    void PlayerService::SwapChainPanel(Microsoft::UI::Xaml::Controls::SwapChainPanel const& value)
    {
        m_ChangingSwapchain = true;
        m_SwapChainPanel = value;

        if (m_IsMFSupported && (m_Mode == PlayerServiceMode::AUTO || m_Mode == PlayerServiceMode::MEDIA_FOUNDATION))
        {
            m_VideoSurfaceHandle = m_MediaEngineWrapper ? m_MediaEngineWrapper->GetSurfaceHandle() : nullptr;
        }

        m_UIDispatcherQueue.TryEnqueue([&]()
        {
            m_DeviceResources->SetSwapChainPanel(m_SwapChainPanel);

            com_ptr<ISwapChainPanelNative2> panelNative;
            m_SwapChainPanel.as(panelNative);

            if (m_IsMFSupported && (m_Mode == PlayerServiceMode::AUTO || m_Mode == PlayerServiceMode::MEDIA_FOUNDATION))
            {
                check_hresult(panelNative->SetSwapChainHandle(m_VideoSurfaceHandle));
                auto size = m_SwapChainPanel.ActualSize();
                ResizeVideo(size.x, size.y);
                m_ChangingSwapchain = false;
                return;
            }

            panelNative->SetSwapChain(m_DeviceResources->GetSwapChain());
            m_ChangingSwapchain = false;
        });
    }

    Microsoft::UI::Dispatching::DispatcherQueue PlayerService::UIDispatcher()
    {
        return m_UIDispatcherQueue;
    }

    void PlayerService::UIDispatcher(Microsoft::UI::Dispatching::DispatcherQueue const& value)
    {
        if (m_UIDispatcherQueue != value)
        {
            m_UIDispatcherQueue = value;
        }
    }

    MediaMetadata PlayerService::GetMetadataInternal(
        const hstring& path)
    {
        com_ptr<IPropertyStore> props;
        com_ptr<IMFSourceResolver> sourceResolver;
        com_ptr<::IUnknown> source;
        MediaMetadata metadata;

        metadata.Id = GuidHelper::CreateNewGuid();
        metadata.Path = path;
        metadata.IsSelected = false;
        metadata.AddedAt = clock::now();

        try
        {
            DWORD cProps;
            MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;

            check_hresult(MFCreateSourceResolver(sourceResolver.put()));
            check_hresult(sourceResolver->CreateObjectFromURL(
                path.c_str(),
                MF_RESOLUTION_MEDIASOURCE,
                nullptr,
                &objectType,
                source.put()
            ));

            check_hresult(MFGetService(source.get(), MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(props.put())));

            check_hresult(props->GetCount(&cProps));

            for (DWORD i = 0; i < cProps; i++)
            {
                PROPERTYKEY key;
                PROPVARIANT pv;

                check_hresult(props->GetAt(i, &key));
                check_hresult(props->GetValue(key, &pv));

                if (key == PKEY_Media_Duration)
                {
                    ULONGLONG duration;
                    check_hresult(PropVariantToUInt64(pv, &duration));
                    metadata.Duration = duration / 10000;
                }
                else if (key == PKEY_Audio_ChannelCount)
                {
                    metadata.AudioChannelCount = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioChannelCount));
                }
                else if (key == PKEY_Audio_EncodingBitrate)
                {
                    metadata.AudioBitrate = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioBitrate));
                }
                else if (key == PKEY_Audio_SampleRate)
                {
                    metadata.AudioSampleRate = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioSampleRate));
                }
                else if (key == PKEY_Audio_SampleSize)
                {
                    metadata.AudioSampleSize = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioSampleSize));
                }
                else if (key == PKEY_Audio_StreamNumber)
                {
                    metadata.AudioStreamId = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.AudioStreamId));
                }
                else if (key == PKEY_Video_EncodingBitrate)
                {
                    metadata.VideoBitrate = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoBitrate));
                }
                else if (key == PKEY_Video_FrameWidth)
                {
                    metadata.VideoWidth = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoWidth));
                }
                else if (key == PKEY_Video_FrameHeight)
                {
                    metadata.VideoHeight = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoHeight));
                }
                else if (key == PKEY_Video_FrameRate)
                {
                    metadata.VideoFrameRate = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoFrameRate));
                    metadata.VideoFrameRate /= 1000;
                }
                else if (key == PKEY_Video_StreamNumber)
                {
                    metadata.VideoStreamId = 0;
                    check_hresult(PropVariantToUInt32(pv, &metadata.VideoStreamId));
                }
                else if (key == PKEY_Author)
                {
                    wchar_t* buffer;
                    check_hresult(PropVariantToStringAlloc(pv, &buffer));
                    metadata.Author = hstring(buffer);
                    CoTaskMemFree(buffer);
                }
                else if (key == PKEY_Title)
                {
                    wchar_t* buffer;
                    check_hresult(PropVariantToStringAlloc(pv, &buffer));
                    metadata.Title = hstring(buffer);
                    CoTaskMemFree(buffer);
                }
                else if (key == PKEY_Music_AlbumTitle)
                {
                    wchar_t* buffer;
                    check_hresult(PropVariantToStringAlloc(pv, &buffer));
                    metadata.AlbumTitle = hstring(buffer);
                    CoTaskMemFree(buffer);
                }
                else if (key == PKEY_Audio_IsVariableBitRate)
                {
                    BOOL val;
                    check_hresult(PropVariantToBoolean(pv, &val));
                    metadata.AudioIsVariableBitrate = val;
                }
                else if (key == PKEY_Video_IsStereo)
                {
                    BOOL val;
                    check_hresult(PropVariantToBoolean(pv, &val));
                    metadata.VideoIsStereo = val;
                }

                check_hresult(PropVariantClear(&pv));
            }
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::GetMetadata: Failed to get media metadata;\n" << e.message() << '\n';

            MediaMetadata mm;
            mm.Path = path;
            return mm;
        }

        return metadata;
    }

    void PlayerService::VideoRender()
    {
        m_CurFrame = {};
        std::list<SubtitleItem> currentSubs;
        double videoStartTime = -1.0;

        while (m_State != PlayerServiceState::CLOSED && m_State != PlayerServiceState::SOURCE_SWITCH)
        {
            double audioTime = Position() / 1000.0;

            if (videoStartTime < 0)
            {
                videoStartTime = audioTime - m_CurFrame.FrameTime;
                m_CurFrame = m_FfmpegDecoder.GetNextFrame();
                m_LastFrameSize = { static_cast<float>(m_CurFrame.Width), static_cast<float>(m_CurFrame.Height) };
            }

            if (m_State != PlayerServiceState::PAUSED && !m_CurFrame.Buffer.empty())
            {
                double frameTime = videoStartTime + m_CurFrame.FrameTime;
                double syncOffset = audioTime - frameTime;
                constexpr double SYNC_THRESHOLD = 0.02;
                if (syncOffset > SYNC_THRESHOLD || m_Seeked)
                {
                    m_CurFrame = m_FfmpegDecoder.GetNextFrame();
                    m_Seeked = false;
                    m_LastFrameSize = { static_cast<float>(m_CurFrame.Width), static_cast<float>(m_CurFrame.Height) };
                }
            }

            if (!m_FfmpegDecoder.GetSubsQueue().empty() &&
                m_FfmpegDecoder.GetSubsQueue().front().EndTime < audioTime + (m_FfmpegDecoder.GetSubsQueue().front().EndTime - m_FfmpegDecoder.GetSubsQueue().front().StartTime))
            {
                currentSubs.push_back(m_FfmpegDecoder.GetSubsQueue().front());
                m_FfmpegDecoder.GetSubsQueue().pop();
            }

            if (!currentSubs.empty() && currentSubs.front().EndTime < audioTime)
            {
                currentSubs.pop_front();
            }

            if (m_ChangingSwapchain) continue;

            auto context = m_DeviceResources->GetD3DDeviceContext();

            if (!m_CurFrame.Buffer.empty())
            {
                m_TexturePlaneRenderer->SetImage(m_CurFrame.Buffer.data(), m_CurFrame.Width, m_CurFrame.Height);
            }

            float width = m_DesiredSize.Width;
            float height = m_DesiredSize.Height;

            if (m_ResizeNeeded && !m_CurFrame.Buffer.empty())
            {
                m_DeviceResources->SetLogicalSize({ width, height });
                m_TexturePlaneRenderer->CreateWindowSizeDependentResources();

                m_ResizeNeeded = false;
            }

            auto viewport = m_DeviceResources->GetScreenViewport();
            float panelAspect = width / height;
            float videoAspect = m_LastFrameSize.Width / m_LastFrameSize.Height;
            if (panelAspect > videoAspect)
            {
                float widthCorrected = height * videoAspect;
                viewport.TopLeftX = (width - widthCorrected) / 2;
                viewport.Width = widthCorrected;
                viewport.Height = height;
            }
            else if (videoAspect >= panelAspect)
            {
                float heightCorrected = width / videoAspect;
                viewport.TopLeftY = (height - heightCorrected) / 2;
                viewport.Width = width;
                viewport.Height = heightCorrected;
            }

            context->RSSetViewports(1, &viewport);

            ID3D11RenderTargetView* const targets[1] = { m_DeviceResources->GetBackBufferRenderTargetView() };
            if (!targets[0]) continue;
            context->OMSetRenderTargets(1, targets, m_DeviceResources->GetDepthStencilView());

            context->ClearRenderTargetView(m_DeviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::Transparent);
            context->ClearDepthStencilView(m_DeviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

            if (!m_CurFrame.Buffer.empty())
            {
                m_TexturePlaneRenderer->Render();
            }

            for (auto& sub : currentSubs)
            {
                m_TextRenderer->Render(sub.Text, 0.0f, -10.0f);
            }

            m_DeviceResources->Present();
        }
    }

    void PlayerService::OnLoaded()
    {
        SwapChainPanel(SwapChainPanel());
    }

    void PlayerService::OnPlaybackEnded()
    {
        m_UIDispatcherQueue.TryEnqueue([&]()
        {
            Next();
        });
    }

    void PlayerService::OnError(MF_MEDIA_ENGINE_ERR error, HRESULT hr)
    {
    }
}
