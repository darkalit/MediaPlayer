#include "pch.h"
#include "TexturePlaneRenderer.h"

#include "d3d11_3.h"
#include "DirectXUtils.h"
#include "App.xaml.h"

using namespace winrt;
using namespace DirectX;

TexturePlaneRenderer::TexturePlaneRenderer(const std::shared_ptr<DeviceResources>& deviceResources)
    : m_DeviceResources(deviceResources)
    , m_ResourceManager(MediaPlayer::implementation::App::GetResourceManager())
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

void TexturePlaneRenderer::CreateDeviceDependentResources()
{
    m_ResourceManager->AddShader(L"Default", L"PlaneVS.cso", L"PlanePS.cso");

    D3D11_SAMPLER_DESC samplerDesc = {
        .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 1,
        .ComparisonFunc = D3D11_COMPARISON_NEVER,
        .BorderColor = { 1.0f, 1.0f, 1.0f, 1.0f },
        .MinLOD = 0.0f,
        .MaxLOD = FLT_MAX,
    };

    check_hresult(m_DeviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, m_SamplerState.put()));

    static constexpr float vertexData[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 1.0f
    };

    CD3D11_BUFFER_DESC vertexBufferDesc(
        sizeof(vertexData),
        D3D11_BIND_VERTEX_BUFFER
    );

    D3D11_SUBRESOURCE_DATA vertexBufferData = {
        .pSysMem = vertexData,
        .SysMemPitch = 0,
        .SysMemSlicePitch = 0,
    };

    check_hresult(m_DeviceResources->GetD3DDevice()->CreateBuffer(
        &vertexBufferDesc,
        &vertexBufferData,
        m_VertexBuffer.put()
    ));

    m_Stride = 4 * sizeof(float);
    m_Offset = 0;
    m_VerticesCount = sizeof(vertexData) / m_Stride;

    m_LoadingComplete = true;
}

void TexturePlaneRenderer::CreateWindowSizeDependentResources()
{
}

void TexturePlaneRenderer::ReleaseDeviceDependentResources()
{
    m_LoadingComplete = false;
    m_VertexBuffer->Release();
    m_SamplerState->Release();
}

void TexturePlaneRenderer::SetImage(const uint8_t* data, uint32_t width, uint32_t height)
{
    if (!m_DeviceResources) return;

    if (!m_FrameTexture)
    {
        CreateFrameTexture(data, width, height);
        return;
    }

    D3D11_TEXTURE2D_DESC desc;
    m_FrameTexture->GetDesc(&desc);

    if (desc.Width != width || desc.Height != height)
    {
        CreateFrameTexture(data, width, height);
    }
    else
    {
        auto context = m_DeviceResources->GetD3DDeviceContext();
        D3D11_MAPPED_SUBRESOURCE mapped;
        check_hresult(context->Map(m_FrameTexture.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
        memcpy(mapped.pData, data, width * height * 4);
        context->Unmap(m_FrameTexture.get(), 0);
    }
}

void TexturePlaneRenderer::Render(winrt::hstring const& shaderName)
{
    if (!m_LoadingComplete || !m_DeviceResources)
    {
        return;
    }

    auto context = m_DeviceResources->GetD3DDeviceContext();

    ID3D11Buffer* vBuffers[] = { m_VertexBuffer.get() };
    context->IASetVertexBuffers(
        0,
        1,
        vBuffers,
        &m_Stride,
        &m_Offset
    );

    auto shader = m_ResourceManager->GetShader(shaderName);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(shader->InputLayout.get());

    context->VSSetShader(shader->VertexShader.get(), nullptr, 0);
    context->PSSetShader(shader->PixelShader.get(), nullptr, 0);

    ID3D11ShaderResourceView* srvs[] = { m_ImageShaderResourceView.get() };
    context->PSSetShaderResources(0, 1, srvs);

    ID3D11SamplerState* samplers[] = { m_SamplerState.get() };
    context->PSSetSamplers(0, 1, samplers);
    context->Draw(m_VerticesCount, 0);
}

void TexturePlaneRenderer::CreateFrameTexture(const uint8_t* data, uint32_t width, uint32_t height)
{
    if (!m_DeviceResources) return;

    D3D11_TEXTURE2D_DESC textureDesc = {
        .Width = width,
        .Height = height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0,
        },
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
    };

    auto device = m_DeviceResources->GetD3DDevice();

    if (data)
    {
        D3D11_SUBRESOURCE_DATA imageSubresourceData = {
            .pSysMem = data,
            .SysMemPitch = width * 4,
        };

        device->CreateTexture2D(&textureDesc, &imageSubresourceData, m_FrameTexture.put());
    }
    else
    {
        device->CreateTexture2D(&textureDesc, nullptr, m_FrameTexture.put());
    }

    device->CreateShaderResourceView(m_FrameTexture.get(), nullptr, m_ImageShaderResourceView.put());
}
