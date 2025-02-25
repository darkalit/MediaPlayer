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
            m_AudioThread.join();

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

            m_UseFfmpeg = false;
            SwapChainPanel(SwapChainPanel());

            return;
        }

        if (!(!m_IsMFSupported || m_Mode != PlayerServiceMode::FFMPEG))
        {
            return;
        }

        m_UseFfmpeg = true;
        SwapChainPanel(SwapChainPanel());
        m_FfmpegDecoder.OpenFile(path);
        m_SubTracks.Clear();
        for (auto& s : m_FfmpegDecoder.GetSubtitleStreams())
        {
            m_SubTracks.Append(s);
        }

        Position(0);
        State(PlayerServiceState::STOPPED);

        m_FfmpegDecoder.PauseDecoding(true);
        m_FfmpegDecoder.SetupDecoding(m_FrameQueue, m_SubtitleQueue, m_AudioSamplesQueue);

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

        if (m_AudioThread.joinable())
        {
            m_AudioThread.join();
        }

        m_AudioThread = std::thread(&PlayerService::AudioRender, this);
    }

    void PlayerService::SetSubtitleIndex(int32_t index)
    {
        m_FfmpegDecoder.OpenSubtitle(index);
    }

    void PlayerService::SetSubtitleFromFile(hstring const& path)
    {
        m_FfmpegDecoder.OpenSubtitle(path);
    }

    bool PlayerService::HasSource()
    {
        if (!m_UseFfmpeg)
        {
            return !!m_SourceReader && m_MediaEngineWrapper && m_Playlist.Size() > 0 && CurrentMediaIndex() > -1;
        }

        return m_FfmpegDecoder.HasSource();
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

        if (!m_UseFfmpeg)
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
        m_FfmpegDecoder.PauseDecoding(false);
        if (!HasSource()) return;

        try
        {
            if (CurrentMediaIndex() > -1 && Position() >= Metadata().Duration)
            {
                Stop();
                return;
            }

            double position = static_cast<double>(Position()) / 1000.0;
            if (m_UseFfmpeg)
            {
                m_FfmpegDecoder.Seek(Position());
                m_XAudio2Player.Start();
                m_FrameQueue.Clear();
                m_AudioSamplesQueue.Clear();
                m_SubtitleQueue.Clear();
            }
            else
            {
                m_MediaEngineWrapper->Start(position);
            }
            m_Seeked = true;
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
        m_FfmpegDecoder.PauseDecoding(false);
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
            if (m_UseFfmpeg)
            {
                m_FfmpegDecoder.Seek(timePos);
                m_XAudio2Player.Start();
                m_FrameQueue.Clear();
                m_AudioSamplesQueue.Clear();
                m_SubtitleQueue.Clear();
            }
            else
            {
                m_MediaEngineWrapper->Start(position);
            }
            m_Seeked = true;
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
        m_FfmpegDecoder.PauseDecoding(true);
        if (!HasSource()) return;

        try
        {
            m_Position = 0;

            if (m_UseFfmpeg)
            {
                m_XAudio2Player.Stop();
                m_FrameQueue.Clear();
                m_AudioSamplesQueue.Clear();
                m_SubtitleQueue.Clear();
            }
            else
            {
                m_MediaEngineWrapper->Stop();
            }
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Stop: Failed to stop media;\n" << e.message() << '\n';
        }
    }

    void PlayerService::Pause()
    {
        State(PlayerServiceState::PAUSED);
        m_FfmpegDecoder.PauseDecoding(true);
        if (!HasSource()) return;

        try
        {
            if (!m_UseFfmpeg) m_MediaEngineWrapper->Pause();
        }
        catch (const hresult_error& e)
        {
            std::wcerr << L"PlayerService::Pause: Failed to stop media;\n" << e.message() << '\n';
        }
    }

    void PlayerService::ResizeVideo(float width, float height)
    {
        if (!m_MediaEngineWrapper) return;
        if (!m_UseFfmpeg)
        {
            m_MediaEngineWrapper->WindowUpdate(width, height);
            return;
        }

        m_ResizeNeeded = true;
        m_DesiredSize = { width, height };
    }

    uint64_t PlayerService::Position()
    {
        if (!m_UseFfmpeg)
        {
            m_Position = m_MediaEngineWrapper->GetCurrentTime() * 1000.0;   
        }
        else
        {
            m_Position = m_FfmpegDecoder.GetPosition();
        }

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

            if (!m_UseFfmpeg) m_MediaEngineWrapper->SetPlaybackSpeed(value);
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
        if (!m_UseFfmpeg)
        {
            return m_MediaEngineWrapper->GetVolume();
        }

        return 1.0;
    }

    void PlayerService::Volume(double value)
    {
        if (Volume() != value)
        {
            if (!m_UseFfmpeg) m_MediaEngineWrapper->SetVolume(value);
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

        if (!m_UseFfmpeg)
        {
            m_VideoSurfaceHandle = m_MediaEngineWrapper ? m_MediaEngineWrapper->GetSurfaceHandle() : nullptr;
        }

        m_UIDispatcherQueue.TryEnqueue([&]()
        {
            m_DeviceResources->SetSwapChainPanel(m_SwapChainPanel);

            com_ptr<ISwapChainPanelNative2> panelNative;
            m_SwapChainPanel.as(panelNative);

            if (!m_UseFfmpeg)
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
        return FfmpegDecoder::GetMetadata(path);
    }

    void PlayerService::VideoRender()
    {
        m_CurFrame = {};
        std::list<SubtitleItem> currentSubs;

        while (m_State != PlayerServiceState::CLOSED && m_State != PlayerServiceState::SOURCE_SWITCH)
        {
            if (m_FrameQueue.Empty())
            {
                continue;
            }
            
            m_CurFrame = m_FrameQueue.Pop();
            m_LastFrameSize = { static_cast<float>(m_CurFrame.Width), static_cast<float>(m_CurFrame.Height) };
            
            double diff = m_CurFrame.StartTime - m_CurAudioSample.StartTime;
            auto timestampDiff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(diff)).count();
            if (timestampDiff > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(timestampDiff));
            }

            if (!m_SubtitleQueue.Empty() &&
                m_SubtitleQueue.Front().EndTime < m_CurAudioSample.StartTime + (m_SubtitleQueue.Front().EndTime - m_SubtitleQueue.Front().StartTime))
            {
                currentSubs.push_back(m_SubtitleQueue.Pop());
            }

            if (!currentSubs.empty() && currentSubs.front().EndTime < m_CurAudioSample.StartTime)
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

    void PlayerService::AudioRender()
    {
        m_CurAudioSample = {};

        while (m_State != PlayerServiceState::CLOSED && m_State != PlayerServiceState::SOURCE_SWITCH)
        {
            if (m_AudioSamplesQueue.Empty()) continue;

            double diff = m_CurAudioSample.StartTime - m_CurFrame.StartTime;
            auto timestampDiff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(diff)).count();
            if (timestampDiff > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(timestampDiff));
            }

            if (m_AudioSamplesQueue.Size() < 24 || !m_XAudio2Player.IsSampleConsumed()) continue;

            m_CurAudioSample = {};
            m_CurAudioSample.Duration = 0;
            for (int i = 0; i < 24; ++i)
            {
                if (m_AudioSamplesQueue.Empty())
                {
                    break;
                }

                auto sample = m_AudioSamplesQueue.Pop();
                if (i == 0)
                {
                    m_CurAudioSample.StartTime = sample.StartTime;
                }

                m_CurAudioSample.Duration += sample.Duration;
                m_CurAudioSample.Buffer.insert(m_CurAudioSample.Buffer.end(), sample.Buffer.begin(), sample.Buffer.end());
            }

            if (!m_CurAudioSample.Buffer.empty() && m_State != PlayerServiceState::PAUSED && m_State != PlayerServiceState::STOPPED)
            {
                m_XAudio2Player.SubmitSample(m_CurAudioSample);
            }
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
