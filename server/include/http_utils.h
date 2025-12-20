#pragma once
#include <string>
#include <winsock2.h>

std::string http_response(const std::string& body, const std::string& status = "200 OK", const std::string& content_type = "text/html");
void serve_file(SOCKET client_socket, const std::string& filepath);