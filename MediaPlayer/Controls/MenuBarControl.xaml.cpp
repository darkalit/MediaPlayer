#include "pch.h"
#include "MenuBarControl.xaml.h"
#if __has_include("MenuBarControl.g.cpp")
#include "MenuBarControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::MediaPlayer::implementation
{
    MenuBarControl::MenuBarControl()
    {
        m_ViewModel = make<MediaPlayer::implementation::MenuBarControlViewModel>();
        DataContext(ViewModel());
    }

    MediaPlayer::MenuBarControlViewModel MenuBarControl::ViewModel()
    {
        return m_ViewModel;
    }
}
