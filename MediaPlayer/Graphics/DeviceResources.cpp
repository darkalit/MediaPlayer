#include "pch.h"
#include "DeviceResources.h"

#include "App.xaml.h"
#include "winrt/Windows.Graphics.Display.h"
#include "microsoft.ui.xaml.media.dxinterop.h"

using namespace winrt::Windows::Graphics::Display;

DX::DeviceResources::DeviceResources()
{
    unsigned int creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    D3D_FEATURE_LEVEL featureLevels[] = {
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

    D3D_FEATURE_LEVEL supportedFeatureLevel;
    winrt::check_hresult(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        creationFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        m_D3dDevice.put(),
        &supportedFeatureLevel,
        nullptr
    ));

    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    m_D3dDevice.as(dxgiDevice);

    winrt::check_hresult(D2D1CreateDevice(dxgiDevice.get(), nullptr, m_D2dDevice.put()));
    winrt::check_hresult(m_D2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_D2dContext.put()));
}

void DX::DeviceResources::SetSwapChainPanel(winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel, HWND mainWindowHwnd)
{
    m_SwapChainPanel = const_cast<winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel*>(&panel);
    m_LogicalWidth = panel.ActualWidth();
    m_LogicalHeight = panel.ActualHeight();
    m_CompositionScaleX = panel.CompositionScaleX();
    m_CompositionScaleY = panel.CompositionScaleY();
    m_Dpi = GetDpiForWindow(mainWindowHwnd);
    m_D2dContext->SetDpi(m_Dpi, m_Dpi);

    CreateWindowSizeDependentResources();
}

void DX::DeviceResources::Present()
{
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.f,
        96.f
    );

    winrt::com_ptr<IDXGISurface> dxgiBackBuffer;
    m_SwapChain->GetBuffer(0, __uuidof(dxgiBackBuffer), dxgiBackBuffer.put_void());

    winrt::com_ptr<ID2D1Bitmap1> targetBitmap;
    winrt::check_hresult(m_D2dContext->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.get(),
        &bitmapProperties,
        targetBitmap.put()
    ));

    m_D2dContext->SetTarget(targetBitmap.get());

    m_D2dContext->BeginDraw();

    m_D2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Orange));

    m_D2dContext->EndDraw();

    m_SwapChain->Present(1, 0);
}

//HWND DX::DeviceResources::GetSwapChainHWND()
//{
//    //HWND hwnd;
//    //winrt::check_hresult(m_SwapChain->GetHwnd(&hwnd));
//    //return hwnd;
//    return HWND(m_SwapChain.get());
//}

void DX::DeviceResources::CreateWindowSizeDependentResources()
{
    UpdateRenderTargetSize();

    if (m_SwapChain)
    {
        winrt::check_hresult(m_SwapChain->ResizeBuffers(
            2,
            lround(m_OutputWidth),
            lround(m_OutputHeight),
            DXGI_FORMAT_B8G8R8A8_UNORM,
            0
        ));
    }
    else
    {
        winrt::com_ptr<IDXGIDevice> dxgiDevice;
        m_D3dDevice.as(dxgiDevice);

        winrt::com_ptr<IDXGIAdapter> dxgiAdapter;
        dxgiDevice->GetAdapter(dxgiAdapter.put());

        winrt::com_ptr<IDXGIFactory2> dxgiFactory;
        dxgiFactory.capture(dxgiAdapter, &IDXGIAdapter::GetParent);

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = lround(m_OutputWidth);
        swapChainDesc.Height = lround(m_OutputHeight);
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // TODO: change for best results
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.Flags = 0;

        winrt::check_hresult(dxgiFactory->CreateSwapChainForComposition(
            m_D3dDevice.get(),
            &swapChainDesc,
            nullptr,
            m_SwapChain.put()
        ));

        winrt::com_ptr<ISwapChainPanelNative> panelNative;
        m_SwapChainPanel->as(panelNative);
        winrt::check_hresult(panelNative->SetSwapChain(m_SwapChain.get()));
    }
}

void DX::DeviceResources::UpdateRenderTargetSize()
{
    m_OutputWidth = ConvertDipsToPixels(m_LogicalWidth, m_Dpi);
    m_OutputHeight = ConvertDipsToPixels(m_LogicalHeight, m_Dpi);

    m_OutputWidth = max(m_OutputWidth, 1);
    m_OutputHeight = max(m_OutputHeight, 1);
}

double DX::ConvertDipsToPixels(double dips, double dpi)
{
    static const double dipsPerInch = 96.0;
    return floorl(dips * dpi / dipsPerInch + 0.5);
}
