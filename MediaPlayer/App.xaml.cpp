#include "pch.h"
#include "App.xaml.h"

#include "microsoft.ui.xaml.window.h"

#include "Pages/PiPWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::MediaPlayer::implementation
{
    Window App::s_Window = nullptr;
    MediaPlayer::PiPWindow App::s_PiPWindow = nullptr;
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

    MediaPlayer::PiPWindow App::OpenPiPWindow()
    {
        if (!s_PiPWindow)
        {
            s_PiPWindow = make<implementation::PiPWindow>();
            s_PiPWindow.Closed([&](auto&&, auto&&)
            {
                s_PiPWindow = nullptr;
            });
        }

        s_PiPWindow.Activate();
        HideWindow(s_Window, true);

        return s_PiPWindow;
    }

    Microsoft::UI::Xaml::Window App::OpenMainWindow()
    {
        HideWindow(s_Window, false);
        s_PiPWindow.Close();
        return s_Window;
    }

    void App::HideWindow(Microsoft::UI::Xaml::Window const& window, bool hide)
    {
        auto windowNative = window.try_as<IWindowNative>();

        HWND hwnd;
        windowNative->get_WindowHandle(&hwnd);

        ShowWindow(hwnd, hide ? SW_HIDE : SW_SHOW);
    }
}
