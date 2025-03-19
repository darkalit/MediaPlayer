#include "pch.h"
#include "InternetResourceWindow.xaml.h"

#if __has_include("InternetResourceWindow.g.cpp")
#include "InternetResourceWindow.g.cpp"
#endif

#include "microsoft.ui.xaml.window.h"
#include "winrt/Microsoft.UI.Windowing.h"
#include "winrt/Microsoft.UI.Interop.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::MediaPlayer::implementation
{
    InternetResourceWindow::InternetResourceWindow()
    {
        m_ViewModel = make<MediaPlayer::implementation::InternetResourceViewModel>();

        HWND hwnd;
        auto nativeWindow = try_as<IWindowNative>();
        nativeWindow->get_WindowHandle(&hwnd);


        Microsoft::UI::WindowId windowId = Microsoft::UI::GetWindowIdFromWindow(hwnd);
        auto appWindow = Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);
        appWindow.Resize(Windows::Graphics::SizeInt32(500, 180));
    }

    MediaPlayer::InternetResourceViewModel InternetResourceWindow::ViewModel()
    {
        return m_ViewModel;
    }
}
