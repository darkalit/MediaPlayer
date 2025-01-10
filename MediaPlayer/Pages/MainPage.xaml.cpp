#include "pch.h"
#include "MainPage.xaml.h"
#if __has_include("MainPage.g.cpp")
#include "MainPage.g.cpp"
#endif

#include "App.xaml.h"
#include "Services/PlayerService.h"

using namespace winrt;
using namespace Microsoft::UI;
using namespace Xaml;
using namespace Xaml::Input;
using namespace Windows::Foundation;

namespace winrt::MediaPlayer::implementation
{
    MainPage::MainPage()        
        :
        m_PlayerService(App::GetPlayerService())
    {
    }

    void MainPage::OnLoad(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        m_PlayerService->SetSwapChainPanel(SwapChainPanel_Video());
    }

    void MainPage::SwapChainPanel_Video_SizeChanged(Windows::Foundation::IInspectable const&, SizeChangedEventArgs const&)
    {
        auto size = SwapChainPanel_Video().ActualSize();
        m_PlayerService->ResizeVideo(size.x, size.y);
    }
}
