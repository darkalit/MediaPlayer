#pragma once
#include "DateToStringConverter.g.h"

namespace winrt::MediaPlayer::implementation
{
    struct DateToStringConverter : DateToStringConverterT<DateToStringConverter>
    {
        DateToStringConverter() = default;

        Windows::Foundation::IInspectable Convert(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language);
        Windows::Foundation::IInspectable ConvertBack(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language);
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct DateToStringConverter : DateToStringConverterT<DateToStringConverter, implementation::DateToStringConverter>
    {
    };
}
