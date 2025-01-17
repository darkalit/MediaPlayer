#include "pch.h"
#include "DateToStringConverter.h"
#include "DateToStringConverter.g.cpp"

namespace winrt::MediaPlayer::implementation
{
    winrt::Windows::Foundation::IInspectable DateToStringConverter::Convert(winrt::Windows::Foundation::IInspectable const& value, winrt::Windows::UI::Xaml::Interop::TypeName const& targetType, winrt::Windows::Foundation::IInspectable const& parameter, hstring const& language)
    {
        auto datetime = unbox_value<Windows::Foundation::DateTime>(value);
        auto tt = clock::to_time_t(datetime);
        std::wstringstream ss;

        std::tm tm;
        gmtime_s(&tm, &tt);
        ss << std::put_time(&tm, L"%F %T");

        return box_value(hstring(ss.str()));
    }
    winrt::Windows::Foundation::IInspectable DateToStringConverter::ConvertBack(winrt::Windows::Foundation::IInspectable const& value, winrt::Windows::UI::Xaml::Interop::TypeName const& targetType, winrt::Windows::Foundation::IInspectable const& parameter, hstring const& language)
    {
        throw hresult_not_implemented();
    }
}
