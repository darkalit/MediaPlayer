#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include "App.xaml.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Xaml;
using namespace Xaml::Input;
using namespace Windows::Foundation;

namespace winrt::MediaPlayer::implementation
{
    MainPage::MainPage()
    {
        m_ViewModel = make<MediaPlayer::implementation::MainPageViewModel>();
        DataContext(ViewModel());
        //KeyDown(KeyEventHandler{ this, &MainPage::OnKeyDown });
    }

    void MainPage::OnLoad(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        this->Focus(FocusState::Programmatic);

        ViewModel().SetSwapChain().Execute(SwapChainPanel_Video());

        Slider_Timeline().AddHandler(
            UIElement::PointerPressedEvent(),
            box_value(PointerEventHandler{ this, &MainPage::Slider_Timeline_PointerPressed }),
            true);
        Slider_Timeline().AddHandler(
            UIElement::PointerReleasedEvent(),
            box_value(PointerEventHandler{ this, &MainPage::Slider_Timeline_PointerReleased }),
            true);
    }

    void MainPage::Up_Invoked(Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender,
        Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args)
    {
        ViewModel().VolumeValue(ViewModel().VolumeValue() + 5);
        args.Handled(true);
    }

    void MainPage::Down_Invoked(Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender,
        Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args)
    {
        ViewModel().VolumeValue(ViewModel().VolumeValue() - 5);
        args.Handled(true);
    }

    void MainPage::Left_Invoked(Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender,
        Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args)
    {
        ViewModel().Pause().Execute(IInspectable());
        ViewModel().CurrentTimeValue(ViewModel().CurrentTimeValue() - 5);
        auto time = static_cast<uint64_t>(ViewModel().CurrentTimeValue()) * 1000;
        ViewModel().Play().Execute(box_value(time));
        args.Handled(true);
    }

    void MainPage::Right_Invoked(Microsoft::UI::Xaml::Input::KeyboardAccelerator const& sender,
        Microsoft::UI::Xaml::Input::KeyboardAcceleratorInvokedEventArgs const& args)
    {
        ViewModel().Pause().Execute(IInspectable());
        ViewModel().CurrentTimeValue(ViewModel().CurrentTimeValue() + 5);
        auto time = static_cast<uint64_t>(ViewModel().CurrentTimeValue()) * 1000;
        ViewModel().Play().Execute(box_value(time));
        args.Handled(true);
    }

    void MainPage::SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, SizeChangedEventArgs const&)
    {
        ViewModel().ResizeVideo().Execute(box_value(SwapChainPanel_Video().ActualSize()));
    }

    void MainPage::Slider_Timeline_PointerReleased(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        auto time = static_cast<uint64_t>(ViewModel().CurrentTimeValue()) * 1000;
        ViewModel().Play().Execute(box_value(time));
    }

    void MainPage::Slider_Timeline_PointerPressed(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        ViewModel().Pause().Execute(IInspectable());
    }

    MediaPlayer::MainPageViewModel MainPage::ViewModel()
    {
        return m_ViewModel;
    }
}
