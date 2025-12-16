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
#include "processes.h"
#include "http_utils.h"

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
