#include "pch.h"
#include "PiPWindow.xaml.h"
#if __has_include("PiPWindow.g.cpp")
#include "PiPWindow.g.cpp"
#endif

#include "microsoft.ui.xaml.window.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::MediaPlayer::implementation
{
    PiPWindow::PiPWindow()
    {
        m_ViewModel = make<PiPWindowViewModel>();

        auto windowNative = try_as<IWindowNative>();
        HWND hwnd;

        windowNative->get_WindowHandle(&hwnd);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 640, 360, SWP_NOACTIVATE);
    }

    MediaPlayer::PiPWindowViewModel PiPWindow::ViewModel()
    {
        return m_ViewModel;
    }
}
