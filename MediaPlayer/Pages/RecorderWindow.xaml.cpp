#include "pch.h"
#include "RecorderWindow.xaml.h"
#if __has_include("RecorderWindow.g.cpp")
#include "RecorderWindow.g.cpp"
#endif

#include "App.xaml.h"
#include "ViewModels/RecorderViewModel.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::MediaPlayer::implementation
{
    RecorderWindow::RecorderWindow()
    {
        m_ViewModel = make<MediaPlayer::implementation::RecorderViewModel>();
    }

    MediaPlayer::RecorderViewModel RecorderWindow::ViewModel()
    {
        return MediaPlayer::RecorderViewModel();
    }
}
