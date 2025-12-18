#pragma once
#include <string>

std::string http_response(const std::string& body, const std::string& status = "200 OK", const std::string& content_type = "text/html");