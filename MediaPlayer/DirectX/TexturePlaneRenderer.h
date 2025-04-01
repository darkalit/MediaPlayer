#pragma once

#include "DeviceResources.h"
#include "ResourceManager.h"

class TexturePlaneRenderer
{
public:
    TexturePlaneRenderer(const std::shared_ptr<DeviceResources>& deviceResources);
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void ReleaseDeviceDependentResources();
    void SetImage(const uint8_t* data, uint32_t width, uint32_t height);
    void Render(winrt::hstring const& shaderName);

private:
    void CreateFrameTexture(const uint8_t* data, uint32_t width, uint32_t height);

    std::shared_ptr<DeviceResources> m_DeviceResources;
    std::shared_ptr<ResourceManager> m_ResourceManager;

    //winrt::com_ptr<ID3D11InputLayout> m_InputLayout;
    winrt::com_ptr<ID3D11Buffer> m_VertexBuffer;
    //winrt::com_ptr<ID3D11VertexShader> m_VertexShader;
    //winrt::com_ptr<ID3D11PixelShader> m_PixelShader;
    winrt::com_ptr<ID3D11SamplerState> m_SamplerState;
    winrt::com_ptr<ID3D11Texture2D> m_FrameTexture;
    winrt::com_ptr<ID3D11ShaderResourceView> m_ImageShaderResourceView;

    uint32_t m_VerticesCount = 0;
    uint32_t m_Stride = 0;
    uint32_t m_Offset = 0;

    bool m_LoadingComplete = false;
};

