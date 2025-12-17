#include <string>
#include <winsock2.h>
#include <tlhelp32.h>
#include <algorithm>
#include "apps.h"

// Helper: Kiểm tra process đang chạy
bool is_running(const std::string& exe_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            std::string current_exe(pe.szExeFile);
            
            std::string target_exe = exe_name;
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

// Helper: Stop process
int stop_app_by_exe(const std::string& exe_name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    int count = 0;

    if (Process32First(hSnapshot, &pe)) {
        do {
            std::string current_exe(pe.szExeFile);
            std::string target_exe = exe_name;
            std::transform(current_exe.begin(), current_exe.end(), current_exe.begin(), ::tolower);
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

// Trả về danh sách <li> để frontend hiển thị
std::string list_apps() {
    std::string rows;
    for (const auto& pair : APPS) {
        std::string name = pair.first;
        std::string exe = pair.second;
        bool running = is_running(exe);
        
        // CSS class badge để hiển thị màu
        std::string status_badge = running 
            ? "<span style='color:green; font-weight:bold'>RUNNING</span>" 
            : "<span style='color:gray'>STOPPED</span>";
        
        rows += "<li>";
        rows += "<strong>" + name + "</strong>: " + status_badge;
        rows += "</li>";
    }
    // Trả về chuỗi raw, không bọc <html>
    return rows;
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