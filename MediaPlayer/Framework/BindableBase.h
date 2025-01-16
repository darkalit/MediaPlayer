#pragma once
#include "BindableBase.g.h"

namespace winrt::MediaPlayer::implementation
{
    struct BindableBase : BindableBaseT<BindableBase>
    {
        BindableBase() = default;

        void RaisePropertyChanged(hstring const& properyName);
        event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(event_token const& token) noexcept;

    private:
        event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_PropertyChanged;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct BindableBase : BindableBaseT<BindableBase, implementation::BindableBase>
    {
    };
}