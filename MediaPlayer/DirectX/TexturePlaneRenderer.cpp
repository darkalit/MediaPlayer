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
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

void TexturePlaneRenderer::Render()
{
    if (!m_LoadingComplete)
    {
        return;
    }

    auto context = m_DeviceResources->GetD3DDeviceContext();

    context->IASetVertexBuffers(
        0,
        1,
        m_VertexBuffer.put(),
        &m_Stride,
        &m_Offset
    );

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_InputLayout.get());

    context->VSSetShader(m_VertexShader.get(), nullptr, 0);
    context->PSSetShader(m_PixelShader.get(), nullptr, 0);
    context->PSSetSamplers(0, 1, m_SamplerState.put());
    context->Draw(m_VerticesCount, 0);
}
