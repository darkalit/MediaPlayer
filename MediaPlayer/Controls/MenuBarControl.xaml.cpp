#include "pch.h"
#include "MenuBarControl.xaml.h"
#if __has_include("MenuBarControl.g.cpp")
#include "MenuBarControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::MediaPlayer::implementation
{
    MenuBarControl::MenuBarControl()
    {
        m_ViewModel = make<MediaPlayer::implementation::MenuBarControlViewModel>();
        DataContext(ViewModel());

        ViewModel().SubTracks().VectorChanged({ this, &MenuBarControl::OnSubTracksVectorChanged });
    }

    MediaPlayer::MenuBarControlViewModel MenuBarControl::ViewModel()
    {
        return m_ViewModel;
    }

    void MenuBarControl::OnSubTracksVectorChanged(
        winrt::Windows::Foundation::Collections::IObservableVector<SubtitleStream> const& sender,
        winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
    {
        MenuSubItem_SubTrack().Items().Clear();

        Controls::RadioMenuFlyoutItem radioItem;
        radioItem.Text(L"None");
        radioItem.GroupName(L"SubTracksGroup");
        radioItem.Command(ViewModel().ChangeSubTrack());
        radioItem.CommandParameter(box_value(-1));

        MenuSubItem_SubTrack().Items().Append(radioItem);

        for (auto const& s : ViewModel().SubTracks())
        {
            Controls::RadioMenuFlyoutItem radioItem;
            radioItem.Text(s.Title + L" - [" + s.Language + L"]" );
            radioItem.GroupName(L"SubTracksGroup");
            radioItem.Command(ViewModel().ChangeSubTrack());
            radioItem.CommandParameter(box_value(s.Index));

            MenuSubItem_SubTrack().Items().Append(radioItem);
        }
    }
}
