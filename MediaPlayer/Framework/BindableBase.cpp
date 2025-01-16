#include "pch.h"
#include "BindableBase.h"
#include "BindableBase.g.cpp"

using namespace winrt::Microsoft::UI::Xaml::Data;

namespace winrt::MediaPlayer::implementation
{
    void BindableBase::RaisePropertyChanged(hstring const& properyName)
    {
        m_PropertyChanged(*this, PropertyChangedEventArgs(properyName));
    }
    event_token BindableBase::PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_PropertyChanged.add(handler);
    }
    void BindableBase::PropertyChanged(event_token const& token) noexcept
    {
        m_PropertyChanged.remove(token);
    }
}
