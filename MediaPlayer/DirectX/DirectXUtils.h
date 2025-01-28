#pragma once
#include "ppltasks.h"

namespace DXUtils
{
    concurrency::task<std::vector<uint8_t>> ReadDataAsync(const std::wstring& filename);
    float ConvertDipsToPixels(float dips, float dpi);
#ifdef _DEBUG
    bool SdkLayersAvailable();
#endif
}
