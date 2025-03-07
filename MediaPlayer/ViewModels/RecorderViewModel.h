#pragma once
#include "RecorderViewModel.g.h"
#include "Framework/DelegateCommand.h"
#include "Framework/BindableBase.h"
#include "Services/PlayerService.h"

namespace winrt::MediaPlayer::implementation
{
    struct RecorderViewModel : RecorderViewModelT<RecorderViewModel, MediaPlayer::implementation::BindableBase>
    {
        RecorderViewModel();

        double HoursStart();
        void HoursStart(double value);
        double MinutesStart();
        void MinutesStart(double value);
        double SecondsStart();
        void SecondsStart(double value);
        double HoursEnd();
        void HoursEnd(double value);
        double MinutesEnd();
        void MinutesEnd(double value);
        double SecondsEnd();
        void SecondsEnd(double value);

        hstring ErrorMessage();
        void ErrorMessage(hstring value);

        Microsoft::UI::Xaml::Input::ICommand Record();

    private:
        MediaPlayer::PlayerService m_PlayerService;

        double m_HoursStart;
        double m_MinutesStart;
        double m_SecondsStart;
        double m_HoursEnd;
        double m_MinutesEnd;
        double m_SecondsEnd;

        hstring m_ErrorMessage;

        MediaPlayer::DelegateCommand m_RecordSegmentCommand = nullptr;
    };
}
namespace winrt::MediaPlayer::factory_implementation
{
    struct RecorderViewModel : RecorderViewModelT<RecorderViewModel, implementation::RecorderViewModel>
    {
    };
}
