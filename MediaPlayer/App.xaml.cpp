#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"

#include "microsoft.ui.xaml.window.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::MediaPlayer::implementation
{
    winrt::Microsoft::UI::Xaml::Window App::s_Window = nullptr;

    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    App::App()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

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

    /// <summary>
    /// Invoked when the application is launched.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {
        s_Window = make<MainWindow>();
        Frame rootFrame = CreateRootFrame();
        if (!rootFrame.Content())
        {
            rootFrame.Navigate(xaml_typename<MediaPlayer::MainPage>());
        }
        s_Window.Activate();
    }

    void App::OnNavigationFailed(const IInspectable, const Microsoft::UI::Xaml::Navigation::INavigationFailedEventArgs e)
    {
        throw hresult_error(E_FAIL, hstring(L"Failed to load page ") + e.SourcePageType().Name);
    }

    HWND App::GetMainWindow()
    {
        auto windowNative = App::s_Window.try_as<::IWindowNative>();
        HWND hwnd = 0;
        windowNative->get_WindowHandle(&hwnd);
        return hwnd;
    }

    Microsoft::UI::Xaml::Controls::Frame App::CreateRootFrame()
    {
        Frame rootFrame = nullptr;
        auto content = s_Window.Content();
        if (content)
        {
            rootFrame = content.try_as<Frame>();
        }

        if (!rootFrame)
        {
            rootFrame = Frame();
            rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });
            s_Window.Content(rootFrame);
        }

        return rootFrame;
    }
}
