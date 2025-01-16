#include "pch.h"
#include "App.xaml.h"

#include "microsoft.ui.xaml.window.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::MediaPlayer::implementation
{
    Window App::s_Window = nullptr;

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

    HWND App::GetMainWindow()
    {
        auto windowNative = s_Window.try_as<IWindowNative>();
        HWND hwnd = 0;
        windowNative->get_WindowHandle(&hwnd);
        return hwnd;
    }
}
