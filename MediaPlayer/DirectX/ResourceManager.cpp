#include "pch.h"
#include "ResourceManager.h"

#include "ranges"
#include "DirectXUtils.h"

using namespace winrt;

ResourceManager::ResourceManager(const std::shared_ptr<DeviceResources>& deviceResources)
    : m_DeviceResources(deviceResources)
{
}

ResourceManager::~ResourceManager()
{
    for (auto& [_, shader] : m_Shaders)
    {
        shader.PixelShader->Release();
        shader.VertexShader->Release();
        shader.InputLayout->Release();
    }
}

void ResourceManager::AddShader(winrt::hstring const& name, winrt::hstring const& vsFilename,
                                winrt::hstring const& psFilename)
{
    Shader shader;

    auto vsData = DXUtils::ReadData(vsFilename);
    auto psData = DXUtils::ReadData(psFilename);

    check_hresult(m_DeviceResources->GetD3DDevice()->CreateVertexShader(
        vsData.data(),
        vsData.size(),
        nullptr,
        shader.VertexShader.put()
    ));

    static constexpr D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    check_hresult(m_DeviceResources->GetD3DDevice()->CreateInputLayout(
        vertexDesc,
        ARRAYSIZE(vertexDesc),
        vsData.data(),
        vsData.size(),
        shader.InputLayout.put()
    ));

    check_hresult(m_DeviceResources->GetD3DDevice()->CreatePixelShader(
        psData.data(),
        psData.size(),
        nullptr,
        shader.PixelShader.put()
    ));

    m_Shaders[name] = shader;
}

Shader* ResourceManager::GetShader(winrt::hstring const& name)
{
    if (auto it = m_Shaders.find(name); it != m_Shaders.end())
    {
        return &it->second;
    }

    return nullptr;
}

std::vector<winrt::hstring> ResourceManager::GetShaderNames()
{
    auto view = m_Shaders | std::views::keys;
    return std::vector<hstring>(view.begin(), view.end());
}
