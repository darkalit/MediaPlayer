#include "pch.h"
#include "App.xaml.h"

#include "microsoft.ui.xaml.window.h"

#include "Pages/PiPWindow.xaml.h"
#include "Pages/InternetResourceWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::MediaPlayer::implementation
{
    Window App::s_Window = nullptr;
    Window App::s_PiPWindow = nullptr;
    Window App::s_InternetResourceWindow = nullptr;
    Window App::s_RecorderWindow = nullptr;
    std::shared_ptr<DeviceResources> App::s_DeviceResources = nullptr;

    App::App()
    {
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            if (IsDebuggerPresent())
            {
                auto errorMessage = e.Message();
                __debugbreak();
            }
        });
#endif
    }

    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {
        s_DeviceResources = std::make_shared<DeviceResources>();

        GetPlayerService().Init();

        s_Window = make<MainWindow>();

        s_Window.Closed([&](auto&&, auto&&)
        {
            if (s_InternetResourceWindow) s_InternetResourceWindow.Close();
            if (s_RecorderWindow) s_RecorderWindow.Close();
        });

        auto mainWindow = s_Window.try_as<MainWindow>();
        mainWindow->Navigate<MainPage>();
        s_Window.Activate();

        GetPlayerService().UIDispatcher(s_Window.DispatcherQueue());
    }

    MediaPlayer::PlayerService App::GetPlayerService()
    {
        return Application::Current().Resources().TryLookup(box_value(L"PlayerService")).as<MediaPlayer::PlayerService>();
    }

    std::shared_ptr<DeviceResources> App::GetDeviceResources()
    {
        return s_DeviceResources;
    }

    HWND App::GetMainWindow()
    {
        auto windowNative = s_Window.try_as<IWindowNative>();
        HWND hwnd = 0;
        windowNative->get_WindowHandle(&hwnd);
        return hwnd;
    }

    Window App::OpenPiPWindow()
    {
        if (!s_PiPWindow)
        {
            s_PiPWindow = make<PiPWindow>();
            s_PiPWindow.Closed([&](auto&&, auto&&)
            {
                s_PiPWindow = nullptr;
                HideWindow(s_Window, false);
            });
        }

        s_PiPWindow.Activate();
        HideWindow(s_Window, true);

        return s_PiPWindow;
    }

    Window App::OpenInternetResourceWindow()
    {
        if (!s_InternetResourceWindow)
        {
            s_InternetResourceWindow = make<InternetResourceWindow>();
            s_InternetResourceWindow.Closed([&](auto&&, auto&&) {
                s_InternetResourceWindow = nullptr;
            });
        }

        s_InternetResourceWindow.Activate();

        return s_InternetResourceWindow;
    }

    Window App::OpenRecorderWindow()
    {
        if (!s_RecorderWindow)
        {
            s_RecorderWindow = make<InternetResourceWindow>();
            s_RecorderWindow.Closed([&](auto&&, auto&&) {
                s_RecorderWindow = nullptr;
            });
        }

        s_RecorderWindow.Activate();

        return s_RecorderWindow;
    }

    Window App::OpenMainWindow()
    {
        HideWindow(s_Window, false);
        s_PiPWindow.Close();
        return s_Window;
    }

    void App::HideWindow(Window const& window, bool hide)
    {
        auto windowNative = window.try_as<IWindowNative>();

        HWND hwnd;
        windowNative->get_WindowHandle(&hwnd);

        ShowWindow(hwnd, hide ? SW_HIDE : SW_SHOW);
    }
}
