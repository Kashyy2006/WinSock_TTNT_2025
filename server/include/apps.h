#pragma once
#include <string>
#include <map>
#include "config.h"
#include "http_utils.h"

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
void start_app_sys(const std::string& exe_path);
std::string list_apps();