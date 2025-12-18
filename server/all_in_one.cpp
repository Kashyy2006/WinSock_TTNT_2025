// Tên miền

// - GET /                -> index page
// - GET /apps        -> list configured apps (APPS dict)
// - GET /apps/start?app=name  -> start app (by key in APPS)
// - GET /apps/stop?app=name   -> stop app (by key in APPS)

// - GET /processes        -> list running processes (name + pid)
// - GET /processes/actions/start?path=... -> start arbitrary exe by path
// - GET /processes/actions/stop?pid=1234  -> stop process by PID
// - GET /processes/actions/stop?name=chrome.exe -> stop processes by name

// - GET /keylogger            -> key capture page
// - GET /keylogger/send?key=K -> receive key from client

// - GET /screenshot      -> take screenshot, returns filename
// - GET /webcam_rec      -> record 10 seconds from default webcam, returns filename

// - GET /shutdown        -> shutdown machine (requires admin)
// - GET /restart         -> restart machine (requires admin)

#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>

// --- ADDED FOR SCREENSHOT & WEBCAM ---
#include <vfw.h>     // Video for Windows
// Link libraries (cho Visual Studio/MSVC)
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "vfw32.lib")
// -------------------------------------

std::map<std::string, std::string> APPS = {
    {"notepad", "notepad.exe"},
    {"mspaint", "mspaint.exe"},
    {"cmd", "cmd.exe"}
};

const int PORT = 6969;
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

// ====================== KeyLogger ======================
std::string KEY_LOG = "";

std::string keylog_control() {
    std::string body = R"(
        <h1>Key Logger (Client-side)</h1>
        <p>Typing anything and data will be sent to server!</p>

        <script>
            document.addEventListener("keydown", function(e) {
                fetch("/keylogger/send?key=" + encodeURIComponent(e.key));
            });
        </script>

        <p>Keys are being sent to server...</p>
    )";

    return html_page(body);
}

std::string keylog_receive(const std::map<std::string, std::string>& query) {
    if (!query.count("key")) {
        return html_page("<h1>No key</h1>");
    }

    std::string key = query.at("key");

    // Xử lý các phím đặc biệt cho dễ đọc
    if (key == "Enter") KEY_LOG += "\n";
    else if (key == "Space") KEY_LOG += " ";
    else if (key == "Backspace") {
        if (!KEY_LOG.empty()) KEY_LOG.pop_back();
    }
    else if (key.length() == 1) {
        KEY_LOG += key;
    } else {
        KEY_LOG += "[" + key + "]";
    }

    std::cout << "[KEY_LOG] " << KEY_LOG << std::endl;

    return html_page("<h1>OK</h1>");
}

// ====================== SCREENSHOT ======================

std::string take_screenshot() {
    int x1 = 0;
    int y1 = 0;
    int x2 = GetSystemMetrics(SM_CXSCREEN);
    int y2 = GetSystemMetrics(SM_CYSCREEN);
    int width = x2 - x1;
    int height = y2 - y1;

    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);
    SelectObject(hDC, hBitmap);

    BitBlt(hDC, 0, 0, width, height, hScreen, x1, y1, SRCCOPY);

    BITMAP bmpScreen;
    GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);
    
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
    
    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char *lpbitmap = (char *)GlobalLock(hDIB);
    GetDIBits(hScreen, hBitmap, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    std::string filename = "screenshot.bmp";
    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42; // BM

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    CloseHandle(hFile);
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);

    return filename;
}

// ====================== WEBCAM REC 10S ======================

void record_webcam_thread(std::string filename, int duration_ms) {
    // Tạo cửa sổ capture ẩn
    HWND hWndC = capCreateCaptureWindowA("WebcamCapture", WS_POPUP, 0, 0, 320, 240, NULL, 0);
    
    if (hWndC) {
        if (SendMessage(hWndC, WM_CAP_DRIVER_CONNECT, 0, 0)) {
            
            SendMessage(hWndC, WM_CAP_FILE_SET_CAPTURE_FILEA, 0, (LPARAM)filename.c_str());

            CAPTUREPARMS CaptureParms;
            SendMessage(hWndC, WM_CAP_GET_SEQUENCE_SETUP, sizeof(CAPTUREPARMS), (LPARAM)&CaptureParms);
            CaptureParms.fRequestMicroSecPerFrame = (1000000 / 15); // 15 FPS
            CaptureParms.fMakeUserHitOKToCapture = FALSE;
            CaptureParms.fYield = TRUE;
            CaptureParms.fCaptureAudio = FALSE;
            
            // Limit time
            CaptureParms.fLimitEnabled = TRUE;
            CaptureParms.wTimeLimit = duration_ms / 1000;

            SendMessage(hWndC, WM_CAP_SET_SEQUENCE_SETUP, sizeof(CAPTUREPARMS), (LPARAM)&CaptureParms);
            SendMessage(hWndC, WM_CAP_SEQUENCE, 0, 0); // Quay
            SendMessage(hWndC, WM_CAP_DRIVER_DISCONNECT, 0, 0);
        }
        DestroyWindow(hWndC);
    }
}

std::string start_webcam_recording() {
    std::string filename = "webcam_video.avi";
    std::thread t(record_webcam_thread, filename, 10000); // 10s
    t.detach();
    return filename;
}

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
                                     "<li><a href='/keylogger'>Keylogger</a></li>"
                                     "<li><a href='/screenshot'>Screenshot</a></li>"
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
            
            // === LOGIC MỚI ===
            } else if (route == "/screenshot") {
                std::string file = take_screenshot();
                response = html_page("<h1>Screenshot Saved</h1><p>File: " + file + "</p>");
            } else if (route == "/webcam_rec") {
                // 1. Lấy tham số 'time' từ URL query (ví dụ: /webcam_rec?time=20)
                int duration = 10; // Mặc định 10s
                
                if (query.count("time")) {
                    try {
                        duration = std::stoi(query["time"]); // Chuyển chuỗi sang số
                    } catch (...) {
                        duration = 10; // Nếu lỗi (người dùng nhập chữ), quay về 10s
                    }
                }

                // 2. Gọi hàm với thời gian tùy chỉnh
                std::string file = start_webcam_recording(duration);
                
                // 3. Phản hồi lại
                response = html_page("<h1>Recording Started</h1>"
                                     "<p>Duration: " + std::to_string(duration) + " seconds</p>"
                                     "<p>Saving to: " + file + "</p>");
            } else if (route == "/shutdown") {
                power_action(false);
                response = html_page("<h1>Shutting down...</h1>");
            } else if (route == "/restart") {
                power_action(true);
                response = html_page("<h1>Restarting...</h1>");
            // =================
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