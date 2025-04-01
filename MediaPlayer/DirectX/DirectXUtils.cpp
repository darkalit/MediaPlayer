#include "pch.h"
#include "DirectXUtils.h"

#include "d3d11.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"
#include "pplawait.h"
#include "vector"
#include "string"
#include "mutex"
#include "future"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Streams;
using namespace concurrency;

namespace DXUtils
{
    concurrency::task<std::vector<uint8_t>> ReadDataAsync(winrt::hstring const& filename)
    {
        auto folder = Windows::ApplicationModel::Package::Current().InstalledLocation();

        StorageFile file = co_await folder.GetFileAsync(filename);
        IBuffer fileBuffer = co_await FileIO::ReadBufferAsync(file);

        std::vector<uint8_t> buffer(fileBuffer.Length());
        DataReader::FromBuffer(fileBuffer).ReadBytes(buffer);

        co_return buffer;
    }

    std::vector<uint8_t> ReadData(winrt::hstring const& filename)
    {
        auto task = std::async(std::launch::async, [filename]() -> std::vector<uint8_t>
        {
            auto folder = Windows::ApplicationModel::Package::Current().InstalledLocation();
            auto file = folder.GetFileAsync(filename).get();
            IBuffer fileBuffer = FileIO::ReadBufferAsync(file).get();
            std::vector<uint8_t> buffer(fileBuffer.Length());
            DataReader::FromBuffer(fileBuffer).ReadBytes(buffer);

            return buffer;
        });

        return task.get();
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
