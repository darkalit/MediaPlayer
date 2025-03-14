#pragma once

#include "PiPWindow.g.h"
#include "ViewModels/PiPWindowViewModel.h"

namespace winrt::MediaPlayer::implementation
{
    struct PiPWindow : PiPWindowT<PiPWindow>
    {
        PiPWindow();

        MediaPlayer::PiPWindowViewModel ViewModel();

        void OnLoad(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void SwapChainPanel_Video_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);

    private:
        MediaPlayer::PiPWindowViewModel m_ViewModel = nullptr;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct PiPWindow : PiPWindowT<PiPWindow, implementation::PiPWindow>
    {
    };
}
