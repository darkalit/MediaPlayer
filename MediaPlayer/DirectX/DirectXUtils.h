#pragma once
#include "ppltasks.h"

namespace DXUtils
{
    concurrency::task<std::vector<uint8_t>> ReadDataAsync(winrt::hstring const& filename);
    std::vector<uint8_t> ReadData(winrt::hstring const& filename);
    float ConvertDipsToPixels(float dips, float dpi);
#ifdef _DEBUG
    bool SdkLayersAvailable();
#endif
}
