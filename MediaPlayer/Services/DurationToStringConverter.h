#pragma once
#include "DurationToStringConverter.g.h"

namespace winrt::MediaPlayer::implementation
{
    struct DurationToStringConverter : DurationToStringConverterT<DurationToStringConverter>
    {
        DurationToStringConverter() = default;

        Windows::Foundation::IInspectable Convert(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language);
        Windows::Foundation::IInspectable ConvertBack(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language);
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct DurationToStringConverter : DurationToStringConverterT<DurationToStringConverter, implementation::DurationToStringConverter>
    {
    };
}
