#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <sstream>
#include "processes.h"
#include "http_utils.h"

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