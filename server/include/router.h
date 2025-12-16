#pragma once
#include <string>
#include <map>

std::map<std::string, std::string>
parse_request_path(const std::string& request, std::string& route);
