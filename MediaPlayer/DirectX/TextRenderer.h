#pragma once

#include "DeviceResources.h"

class TextRenderer
{
public:
    TextRenderer(const std::shared_ptr<DeviceResources>& deviceResources);
    void CreateDeviceDependentResources();
    void Render(const winrt::hstring& text, float x = 0.0f, float y = 0.0f);

private:
    std::shared_ptr<DeviceResources> m_DeviceResources;

    DWRITE_TEXT_METRICS m_TextMetrics;
    winrt::com_ptr<ID2D1SolidColorBrush> m_WhiteBrush;
    winrt::com_ptr<ID2D1SolidColorBrush> m_BlackBrush;
    winrt::com_ptr<IDWriteTextFormat2> m_TextFormat;
    winrt::com_ptr<IDWriteTextLayout3> m_TextLayout;
    winrt::com_ptr<ID2D1DrawingStateBlock1> m_StateBlock;
};

