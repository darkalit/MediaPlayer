#pragma once
#include "InternetResourceViewModel.g.h"
#include "Framework/BindableBase.h"
#include "Framework/DelegateCommand.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct InternetResourceViewModel : InternetResourceViewModelT<InternetResourceViewModel, MediaPlayer::implementation::BindableBase>
    {
        InternetResourceViewModel();

        hstring ErrorMessage();
        void ErrorMessage(hstring const& value);

        Microsoft::UI::Xaml::Input::ICommand LoadResource();

    private:
        MediaPlayer::PlayerService m_PlayerService;

        hstring m_ErrorMessage;

        MediaPlayer::DelegateCommand m_LoadResourceCommand = nullptr;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct InternetResourceViewModel : InternetResourceViewModelT<InternetResourceViewModel, implementation::InternetResourceViewModel>
    {
    };
}
