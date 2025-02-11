#pragma once

#include "d3d11_3.h"
#include "dxgi1_4.h"
#include "d2d1_3.h"
#include "dwrite_3.h"

class DeviceResources
{
public:
    DeviceResources();
    void HandleDeviceLost();
    void Present();

    void SetSwapChainPanel(winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel);
    void SetCompositionScale(winrt::Windows::Foundation::Size const& compositionScale);
    void SetCurrentOrientation(winrt::Windows::Graphics::Display::DisplayOrientations orientation);
    winrt::Windows::Foundation::Size GetOutputSize() const;

    void SetLogicalSize(winrt::Windows::Foundation::Size const& logicalSize);
    winrt::Windows::Foundation::Size GetLogicalSize() const;

    void SetDpi(float dpi);
    float GetDpi() const;

    ID3D11Device3* GetD3DDevice() const;
    ID3D11DeviceContext3* GetD3DDeviceContext() const;
    IDXGISwapChain3* GetSwapChain() const;
    D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const;
    ID3D11RenderTargetView1* GetBackBufferRenderTargetView() const;
    ID3D11DepthStencilView* GetDepthStencilView() const;
    D3D11_VIEWPORT GetScreenViewport() const;

    ID2D1Factory3* GetD2DFactory() const;
    ID2D1Device2* GetD2DDevice() const;
    ID2D1DeviceContext2* GetD2DDeviceContext() const;
    ID2D1Bitmap1* GetD2DTargetBitmap() const;
    IDWriteFactory3* GetDWriteFactory() const;
    D2D1::Matrix3x2F GetOrientationTransform2D() const;

private:
    void CreateDeviceIndependentResources();
    void CreateDeviceResources();
    void CreateWindowSizeDependentResources();
    void UpdateRenderTargetSize();
    DXGI_MODE_ROTATION ComputeDisplayRotation();

    winrt::com_ptr<ID3D11Device3> m_D3dDevice;
    winrt::com_ptr<ID3D11DeviceContext3> m_D3dContext;
    winrt::com_ptr<IDXGISwapChain3> m_SwapChain;

    winrt::com_ptr<ID3D11RenderTargetView1> m_D3dRenderTargetView;
    winrt::com_ptr<ID3D11DepthStencilView> m_D3dDepthStencilView;
    D3D11_VIEWPORT m_ScreenViewport;

    winrt::com_ptr<ID2D1Factory3> m_D2dFactory;
    winrt::com_ptr<ID2D1Device2> m_D2dDevice;
    winrt::com_ptr<ID2D1DeviceContext2> m_D2dContext;
    winrt::com_ptr<ID2D1Bitmap1> m_D2dTargetBitmap;
    winrt::com_ptr<IDWriteFactory3> m_DWriteFactory;

    winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel m_SwapChainPanel;

    D3D_FEATURE_LEVEL m_D3dFeatureLevel;
    winrt::Windows::Foundation::Size m_D3dRenderTargetSize;
    winrt::Windows::Foundation::Size m_OutputSize;
    winrt::Windows::Foundation::Size m_LogicalSize;
    winrt::Windows::Graphics::Display::DisplayOrientations m_NativeOrientation;
    winrt::Windows::Graphics::Display::DisplayOrientations m_CurrentOrientation;
    winrt::Windows::Foundation::Size m_CompositionScale;
    float m_Dpi;

    D2D1::Matrix3x2F m_OrientationTransform2D;
};

