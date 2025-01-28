#include "pch.h"
#include "DirectXUtils.h"

#include "d3d11.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"
#include "pplawait.h"
#include "vector"
#include "string"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Streams;
using namespace concurrency;

namespace DXUtils
{
    concurrency::task<std::vector<uint8_t>> ReadDataAsync(const std::wstring& filename)
    {
        auto folder = Windows::ApplicationModel::Package::Current().InstalledLocation();

        //auto folder = ApplicationData::Current().LocalFolder();

        StorageFile file = co_await folder.GetFileAsync(filename);
        IBuffer fileBuffer = co_await FileIO::ReadBufferAsync(file);

        std::vector<uint8_t> buffer(fileBuffer.Length());
        DataReader::FromBuffer(fileBuffer).ReadBytes(buffer);

        co_return buffer;
    }

    float ConvertDipsToPixels(float dips, float dpi)
    {
        constexpr float dipsPerInch = 96.0f;
        return floorf(dips * dpi / dipsPerInch + 0.5f);
    }
#ifdef _DEBUG
    bool SdkLayersAvailable()
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,
            0,
            D3D11_CREATE_DEVICE_DEBUG,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            nullptr,
            nullptr,
            nullptr
        );

        return SUCCEEDED(hr);
    }
#endif
}
