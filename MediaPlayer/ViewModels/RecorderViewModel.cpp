#include "pch.h"
#include "RecorderViewModel.h"
#include "RecorderViewModel.g.cpp"

#include "App.xaml.h"
#include "Utils.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI;

namespace winrt::MediaPlayer::implementation
{
    RecorderViewModel::RecorderViewModel()
        : m_PlayerService(App::GetPlayerService())
    {
        m_RecordSegmentCommand = make<DelegateCommand>([this](auto&&)
        {
            uint64_t millisStart = ((m_HoursStart * 60 + m_MinutesStart) * 60 + m_SecondsStart) * 1000;
            uint64_t millisEnd = ((m_HoursEnd * 60 + m_MinutesEnd) * 60 + m_SecondsEnd) * 1000;

            if (millisStart == millisEnd)
            {
                ErrorMessage(L"Start point cannot be equal to end point");
                return;
            }

            if (millisStart > millisEnd)
            {
                ErrorMessage(L"Start point cannot be further than end point");
                return;
            }

            if (millisEnd > m_PlayerService.Metadata().Duration)
            {
                ErrorMessage(L"End point cannot be further than media duration");
                return;
            }

            m_PlayerService.RecordSegment(millisStart, millisEnd);

            ErrorMessage(L"");
        });
    }

    double RecorderViewModel::HoursStart()
    {
        return m_HoursStart;
    }

    void RecorderViewModel::HoursStart(double value)
    {
        if (m_HoursStart != value)
        {
            m_HoursStart = value;
            RaisePropertyChanged(L"HoursStart");
        }
    }

    double RecorderViewModel::MinutesStart()
    {
        return m_MinutesStart;
    }

    void RecorderViewModel::MinutesStart(double value)
    {
        if (m_MinutesStart != value)
        {
            m_MinutesStart = value;
            RaisePropertyChanged(L"MinutesStart");
        }
    }

    double RecorderViewModel::SecondsStart()
    {
        return m_SecondsStart;
    }

    void RecorderViewModel::SecondsStart(double value)
    {
        if (m_SecondsStart != value)
        {
            m_SecondsStart = value;
            RaisePropertyChanged(L"SecondsStart");
        }
    }

    double RecorderViewModel::HoursEnd()
    {
        return m_HoursEnd;
    }

    void RecorderViewModel::HoursEnd(double value)
    {
        if (m_HoursEnd != value)
        {
            m_HoursEnd = value;
            RaisePropertyChanged(L"HoursEnd");
        }
    }

    double RecorderViewModel::MinutesEnd()
    {
        return m_MinutesEnd;
    }

    void RecorderViewModel::MinutesEnd(double value)
    {
        if (m_MinutesEnd != value)
        {
            m_MinutesEnd = value;
            RaisePropertyChanged(L"MinutesEnd");
        }
    }

    double RecorderViewModel::SecondsEnd()
    {
        return m_SecondsEnd;
    }

    void RecorderViewModel::SecondsEnd(double value)
    {
        if (m_SecondsEnd != value)
        {
            m_SecondsEnd = value;
            RaisePropertyChanged(L"SecondsEnd");
        }
    }

    hstring RecorderViewModel::ErrorMessage()
    {
        return m_ErrorMessage;
    }

    void RecorderViewModel::ErrorMessage(hstring value)
    {
        if (m_ErrorMessage != value)
        {
            m_ErrorMessage = value;
            RaisePropertyChanged(L"ErrorMessage");
        }
    }

    Xaml::Input::ICommand RecorderViewModel::Record()
    {
        return m_RecordSegmentCommand;
    }
}
