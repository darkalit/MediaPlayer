#include "pch.h"
#include "InternetResourceViewModel.h"
#include "InternetResourceViewModel.g.cpp"

#include "App.xaml.h"

namespace winrt::MediaPlayer::implementation
{
    InternetResourceViewModel::InternetResourceViewModel()
        : m_PlayerService(App::GetPlayerService())
    {
        m_LoadResourceCommand = make<DelegateCommand>([&](IInspectable const& parameter)
        {
            auto url = unbox_value<hstring>(parameter);

            OutputDebugString(url.c_str());
            OutputDebugString(L"\n");
        });
    }

    Microsoft::UI::Xaml::Input::ICommand InternetResourceViewModel::LoadResource()
    {
        return m_LoadResourceCommand;
    }
}
