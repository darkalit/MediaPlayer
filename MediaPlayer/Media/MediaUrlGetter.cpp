#include "pch.h"
#include "MediaUrlGetter.h"

#include "Utils.h"
#include "winstring.h"
#include "winrt/Windows.Storage.h"

using namespace winrt;

std::vector<hstring> MediaUrlGetter::GetYoutubeStreamUrls(hstring const& url)
{
    SECURITY_ATTRIBUTES securityAttributes = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .bInheritHandle = true,
    };

    HANDLE stdOutRd = nullptr;
    HANDLE stdOutWr = nullptr;

    if (!CreatePipe(&stdOutRd, &stdOutWr, &securityAttributes, 0))
    {
        OutputDebugString(L"MediaUrlGetter::GetYoutubeStreamUrls Stdout pipe creation failed\n");
        return {};
    }

    if (!SetHandleInformation(stdOutRd, HANDLE_FLAG_INHERIT, 0))
    {
        OutputDebugString(L"MediaUrlGetter::GetYoutubeStreamUrls SetHandleInformation failed\n");
        return {};
    }

    STARTUPINFO info = {
        .cb = sizeof(info),
        .dwFlags = STARTF_USESTDHANDLES,
        .hStdOutput = stdOutWr,
    };
    PROCESS_INFORMATION processInfo;

    hstring cmd =
        Windows::ApplicationModel::Package::Current().InstalledLocation().Path() +
        L"\\Assets\\yt-dlp.exe" +
        L" -f \"bestvideo+bestaudio\" -g " +
        url;
    LPWSTR command = new wchar_t[cmd.size() + 1];
    defer{ delete[] command; };

    wcscpy_s(command, cmd.size() + 1, cmd.c_str());

    bool success = CreateProcess(
        nullptr,
        command,
        nullptr,
        nullptr,
        true,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &info,
        &processInfo);

    if (!success)
    {
        OutputDebugString(to_hstring(static_cast<uint64_t>(GetLastError())).c_str());
        OutputDebugString(L"\n");
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    CloseHandle(stdOutWr);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    DWORD read;
    std::vector<uint8_t> buffer(4096);
    std::string output;
    while (true)
    {
        bool readSuccess = ReadFile(stdOutRd, buffer.data(), buffer.size() - 1, &read, nullptr);
        if (!readSuccess || read == 0) break;
        buffer[read] = '\0';
        output += reinterpret_cast<char*>(buffer.data());
    }

    std::stringstream ss(output);

    std::vector<hstring> res;
    for (std::string t; std::getline(ss, t, '\n');)
    {
        res.push_back(to_hstring(t));
    }

    for (auto& str : res)
    {
        OutputDebugString(str.c_str());
        OutputDebugString(L"\n");
    }
    
    return res;
}
