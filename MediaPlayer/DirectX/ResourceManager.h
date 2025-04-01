#pragma once

#include "DeviceResources.h"

struct Shader
{
    winrt::com_ptr<ID3D11InputLayout> InputLayout;
    winrt::com_ptr<ID3D11VertexShader> VertexShader;
    winrt::com_ptr<ID3D11PixelShader> PixelShader;
};

class ResourceManager
{
public:
    ResourceManager(const std::shared_ptr<DeviceResources>& deviceResources);
    ~ResourceManager();

    void AddShader(winrt::hstring const& name, winrt::hstring const& vsFilename, winrt::hstring const& psFilename);
    Shader* GetShader(winrt::hstring const& name);
    std::vector<winrt::hstring> GetShaderNames();

private:
    std::shared_ptr<DeviceResources> m_DeviceResources;
    std::map<winrt::hstring, Shader> m_Shaders = {};
};
