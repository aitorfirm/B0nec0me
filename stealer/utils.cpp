#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <Lmcons.h>   // UNLEN
#include <string>

std::string get_username() {
    char username[UNLEN + 1];
    DWORD len = UNLEN + 1;
    if (GetUserNameA(username, &len)) {
        return std::string(username);
    }
    return "";
}

std::string create_temp_directory(const std::string& subfolder) {
    char temp_path[MAX_PATH];
    if (!GetTempPathA(MAX_PATH, temp_path)) {
        return "";
    }

    std::string full_path = std::string(temp_path) + subfolder + "\\";
    std::error_code ec;
    if (!std::filesystem::exists(full_path)) {
        std::filesystem::create_directories(full_path, ec);
        if (ec) {
            return "";
        }
    }
    return full_path;
}

std::string get_desktop_path() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path))) {
        return std::string(path);
    }
    return "";
}
