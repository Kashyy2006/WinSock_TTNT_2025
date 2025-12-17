#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <sstream>
#include <algorithm>
#include "processes.h"

// Trả về danh sách <li>
std::string list_processes() {
    std::stringstream rows;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
    
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return "<li>Error: Cannot snapshot processes.</li>";
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            // Format: [PID] Tên_File
            rows << "<li><span style='color:#fbbf24CC'>[" << pe.th32ProcessID << "]</span> " << pe.szExeFile << "</li>";
        } while (Process32Next(hSnapshot, &pe));
    } else {
        CloseHandle(hSnapshot);
        return "<li>Error: Cannot list processes.</li>";
    }

    CloseHandle(hSnapshot);
    return rows.str();
}

std::string start_process(const std::string& path) {
    HINSTANCE result = ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);

    if ((intptr_t)result <= 32) {
        return "Error starting process. Code: " + std::to_string(GetLastError());
    }
    return "Started: " + path;
}

std::string stop_process_by_pid(const std::string& pid_str) {
    DWORD pid = 0;
    try {
        pid = std::stoul(pid_str);
    } catch (...) {
        return "Error: Invalid PID format.";
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid); 
    if (hProcess == NULL) {
        return "Error: Cannot open PID " + pid_str;
    }
    
    if (!TerminateProcess(hProcess, 0)) {
        CloseHandle(hProcess);
        return "Error terminating PID " + pid_str;
    }

    CloseHandle(hProcess);
    return "Terminated PID " + pid_str;
}

std::string stop_process_by_name(const std::string& name) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return "Error: Cannot snapshot processes.";
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
    return "Terminated " + std::to_string(count) + " processes named " + name;
}