#include "pch.h"
#include "EmptyStringToDefaultConverter.h"
#include "EmptyStringToDefaultConverter.g.cpp"

namespace winrt::MediaPlayer::implementation
{
    Windows::Foundation::IInspectable EmptyStringToDefaultConverter::Convert(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language)
    {
        auto strVal = unbox_value<hstring>(value);
        auto defaultVal = unbox_value_or<hstring>(parameter, L"[Default value]");
        return strVal.empty() ? box_value(defaultVal) : value;
    }
    Windows::Foundation::IInspectable EmptyStringToDefaultConverter::ConvertBack(Windows::Foundation::IInspectable const& value, Windows::UI::Xaml::Interop::TypeName const& targetType, Windows::Foundation::IInspectable const& parameter, hstring const& language)
    {
        throw hresult_not_implemented();
    }
}
