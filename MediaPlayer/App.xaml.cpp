#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"

#include "microsoft.ui.xaml.window.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::MediaPlayer::implementation
{
    winrt::Microsoft::UI::Xaml::Window App::s_Window = nullptr;
    std::shared_ptr<PlayerService> App::m_PlayerService = nullptr;

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
        m_PlayerService = std::make_shared<PlayerService>();
        m_PlayerService->Init();

        s_Window = make<MainWindow>();
        auto mainWindow = s_Window.try_as<MainWindow>();
        mainWindow->Navigate<MainPage>();
        s_Window.Activate();
    }

    std::shared_ptr<PlayerService> App::GetPlayerService()
    {
        return m_PlayerService;
    }

    HWND App::GetMainWindow()
    {
        auto windowNative = App::s_Window.try_as<::IWindowNative>();
        HWND hwnd = 0;
        windowNative->get_WindowHandle(&hwnd);
        return hwnd;
    }
}
