#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "App.xaml.h"
#include "winrt/Windows.Storage.Pickers.h"
#include "winrt/Windows.Foundation.h"
#include "ShObjIdl.h"
#include "Utils.h"

using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Foundation;
using namespace Microsoft::UI;
using namespace Xaml;
using namespace Xaml::Input;

namespace winrt::MediaPlayer::implementation
{
    MainWindow::MainWindow()
    {
    }
}
