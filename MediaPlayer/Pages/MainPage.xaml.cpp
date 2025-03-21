#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include <stdlib.h>

#include "winrt/Microsoft.UI.Input.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Graphics.Imaging.h"
#include "winrt/Windows.UI.Xaml.Media.Imaging.h"

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
        ViewModel().ResizeVideo().Execute(box_value(SwapChainPanel_Video().ActualSize()));

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

    fire_and_forget MainPage::Slider_Timeline_PointerMoved(Windows::Foundation::IInspectable const& sender, PointerRoutedEventArgs const& e)
    {
        static double lastSliderValue = 0.0;
        int imageHeight = 120;

        if (!ViewModel().AltUrl1().empty() || !ViewModel().AltUrl2().empty())
        {
            imageHeight = 0;
        }

        auto slider = sender.as<Controls::Slider>();
        Point point = e.GetCurrentPoint(slider).Position();

        double relativePosition = point.X / slider.ActualWidth();
        double sliderValue = slider.Minimum() + (slider.Maximum() - slider.Minimum()) * relativePosition;

        PreviewTimeText().Text(Utils::DurationToString(sliderValue * 1000));

        auto transform = slider.TransformToVisual(LayoutRoot());
        auto screenPoint = transform.TransformPoint(point);

        double offsetX = screenPoint.X - PreviewPopup().ActualWidth();

        PreviewPopup().HorizontalOffset(offsetX);
        PreviewPopup().VerticalOffset(point.Y - PreviewPopup().ActualHeight() - imageHeight - 10);

        if (abs(sliderValue - lastSliderValue) < 1.0)
        {
            co_return;
        }
        lastSliderValue = sliderValue;

        if (!ViewModel().AltUrl1().empty() || !ViewModel().AltUrl2().empty())
        {
            co_return;
        }

        auto frame = FfmpegDecoder::GetFrame(ViewModel().Path(), sliderValue * 1000, imageHeight);

        if (frame.Buffer.empty())
        {
            OutputDebugString(L"MainPage::Slider_Timeline_PointerMoved GetFrame\n");
            co_return;
        }

        auto memoryStream = Windows::Storage::Streams::InMemoryRandomAccessStream();
        auto encoder = co_await Windows::Graphics::Imaging::BitmapEncoder::CreateAsync(
            Windows::Graphics::Imaging::BitmapEncoder::PngEncoderId(), memoryStream
        );

        encoder.SetPixelData(
            Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
            Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied,
            frame.Width,
            frame.Height,
            96.0,
            96.0,
            array_view<const uint8_t>(frame.Buffer.data(), frame.Buffer.data() + frame.Buffer.size())
        );
        co_await encoder.FlushAsync();
        memoryStream.Seek(0);

        auto bitmap = Media::Imaging::WriteableBitmap(frame.Width, frame.Height);
        bitmap.SetSource(memoryStream);

        PreviewImage().Source(bitmap);
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
