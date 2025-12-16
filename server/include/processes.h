#pragma once
#include <string>

std::string list_processes();
std::string start_process(const std::string& path);
std::string stop_process_by_pid(const std::string& pid);
std::string stop_process_by_name(const std::string& name);
