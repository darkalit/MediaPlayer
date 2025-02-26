#include "pch.h"
#include "TextRenderer.h"

using namespace winrt;

TextRenderer::TextRenderer(const std::shared_ptr<DeviceResources>& deviceResources)
    : m_DeviceResources(deviceResources)
    , m_TextMetrics({})
{
    com_ptr<IDWriteTextFormat> textFormat;
    check_hresult(m_DeviceResources->GetDWriteFactory()->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_MEDIUM,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        32.0f,
        L"en-US",
        textFormat.put()
    ));

    textFormat.as(m_TextFormat);
    check_hresult(m_TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
    check_hresult(m_DeviceResources->GetD2DFactory()->CreateDrawingStateBlock(m_StateBlock.put()));

    CreateDeviceDependentResources();
}

void TextRenderer::CreateDeviceDependentResources()
{
    auto context = m_DeviceResources->GetD2DDeviceContext();
    check_hresult(context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), m_WhiteBrush.put()));
    check_hresult(context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), m_BlackBrush.put()));
}

void TextRenderer::Render(const winrt::hstring& text, float x, float y)
{
    com_ptr<IDWriteTextLayout> textLayout;
    check_hresult(m_DeviceResources->GetDWriteFactory()->CreateTextLayout(
        text.c_str(),
        text.size(),
        m_TextFormat.get(),
        m_DeviceResources->GetOutputSize().Width,
        50.0f,
        textLayout.put()
    ));

    textLayout.as(m_TextLayout);
    m_TextLayout->GetMetrics(&m_TextMetrics);

    auto context = m_DeviceResources->GetD2DDeviceContext();
    auto logicalSize = m_DeviceResources->GetLogicalSize();

    context->SaveDrawingState(m_StateBlock.get());
    context->BeginDraw();

    D2D1::Matrix3x2F screenTransform = D2D1::Matrix3x2F::Translation(
        logicalSize.Width - m_TextMetrics.layoutWidth,
        logicalSize.Height - m_TextMetrics.layoutHeight
    );
    context->SetTransform(screenTransform * m_DeviceResources->GetOrientationTransform2D());
    m_TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    float thickness = 1.6f;
    const D2D1_POINT_2F offsets[] =
    {
        {x - thickness, y - thickness},
        {x, y - thickness},
        {x + thickness, y - thickness},
        {x - thickness, y},
        {x + thickness, y},
        {x - thickness, y + thickness},
        {x, y + thickness},
        {x + thickness, y + thickness}
    };

    for (auto& offset : offsets)
    {
        context->DrawTextLayout(
            offset,
            m_TextLayout.get(),
            m_BlackBrush.get()
        );
    }

    context->DrawTextLayout(
        D2D1::Point2(x, y),
        m_TextLayout.get(),
        m_WhiteBrush.get()
    );

    HRESULT hr = context->EndDraw();
    if (hr != D2DERR_RECREATE_TARGET)
    {
        check_hresult(hr);
    }

    context->RestoreDrawingState(m_StateBlock.get());
}
