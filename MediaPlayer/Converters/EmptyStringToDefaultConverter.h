#pragma once
#include "EmptyStringToDefaultConverter.g.h"

namespace winrt::MediaPlayer::implementation
{
    struct EmptyStringToDefaultConverter : EmptyStringToDefaultConverterT<EmptyStringToDefaultConverter>
    {
        EmptyStringToDefaultConverter() = default;

        Windows::Foundation::IInspectable Convert(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language);
        Windows::Foundation::IInspectable ConvertBack(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language);
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct EmptyStringToDefaultConverter : EmptyStringToDefaultConverterT<EmptyStringToDefaultConverter, implementation::EmptyStringToDefaultConverter>
    {
    };
}
