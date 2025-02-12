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

std::wstring Utils::StringToWString(const std::string& str)
{
    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), nullptr, 0);
    std::wstring ws(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), ws.data(), size);
    return ws;
}
