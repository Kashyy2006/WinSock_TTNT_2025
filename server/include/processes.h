#pragma once
#include <string>

std::string list_processes();
void stop_process_sys(const std::string& process_name);