#include "pch.h"
#include "IsSelectedColorConverter.h"
#include "IsSelectedColorConverter.g.cpp"

#include "App.xaml.h"

using namespace winrt::Microsoft::UI::Xaml;

namespace winrt::MediaPlayer::implementation
{
    Windows::Foundation::IInspectable IsSelectedColorConverter::Convert(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language)
    {
        auto textColor = Application::Current().Resources().TryLookup(box_value(L"TextFillColorPrimaryBrush")).as<Media::SolidColorBrush>();
        auto accentTextColor = Application::Current().Resources().TryLookup(box_value(L"AccentTextFillColorPrimaryBrush")).as<Media::SolidColorBrush>();
        auto currentMediaId = unbox_value<guid>(value);

        if (App::GetPlayerService().Metadata().Id == currentMediaId)
        {
            return box_value(accentTextColor);
        }

        return box_value(textColor);
    }
    Windows::Foundation::IInspectable IsSelectedColorConverter::ConvertBack(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language)
    {
        throw hresult_not_implemented();
    }
}
