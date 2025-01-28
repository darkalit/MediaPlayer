#pragma once

#include "DeviceResources.h"

class TexturePlaneRenderer
{
public:
    TexturePlaneRenderer(const std::shared_ptr<DeviceResources>& deviceResources);
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void ReleaseDeviceDependentResources();
    void Render();

private:
    std::shared_ptr<DeviceResources> m_DeviceResources;

    winrt::com_ptr<ID3D11InputLayout> m_InputLayout;
    winrt::com_ptr<ID3D11Buffer> m_VertexBuffer;
    winrt::com_ptr<ID3D11VertexShader> m_VertexShader;
    winrt::com_ptr<ID3D11PixelShader> m_PixelShader;
    winrt::com_ptr<ID3D11SamplerState> m_SamplerState;

    uint32_t m_VerticesCount = 0;
    uint32_t m_Stride = 0;
    uint32_t m_Offset = 0;

    bool m_LoadingComplete = false;
};

