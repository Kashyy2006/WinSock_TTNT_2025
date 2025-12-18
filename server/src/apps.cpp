#include <string>
#include <winsock2.h>
#include <tlhelp32.h>
#include <algorithm>
#include <sstream>
#include "apps.h"
#include "config.h"
#include "http_utils.h"

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