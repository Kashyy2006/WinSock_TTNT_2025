#pragma once
#include <string>

std::string html_page(const std::string& body);
std::string redirect(const std::string& url = "/");
std::string load_file(const std::string& path);
std::string create_response(const std::string& content, const std::string& content_type); 
