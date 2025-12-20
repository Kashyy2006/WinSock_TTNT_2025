#include <string>
#include <winsock2.h>
#include <tlhelp32.h>
#include <algorithm>
#include <sstream>
#include "apps.h"
#include "config.h"
#include "http_utils.h"

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
    
    EnumWindows(EnumWindowsProc, (LPARAM)&rows);
    
    return http_response(rows.str());
}