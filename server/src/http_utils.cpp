#include "http_utils.h"
#include <fstream>
#include <sstream>

std::string http_response(const std::string& body, const std::string& status, const std::string& content_type) {
    std::string response = "HTTP/1.1 " + status + "\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n"; // Cho phép mọi nguồn truy cập
    response += "Access-Control-Allow-Methods: GET, POST\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    return response;
}