#pragma once
#include <string>
#include <map>

extern std::map<std::string, std::string> APPS;

std::string list_apps();
void start_app(const std::string& app_name);
void stop_app(const std::string& app_name);
