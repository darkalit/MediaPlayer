#pragma once
#include "IsSelectedColorConverter.g.h"

namespace winrt::MediaPlayer::implementation
{
    struct IsSelectedColorConverter : IsSelectedColorConverterT<IsSelectedColorConverter>
    {
        IsSelectedColorConverter() = default;

        Windows::Foundation::IInspectable Convert(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language);
        Windows::Foundation::IInspectable ConvertBack(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language);
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct IsSelectedColorConverter : IsSelectedColorConverterT<IsSelectedColorConverter, implementation::IsSelectedColorConverter>
    {
    };
}
