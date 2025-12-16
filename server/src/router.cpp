#include <string>
#include <map>
#include "router.h"

std::map<std::string, std::string> parse_request_path(const std::string& request, std::string& route) {
    std::map<std::string, std::string> query;
    size_t start = request.find(' ');
    size_t end = request.find(' ', start + 1);

    if (start != std::string::npos && end != std::string::npos) {
        std::string full_path = request.substr(start + 1, end - (start + 1));
        
        size_t query_pos = full_path.find('?');
        if (query_pos != std::string::npos) {
            route = full_path.substr(0, query_pos);
            std::string query_str = full_path.substr(query_pos + 1);
            
            size_t eq_pos = query_str.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = query_str.substr(0, eq_pos);
                std::string val = query_str.substr(eq_pos + 1);
                query[key] = val;
            }

        } else {
            route = full_path;
        }
    }
    return query;
}