#include "http_utils.h"
#include <fstream>
#include <sstream>

std::string html_page(const std::string& body) {
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.length()) + "\r\n"
        "Connection: close\r\n\r\n" +
        body;
    return response;
}

std::string redirect(const std::string& url) {
    return "HTTP/1.1 302 Found\r\n"
           "Location: " + url + "\r\n"
           "Content-Length: 0\r\n\r\n";
}

// Hàm đọc nội dung file từ đường dẫn
std::string load_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    return "";
}

// Hàm tạo HTTP Header chuẩn
std::string create_response(const std::string& content, const std::string& content_type) {
    std::string header = "HTTP/1.1 200 OK\r\n";
    header += "Content-Type: " + content_type + "\r\n";
    header += "Content-Length: " + std::to_string(content.size()) + "\r\n";
    header += "Connection: close\r\n\r\n"; // Kết thúc header
    return header + content;
}