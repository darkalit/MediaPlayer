#include "pch.h"
#include "DelegateCommand.h"
#include "DelegateCommand.g.cpp"

using namespace winrt::Windows::Foundation;

namespace winrt::MediaPlayer::implementation
{
    DelegateCommand::DelegateCommand(std::function<void(IInspectable)> const& execute)
        : DelegateCommand(execute, nullptr)
    {
    }

    DelegateCommand::DelegateCommand(std::function<void(IInspectable)> const& execute,
        std::function<bool(IInspectable)> const& canExecute)
    {
        if (execute == nullptr)
        {
            throw hresult_invalid_argument(L"DelegateCommand::execute");
        }

        m_ExecuteDelegate = execute;
        m_CanExecuteDelegate = canExecute;
    }

    void DelegateCommand::RaiseCanExecuteChanged()
    {
        m_CanExecuteChanged(*this, IInspectable());
    }

    event_token DelegateCommand::CanExecuteChanged(EventHandler<IInspectable> const& handler)
    {
        return m_CanExecuteChanged.add(handler);
    }

    void DelegateCommand::CanExecuteChanged(event_token const& token) noexcept
    {
        m_CanExecuteChanged.remove(token);
    }

    bool DelegateCommand::CanExecute(IInspectable const& parameter)
    {
        if (m_CanExecuteDelegate == nullptr)
        {
            return true;
        }

        bool canExecute = m_CanExecuteDelegate(parameter);

        if (m_LastCanExecute != canExecute)
        {
            m_LastCanExecute = canExecute;
            RaiseCanExecuteChanged();
        }

        return m_LastCanExecute;
    }

    void DelegateCommand::Execute(IInspectable const& parameter)
    {
        if (m_ExecuteDelegate != nullptr)
        {
            m_ExecuteDelegate(parameter);
        }
    }
}
