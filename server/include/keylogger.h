#pragma once
#include <string>
#include <map>

std::string keylog_control();
std::string keylog_receive(const std::map<std::string, std::string>& query);