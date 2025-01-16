#pragma once
#include "DelegateCommand.g.h"

#include "functional"

namespace winrt::MediaPlayer::implementation
{
    struct DelegateCommand : DelegateCommandT<DelegateCommand>
    {
        DelegateCommand(std::function<void(Windows::Foundation::IInspectable)> const& execute);
        DelegateCommand(std::function<void(Windows::Foundation::IInspectable)> const& execute, std::function<bool(Windows::Foundation::IInspectable)> const& canExecute);

        void RaiseCanExecuteChanged();
        event_token CanExecuteChanged(Windows::Foundation::EventHandler<Windows::Foundation::IInspectable> const& handler);
        void CanExecuteChanged(event_token const& token) noexcept;
        bool CanExecute(Windows::Foundation::IInspectable const& parameter);
        void Execute(Windows::Foundation::IInspectable const& parameter);

    private:
        event<Windows::Foundation::EventHandler<Windows::Foundation::IInspectable>> m_CanExecuteChanged;

        std::function<void(Windows::Foundation::IInspectable)> m_ExecuteDelegate;
        std::function<bool(Windows::Foundation::IInspectable)> m_CanExecuteDelegate;
        bool m_LastCanExecute = true;
    };
}
