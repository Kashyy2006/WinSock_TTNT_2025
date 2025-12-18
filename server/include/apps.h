#pragma once
#include <string>
#include <map>
#include "config.h"
#include "http_utils.h"

bool is_running(const std::string& exe_name);
void start_app_sys(const std::string& exe_path);
std::string list_apps();