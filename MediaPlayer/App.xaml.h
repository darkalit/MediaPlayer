#pragma once

#include "App.xaml.g.h"
#include "Converters/DurationToStringConverter.h"
#include "Converters/EmptyStringToDefaultConverter.h"
#include "Converters/IsSelectedColorConverter.h"
#include "Converters/DateToStringConverter.h"
#include "MainWindow.xaml.h"
#include "DirectX/DeviceResources.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

        static MediaPlayer::PlayerService GetPlayerService();
        static std::shared_ptr<DeviceResources> GetDeviceResources();
        static HWND GetMainWindow();
        static Microsoft::UI::Xaml::Window OpenPiPWindow();
        static Microsoft::UI::Xaml::Window OpenInternetResourceWindow();
        static Microsoft::UI::Xaml::Window OpenRecorderWindow();
        static Microsoft::UI::Xaml::Window OpenMainWindow();
        static void HideWindow(Microsoft::UI::Xaml::Window const& window, bool hide);

        template <typename T>
        static void Navigate();

        template <typename T>
        static void Navigate(IInspectable const& parameter);

    private:
        static Microsoft::UI::Xaml::Window s_Window;
        static Microsoft::UI::Xaml::Window s_PiPWindow;
        static Microsoft::UI::Xaml::Window s_InternetResourceWindow;
        static Microsoft::UI::Xaml::Window s_RecorderWindow;
        static std::shared_ptr<DeviceResources> s_DeviceResources;
    };

    template <typename T>
    void App::Navigate()
    {
        auto mainWindow = s_Window.try_as<MainWindow>();
        mainWindow->Navigate<T>();
    }

    template <typename T>
    void App::Navigate(IInspectable const& parameter)
    {
        auto mainWindow = s_Window.try_as<MainWindow>();
        mainWindow->Navigate<T>(parameter);
    }
}
