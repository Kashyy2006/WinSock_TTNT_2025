#include <string>
#include "http_utils.h"

std::string system_control(const std::string& action) {
    if (action == "shutdown") {
        system("shutdown /s /t 60"); // Shutdown sau 60s
        return http_response("Shutting down in 60s...");
    } else if (action == "restart") {
        system("shutdown /r /t 60");
        return http_response("Restarting in 60s...");
    }
    return http_response("Unknown command", "400 Bad Request");
}