#include "pch.h"
#include "TexturePlaneRenderer.h"

#include "d3d11_3.h"
#include "DirectXUtils.h"

using namespace winrt;
using namespace DirectX;

TexturePlaneRenderer::TexturePlaneRenderer(const std::shared_ptr<DeviceResources>& deviceResources)
    : m_DeviceResources(deviceResources)
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

void TexturePlaneRenderer::CreateDeviceDependentResources()
{
    auto loadVSTask = DXUtils::ReadDataAsync(L"PlaneVS.cso");
    auto loadPSTask = DXUtils::ReadDataAsync(L"PlanePS.cso");

    auto createVSTask = loadVSTask.then([this](const std::vector<uint8_t>& fileData)
    {
        check_hresult(m_DeviceResources->GetD3DDevice()->CreateVertexShader(
            fileData.data(),
            fileData.size(),
            nullptr,
            m_VertexShader.put()
        ));

        static constexpr D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        check_hresult(m_DeviceResources->GetD3DDevice()->CreateInputLayout(
            vertexDesc,
            ARRAYSIZE(vertexDesc),
            fileData.data(),
            fileData.size(),
            m_InputLayout.put()
        ));
    });

    auto createPSTask = loadPSTask.then([this](const std::vector<uint8_t>& fileData)
    {
        check_hresult(m_DeviceResources->GetD3DDevice()->CreatePixelShader(
            fileData.data(),
            fileData.size(),
            nullptr,
            m_PixelShader.put()
        ));

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
    });

    auto createPlaneTask = (createVSTask && createPSTask).then([this]()
    {
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
    });

    createPlaneTask.then([this]()
    {
        m_LoadingComplete = true;
    });
}

void TexturePlaneRenderer::CreateWindowSizeDependentResources()
{
}

void TexturePlaneRenderer::ReleaseDeviceDependentResources()
{
    m_LoadingComplete = false;
    m_InputLayout->Release();
    m_VertexBuffer->Release();
    m_VertexShader->Release();
    m_PixelShader->Release();
    m_SamplerState->Release();
}

void TexturePlaneRenderer::SetImage(const uint8_t* data, uint32_t width, uint32_t height)
{
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

void TexturePlaneRenderer::Render()
{
    if (!m_LoadingComplete)
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

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_InputLayout.get());

    context->VSSetShader(m_VertexShader.get(), nullptr, 0);
    context->PSSetShader(m_PixelShader.get(), nullptr, 0);

    ID3D11ShaderResourceView* srvs[] = { m_ImageShaderResourceView.get() };
    context->PSSetShaderResources(0, 1, srvs);

    ID3D11SamplerState* samplers[] = { m_SamplerState.get() };
    context->PSSetSamplers(0, 1, samplers);
    context->Draw(m_VerticesCount, 0);
}

void TexturePlaneRenderer::CreateFrameTexture(const uint8_t* data, uint32_t width, uint32_t height)
{
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
