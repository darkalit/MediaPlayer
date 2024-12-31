#pragma once

#include "dxgi1_2.h"
#include "d3d11_1.h"
#include "d2d1_1.h"

namespace DX
{
    class DeviceResources
    {
    public:
        DeviceResources();
        void SetSwapChainPanel(winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel, HWND mainWindowHwnd);
        void Present();

    private:
        void CreateWindowSizeDependentResources();
        void UpdateRenderTargetSize();

        winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel* m_SwapChainPanel;

        winrt::com_ptr<ID3D11Device> m_D3dDevice;
        winrt::com_ptr<IDXGISwapChain1> m_SwapChain;
        winrt::com_ptr<ID2D1Device> m_D2dDevice;
        winrt::com_ptr<ID2D1DeviceContext> m_D2dContext;

        double m_Dpi = 0;
        double m_LogicalWidth = 0;
        double m_LogicalHeight = 0;
        double m_CompositionScaleX = 0;
        double m_CompositionScaleY = 0;
        double m_OutputWidth = 0;
        double m_OutputHeight = 0;
    };

    double ConvertDipsToPixels(double dips, double dpi);
}
