#include "pch.h"
#include "InternetResourceViewModel.h"
#include "InternetResourceViewModel.g.cpp"

#include "App.xaml.h"
#include "Media/MediaUrlGetter.h"

namespace winrt::MediaPlayer::implementation
{
    InternetResourceViewModel::InternetResourceViewModel()
        : m_PlayerService(App::GetPlayerService())
    {
        m_LoadResourceCommand = make<DelegateCommand>([&](IInspectable const& parameter) -> fire_and_forget
        {
            ErrorMessage(L"");

            auto url = unbox_value<hstring>(parameter);

            OutputDebugString(url.c_str());
            OutputDebugString(L"\n");

            switch(MediaUrlGetter::GetType(url))
            {
            case WebResourceType::Unknown:
                ErrorMessage(L"This web resource cannot be used");
                co_return;

            default:
                break;
            }

            bool isAvailable = co_await m_PlayerService.ResourceIsAvailable(url);
            if (!isAvailable)
            {
                ErrorMessage(L"Resource cannot be accessed");
            }

            m_PlayerService.AddSourceFromUrl(url);
        });
    }

    hstring InternetResourceViewModel::ErrorMessage()
    {
        return m_ErrorMessage;
    }

    void InternetResourceViewModel::ErrorMessage(hstring const& value)
    {
        if (m_ErrorMessage != value)
        {
            m_ErrorMessage = value;
            RaisePropertyChanged(L"ErrorMessage");
        }
    }

    Microsoft::UI::Xaml::Input::ICommand InternetResourceViewModel::LoadResource()
    {
        return m_LoadResourceCommand;
    }
}
