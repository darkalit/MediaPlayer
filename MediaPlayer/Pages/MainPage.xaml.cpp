#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include <stdlib.h>

#include "winrt/Microsoft.UI.Input.h"

#include "App.xaml.h"
#include "Utils.h"
#include "Media/FfmpegDecoder.h"

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

    void MainPage::Slider_Timeline_PointerEntered(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const&)
    {
        PreviewPopup().IsOpen(true);
    }

    void MainPage::Slider_Timeline_PointerMoved(Windows::Foundation::IInspectable const& sender, PointerRoutedEventArgs const& e)
    {
        auto slider = sender.as<Controls::Slider>();
        Point point = e.GetCurrentPoint(slider).Position();

        double relativePosition = point.X / slider.ActualWidth();
        double sliderValue = slider.Minimum() + (slider.Maximum() - slider.Minimum()) * relativePosition;

        PreviewTimeText().Text(Utils::DurationToString(sliderValue * 1000.0f));

        auto transform = slider.TransformToVisual(LayoutRoot());
        auto screenPoint = transform.TransformPoint(point);

        double offsetX = screenPoint.X - PreviewPopup().ActualWidth() / 2;

        PreviewPopup().HorizontalOffset(offsetX);
        PreviewPopup().VerticalOffset(point.Y - PreviewPopup().ActualHeight() - 5);

        FfmpegDecoder::GetFrame(ViewModel().Path(), sliderValue * 1000.0f);
    }

    void MainPage::Slider_Timeline_PointerExited(Windows::Foundation::IInspectable const&, PointerRoutedEventArgs const& e)
    {
        PreviewPopup().IsOpen(false);
    }

    MediaPlayer::MainPageViewModel MainPage::ViewModel()
    {
        return m_ViewModel;
    }
}
