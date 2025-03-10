#include "pch.h"
#include "RecorderWindow.xaml.h"
#if __has_include("RecorderWindow.g.cpp")
#include "RecorderWindow.g.cpp"
#endif

#include "microsoft.ui.xaml.window.h"
#include "winrt/Microsoft.UI.Windowing.h"
#include "winrt/Microsoft.UI.Interop.h"

#include "App.xaml.h"
#include "ViewModels/RecorderViewModel.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::MediaPlayer::implementation
{
    RecorderWindow::RecorderWindow()
    {
        m_ViewModel = make<MediaPlayer::implementation::RecorderViewModel>();

        HWND hwnd;
        auto nativeWindow = try_as<IWindowNative>();
        nativeWindow->get_WindowHandle(&hwnd);


        Microsoft::UI::WindowId windowId = Microsoft::UI::GetWindowIdFromWindow(hwnd);
        auto appWindow = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);
        appWindow.Resize(Windows::Graphics::SizeInt32(500, 140));
    }

    MediaPlayer::RecorderViewModel RecorderWindow::ViewModel()
    {
        return m_ViewModel;
    }
}
