#include "http_utils.h"

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
