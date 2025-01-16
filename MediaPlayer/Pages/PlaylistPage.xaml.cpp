#include "pch.h"
#include "PlaylistPage.xaml.h"
#if __has_include("PlaylistPage.g.cpp")
#include "PlaylistPage.g.cpp"
#endif

#include "App.xaml.h"
#include "ViewModels/PlaylistViewModel.h"
#include "windows.foundation.collections.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Xaml;
using namespace Xaml::Input;
using namespace Windows::Foundation;
using namespace Collections;

namespace winrt::MediaPlayer::implementation
{
    PlaylistPage::PlaylistPage()
    {
        m_ViewModel = make<MediaPlayer::implementation::PlaylistViewModel>();
        DataContext(ViewModel());
    }

    void PlaylistPage::OnLoad(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_ViewModel.SetSwapChain().Execute(SwapChainPanel_Video());

        Slider_Timeline().AddHandler(
            UIElement::PointerPressedEvent(),
            box_value(PointerEventHandler{ this, &PlaylistPage::Slider_Timeline_PointerPressed }),
            true);
        Slider_Timeline().AddHandler(
            UIElement::PointerReleasedEvent(),
            box_value(PointerEventHandler{ this, &PlaylistPage::Slider_Timeline_PointerReleased }),
            true);
    }

    void PlaylistPage::SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, SizeChangedEventArgs const&)
    {
        m_ViewModel.ResizeVideo().Execute(box_value(SwapChainPanel_Video().ActualSize()));
    }

    void PlaylistPage::ItemsView_Playlist_ItemInvoked(Controls::ItemsView const& sender, Controls::ItemsViewItemInvokedEventArgs const&)
    {
        auto index = sender.CurrentItemIndex();
        m_ViewModel.PlayMediaByIndex().Execute(box_value(index));
    }

    void PlaylistPage::Button_DeleteItem_Click(Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&)
    {
        auto button = sender.as<Controls::Button>();
        auto id = button.DataContext().try_as<guid>();

        if (id)
        {
            m_ViewModel.DeleteMedia().Execute(box_value(*id));
        }
    }

    void PlaylistPage::Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        auto time = static_cast<uint64_t>(ViewModel().CurrentTimeValue()) * 1000;
        ViewModel().Play().Execute(box_value(time));
    }

    void PlaylistPage::Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        ViewModel().Pause().Execute(IInspectable());
    }

    MediaPlayer::PlaylistViewModel PlaylistPage::ViewModel()
    {
        return m_ViewModel;
    }
}
