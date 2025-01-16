#pragma once

#include "MainWindow.g.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        template <typename T>
        void Navigate();

        template <typename T>
        void Navigate(IInspectable const& parameter);
    };

    template <typename T>
    void MainWindow::Navigate()
    {
        PageFrame().Navigate(xaml_typename<T>());
    }

    template <typename T>
    void MainWindow::Navigate(IInspectable const& parameter)
    {
        PageFrame().Navigate(xaml_typename<T>(), parameter);
    }
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
