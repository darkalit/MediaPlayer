#include "pch.h"
#include "DeviceResources.h"

#include "App.xaml.h"
#include "dxgi1_4.h"
#include "d3d11_3.h"
#include "d2d1_3.h"
#include "d2d1effects_2.h"
#include "dwrite_3.h"
#include "winuser.h"
#include "windows.ui.xaml.media.dxinterop.h"

#include "DirectXUtils.h"

#include "winrt/Windows.Graphics.Display.h"

using namespace winrt;
using namespace Windows::Graphics::Display;

DeviceResources::DeviceResources()
    : m_D3dFeatureLevel(D3D_FEATURE_LEVEL_11_1)
    , m_NativeOrientation(DisplayOrientations::None)
    , m_CurrentOrientation(DisplayOrientations::None)
    , m_CompositionScale({ 1.0f, 1.0f })
    , m_Dpi(-1.0f)
{
    CreateDeviceResources();
}

void DeviceResources::HandleDeviceLost()
{
    m_SwapChain = nullptr;

    CreateDeviceResources();
    CreateWindowSizeDependentResources();
}

void DeviceResources::Present()
{
    DXGI_PRESENT_PARAMETERS parameters = {};
    auto hr = m_SwapChain->Present1(1, 0, &parameters);

    m_D3dContext->DiscardView1(m_D3dRenderTargetView.get(), nullptr, 0);
    m_D3dContext->DiscardView1(m_D3dDepthStencilView.get(), nullptr, 0);

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        HandleDeviceLost();
    }
    else
    {
        check_hresult(hr);
    }
}

void DeviceResources::SetSwapChainPanel(winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel)
{
    //auto currentDisplayInfo = Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(DisplayMonitor::)

    m_SwapChainPanel = panel;
    m_CurrentOrientation = DisplayOrientations::Landscape;
    m_Dpi = GetDpiForWindow(MediaPlayer::implementation::App::GetMainWindow());
    m_LogicalSize = panel.ActualSize();
    m_CompositionScale = { panel.CompositionScaleX(), panel.CompositionScaleY() };

    CreateWindowSizeDependentResources();
}

void DeviceResources::SetCompositionScale(winrt::Windows::Foundation::Size const& compositionScale)
{
    if (m_CompositionScale != compositionScale)
    {
        m_CompositionScale = compositionScale;
        CreateWindowSizeDependentResources();
    }
}

void DeviceResources::SetCurrentOrientation(winrt::Windows::Graphics::Display::DisplayOrientations orientation)
{
    if (m_CurrentOrientation != orientation)
    {
        m_CurrentOrientation = orientation;
        CreateWindowSizeDependentResources();
    }
}

winrt::Windows::Foundation::Size DeviceResources::GetOutputSize() const
{
    return m_OutputSize;
}

void DeviceResources::SetLogicalSize(winrt::Windows::Foundation::Size const& logicalSize)
{
    if (m_LogicalSize != logicalSize)
    {
        m_LogicalSize = logicalSize;
        CreateWindowSizeDependentResources();
    }
}

winrt::Windows::Foundation::Size DeviceResources::GetLogicalSize() const
{
    return m_LogicalSize;
}

void DeviceResources::SetDpi(float dpi)
{
    if (m_Dpi != dpi)
    {
        m_Dpi = dpi;
        CreateWindowSizeDependentResources();
    }
}

float DeviceResources::GetDpi() const
{
    return m_Dpi;
}

ID3D11Device3* DeviceResources::GetD3DDevice() const
{
    return m_D3dDevice.get();
}

ID3D11DeviceContext3* DeviceResources::GetD3DDeviceContext() const
{
    return m_D3dContext.get();
}

IDXGISwapChain3* DeviceResources::GetSwapChain() const
{
    return m_SwapChain.get();
}

D3D_FEATURE_LEVEL DeviceResources::GetDeviceFeatureLevel() const
{
    return m_D3dFeatureLevel;
}

ID3D11RenderTargetView1* DeviceResources::GetBackBufferRenderTargetView() const
{
    return m_D3dRenderTargetView.get();
}

ID3D11DepthStencilView* DeviceResources::GetDepthStencilView() const
{
    return m_D3dDepthStencilView.get();
}

D3D11_VIEWPORT DeviceResources::GetScreenViewport() const
{
    return m_ScreenViewport;
}

void DeviceResources::CreateDeviceResources()
{
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
    if (DXUtils::SdkLayersAvailable())
    {
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    com_ptr<ID3D11Device> device;
    com_ptr<ID3D11DeviceContext> context;

    auto hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        device.put(),
        &m_D3dFeatureLevel,
        context.put()
    );

    if (FAILED(hr))
    {
        check_hresult(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            creationFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            device.put(),
            &m_D3dFeatureLevel,
            context.put()
        ));
    }

    device.as(m_D3dDevice);
    context.as(m_D3dContext);
}

void DeviceResources::CreateWindowSizeDependentResources()
{
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_D3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
    m_D3dRenderTargetView = nullptr;
    m_D3dContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

    UpdateRenderTargetSize();

    DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

    bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
    m_D3dRenderTargetSize.Width = swapDimensions ? m_OutputSize.Height : m_OutputSize.Width;
    m_D3dRenderTargetSize.Height = swapDimensions ? m_OutputSize.Width : m_OutputSize.Height;

    if (m_SwapChain)
    {
        auto hr = m_SwapChain->ResizeBuffers(
            2,
            lround(m_D3dRenderTargetSize.Width),
            lround(m_D3dRenderTargetSize.Height),
            DXGI_FORMAT_B8G8R8A8_UNORM,
            0
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            HandleDeviceLost();
            return;
        }
        else
        {
            check_hresult(hr);
        }
    }
    else
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = static_cast<UINT>(lround(m_D3dRenderTargetSize.Width)),
            .Height = static_cast<UINT>(lround(m_D3dRenderTargetSize.Height)),
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
            .Stereo = false,
            .SampleDesc = {
                .Count = 1,
                .Quality = 0,
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .Scaling = DXGI_SCALING_STRETCH,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
            .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
            .Flags = 0,
        };

        com_ptr<IDXGIDevice3> dxgiDevice;
        m_D3dDevice.as(dxgiDevice);

        com_ptr<IDXGIAdapter> dxgiAdapter;
        check_hresult(dxgiDevice->GetAdapter(dxgiAdapter.put()));

        com_ptr<IDXGIFactory4> dxgiFactory;
        check_hresult(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.put())));

        com_ptr<IDXGISwapChain1> swapChain;
        check_hresult(dxgiFactory->CreateSwapChainForComposition(
            m_D3dDevice.get(),
            &swapChainDesc,
            nullptr,
            swapChain.put()
        ));

        swapChain.as(m_SwapChain);

        m_SwapChainPanel.DispatcherQueue().TryEnqueue([=]()
        {
            com_ptr<ISwapChainPanelNative2> panelNative;
            //m_SwapChainPanel.as(panelNative);
        //    panelNative->SetSwapChain(m_SwapChain.get());

            check_hresult(dxgiDevice->SetMaximumFrameLatency(1));
        });
    }

    //check_hresult(m_SwapChain->SetRotation(displayRotation));

    DXGI_MATRIX_3X2_F inverseScale = {0};
    inverseScale._11 = 1.0f / m_CompositionScale.Width;
    inverseScale._22 = 1.0f / m_CompositionScale.Height;
    check_hresult(m_SwapChain->SetMatrixTransform(&inverseScale));

    com_ptr<ID3D11Texture2D1> backBuffer;
    check_hresult(m_SwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.put())));
    m_D3dDevice->CreateRenderTargetView1(
        backBuffer.get(),
        nullptr,
        m_D3dRenderTargetView.put()
    );

    CD3D11_TEXTURE2D_DESC1 depthStencilDesc (
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        lround(m_D3dRenderTargetSize.Width),
        lround(m_D3dRenderTargetSize.Height),
        1,
        1,
        D3D11_BIND_DEPTH_STENCIL
    );

    com_ptr<ID3D11Texture2D1> depthStencil;
    check_hresult(m_D3dDevice->CreateTexture2D1(
        &depthStencilDesc,
        nullptr,
        depthStencil.put()
    ));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    check_hresult(m_D3dDevice->CreateDepthStencilView(
        depthStencil.get(),
        &depthStencilViewDesc,
        m_D3dDepthStencilView.put()
    ));

    m_ScreenViewport = CD3D11_VIEWPORT(
        0.0f,
        0.0f,
        m_D3dRenderTargetSize.Width,
        m_D3dRenderTargetSize.Height
    );
    m_D3dContext->RSSetViewports(1, &m_ScreenViewport);
}

void DeviceResources::UpdateRenderTargetSize()
{
    m_OutputSize.Width = max(DXUtils::ConvertDipsToPixels(m_LogicalSize.Width, m_Dpi), 1);
    m_OutputSize.Height = max(DXUtils::ConvertDipsToPixels(m_LogicalSize.Height, m_Dpi), 1);
}

DXGI_MODE_ROTATION DeviceResources::ComputeDisplayRotation()
{
    DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

    if (m_NativeOrientation == DisplayOrientations::Landscape)
    {
        switch (m_CurrentOrientation)
        {
        case DisplayOrientations::Landscape:
            rotation = DXGI_MODE_ROTATION_IDENTITY;
            break;

        case DisplayOrientations::Portrait:
            rotation = DXGI_MODE_ROTATION_ROTATE270;
            break;

        case DisplayOrientations::LandscapeFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE180;
            break;

        case DisplayOrientations::PortraitFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE90;
            break;

        default:
            break;
        }
    }
    else if (m_NativeOrientation == DisplayOrientations::Portrait)
    {
        switch (m_CurrentOrientation)
        {
        case DisplayOrientations::Landscape:
            rotation = DXGI_MODE_ROTATION_ROTATE90;
            break;

        case DisplayOrientations::Portrait:
            rotation = DXGI_MODE_ROTATION_IDENTITY;
            break;

        case DisplayOrientations::LandscapeFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE270;
            break;

        case DisplayOrientations::PortraitFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE180;
            break;

        default:
            break;
        }
    }
    return rotation;
}
