#include "http_utils.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <winsock2.h>

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

void serve_file(SOCKET client_socket, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::string msg = http_response("File not found", "404 Not Found");
        send(client_socket, msg.c_str(), msg.length(), 0);
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) {
        std::string content_type = "application/octet-stream";
        if (filepath.find(".bmp") != std::string::npos) content_type = "image/bmp";
        else if (filepath.find(".avi") != std::string::npos) content_type = "video/x-msvideo";

        std::string header = "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: " + content_type + "\r\n";
        header += "Content-Length: " + std::to_string(size) + "\r\n";
        header += "Access-Control-Allow-Origin: *\r\n";
        header += "Connection: close\r\n\r\n";

        send(client_socket, header.c_str(), header.length(), 0);
        
        send(client_socket, buffer.data(), size, 0);
    }
    file.close();
}