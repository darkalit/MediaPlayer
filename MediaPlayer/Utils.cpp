#include "pch.h"
#include "Utils.h"

using namespace winrt;

hstring Utils::DurationToString(uint64_t duration)
{
    if (duration == 0)
    {
        return L"00:00:00";
    }

    auto hours = duration / (1000 * 60 * 60);
    auto minutes = (duration / (1000 * 60)) % 60;
    auto seconds = (duration / 1000) % 60;

    return (hours < 10 ? L"0" : L"") + to_hstring(hours) + L":" + (minutes < 10 ? L"0" : L"") + to_hstring(minutes) + L":" + (seconds < 10 ? L"0" : L"") + to_hstring(seconds);
}
