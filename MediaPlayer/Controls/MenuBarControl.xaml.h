#pragma once

#include "MenuBarControl.g.h"
#include "ViewModels/MenuBarControlViewModel.h"

namespace winrt::MediaPlayer::implementation
{
    struct MenuBarControl : MenuBarControlT<MenuBarControl>
    {
        MenuBarControl();
        MediaPlayer::MenuBarControlViewModel ViewModel();

    private:
        MediaPlayer::MenuBarControlViewModel m_ViewModel = nullptr;
    };
}

namespace winrt::MediaPlayer::factory_implementation
{
    struct MenuBarControl : MenuBarControlT<MenuBarControl, implementation::MenuBarControl>
    {
    };
}
