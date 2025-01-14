#include "pch.h"
#include "DurationToStringConverter.h"
#include "DurationToStringConverter.g.cpp"

#include "Utils.h"

namespace winrt::MediaPlayer::implementation
{
    Windows::Foundation::IInspectable DurationToStringConverter::Convert(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language)
    {
        auto duration = unbox_value_or<unsigned long long>(value, 0.0);

        return box_value(Utils::DurationToString(duration));
    }

    Windows::Foundation::IInspectable DurationToStringConverter::ConvertBack(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language)
    {
        throw hresult_not_implemented();
    }
}
