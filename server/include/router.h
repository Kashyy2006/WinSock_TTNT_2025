#pragma once
#include <string>
#include <map>

std::map<std::string, std::string>
parse_query(const std::string& path);
std::string get_route_path(const std::string& path);