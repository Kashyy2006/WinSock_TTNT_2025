// Tên miền

// - GET /                -> index page
// - GET /apps        -> list configured apps (APPS dict)
// - GET /apps/start?app=name  -> start app (by key in APPS)
// - GET /apps/stop?app=name   -> stop app (by key in APPS)

// - GET /processes        -> list running processes (name + pid)
// - GET /processes/actions/start?path=... -> start arbitrary exe by path
// - GET /processes/actions/stop?pid=1234  -> stop process by PID
// - GET /processes/actions/stop?name=chrome.exe -> stop processes by name

// - GET /screenshot      -> take screenshot, returns filename
// - GET /webcam_rec      -> record 10 seconds from default webcam, returns filename

// - GET /shutdown        -> shutdown machine (requires admin)
// - GET /restart         -> restart machine (requires admin)

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>

std::map<std::string, std::string> APPS = {
    {"notepad", "notepad.exe"},
    {"mspaint", "mspaint.exe"},
    {"cmd", "cmd.exe"}
};

const int PORT = 8080;
const int BUFFER_SIZE = 4096;

bool is_running(const std::string& exe_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            std::string current_exe(pe.szExeFile);
            
            std::transform(current_exe.begin(), current_exe.end(), current_exe.begin(), ::tolower);
            std::string target_exe = exe_name;
            std::transform(target_exe.begin(), target_exe.end(), target_exe.begin(), ::tolower);

            if (current_exe == target_exe) {
                CloseHandle(hSnapshot);
                return true;
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return false;
}

int stop_app_by_exe(const std::string& exe_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    int count = 0;

    if (Process32First(hSnapshot, &pe)) {
        do {
            std::string current_exe(pe.szExeFile);
            std::transform(current_exe.begin(), current_exe.end(), current_exe.begin(), ::tolower);
            std::string target_exe = exe_name;
            std::transform(target_exe.begin(), target_exe.end(), target_exe.begin(), ::tolower);

            if (current_exe == target_exe) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    count++;
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return count;
}


std::string html_page(const std::string& body) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    return response;
}

std::string redirect(const std::string& url = "/") {
    std::string response = "HTTP/1.1 302 Found\r\n";
    response += "Location: " + url + "\r\n";
    response += "Content-Length: 0\r\n";
    response += "\r\n";
    return response;
}

// ====================== LIST APPS ======================

std::string list_apps() {
    std::string rows;
    for (const auto& pair : APPS) {
        std::string name = pair.first;
        std::string exe = pair.second;
        bool running = is_running(exe);
        std::string status = running ? "running" : "stopped";
        std::string action = running ? "Stop" : "Start";
        std::string action_route = running ? "/apps/stop" : "/apps/start";
        
        rows += "<li>" + name + " - " + status + " | ";
        rows += " <a href='" + action_route + "?app=" + name + "'>" + action + "</a>";
        rows += "</li>";
    }

    return html_page("<h1>App List</h1><ul>" + rows + "</ul>");
}

void start_app(const std::string& app_name) {
    if (APPS.count(app_name)) {
        std::string exe = APPS[app_name];
        
        ShellExecuteA(NULL, "open", exe.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}

void stop_app(const std::string& app_name) {
    if (APPS.count(app_name)) {
        std::string exe = APPS[app_name];
        stop_app_by_exe(exe);
    }
}

std::map<std::string, std::string> parse_request_path(const std::string& request, std::string& route) {
    std::map<std::string, std::string> query;
    size_t start = request.find(' ');
    size_t end = request.find(' ', start + 1);

    if (start != std::string::npos && end != std::string::npos) {
        std::string full_path = request.substr(start + 1, end - (start + 1));
        
        size_t query_pos = full_path.find('?');
        if (query_pos != std::string::npos) {
            route = full_path.substr(0, query_pos);
            std::string query_str = full_path.substr(query_pos + 1);
            
            size_t eq_pos = query_str.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = query_str.substr(0, eq_pos);
                std::string val = query_str.substr(eq_pos + 1);
                query[key] = val;
            }

        } else {
            route = full_path;
        }
    }
    return query;
}
// ====================== LIST PROCESSES ======================

std::string list_processes() {
    std::stringstream rows;
    // Tạo snapshot của tất cả các tiến trình hiện tại
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
    
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return html_page("<h1>Error: Khong the tao snapshot.</h1>");
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            // Lấy PID (th32ProcessID) và Tên (szExeFile)
            rows << "<li>" << pe.th32ProcessID << " - " << pe.szExeFile << "</li>\n";
        } while (Process32Next(hSnapshot, &pe));
    } else {
        CloseHandle(hSnapshot);
        return html_page("<h1>Error: Khong the liet ke tien trinh.</h1>");
    }
    

    CloseHandle(hSnapshot);
    return html_page("<h1>Processes</h1><ul>" + rows.str() + "</ul>");
}

std::string start_process(const std::string& path) {

    HINSTANCE result = ShellExecuteA(
        NULL,       // handle cho cửa sổ cha (không có)
        "open",     // hành động (mở)
        path.c_str(), // Đường dẫn chương trình (exe, bat, cmd, etc.)
        NULL,       // Tham số dòng lệnh
        NULL,       // Thư mục làm việc
        SW_SHOWNORMAL // Cách hiển thị cửa sổ
    );

    if ((intptr_t)result <= 32) {
        DWORD error = GetLastError();
        return html_page("<h1>Error starting process: " + std::to_string(error) + "</h1>");
    }

    return html_page("<h1>Started: " + path + "</h1>");
}

std::string stop_process_by_pid(const std::string& pid_str) {
    DWORD pid = 0;
    try {
        pid = std::stoul(pid_str);
    } catch (...) {
        return html_page("<h1>Error: PID khong hop le.</h1>");
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid); 

    if (hProcess == NULL) {
        DWORD error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER) {
            return html_page("<h1>Error: PID " + pid_str + " khong ton tai.</h1>");
        }
        return html_page("<h1>Error: Khong the mo process (" + std::to_string(error) + ").</h1>");
    }
    
    if (!TerminateProcess(hProcess, 0)) {
        DWORD error = GetLastError();
        CloseHandle(hProcess);
        return html_page("<h1>Error terminating process: " + std::to_string(error) + "</h1>");
    }

    CloseHandle(hProcess);
    return html_page("<h1>Terminated PID " + pid_str + "</h1>");
}

std::string stop_process_by_name(const std::string& name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return html_page("<h1>Error: Khong the tao snapshot.</h1>");
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    int count = 0;

    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    if (Process32First(hSnapshot, &pe)) {
        do {
            std::string current_exe(pe.szExeFile);
            std::transform(current_exe.begin(), current_exe.end(), current_exe.begin(), ::tolower);

            if (current_exe == lower_name) {
                // Giong nhu stop_process_by_pid, nhung su dung PID cua tien trinh hien tai
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess != NULL) {
                    if (TerminateProcess(hProcess, 0)) {
                        count++;
                    }
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return html_page("<h1>Terminated " + std::to_string(count) + " processes named " + name + "</h1>");
}

// ====================== SCREENSHOT ======================
// ====================== WEBCAM REC 10S ======================
// ====================== SHUTDOWN/RESTART ======================

// ====================== CHƯƠNG TRÌNH CHÍNH ======================

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
                
            }else if (route == "/processes") {
                response = list_processes();
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