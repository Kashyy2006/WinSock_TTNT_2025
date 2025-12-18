#include <string>
#include <map>
#include "router.h"
#include <sstream>

std::map<std::string, std::string> parse_query(const std::string& path) {
    std::map<std::string, std::string> query;
    size_t q_pos = path.find('?');
    if (q_pos != std::string::npos) {
        std::string query_str = path.substr(q_pos + 1);
        std::stringstream ss(query_str);
        std::string segment;
        while (std::getline(ss, segment, '&')) {
            size_t eq_pos = segment.find('=');
            if (eq_pos != std::string::npos) {
                query[segment.substr(0, eq_pos)] = segment.substr(eq_pos + 1);
            }
        }
    }
    return query;
}

std::string get_route_path(const std::string& path) {
    size_t q_pos = path.find('?');
    if (q_pos != std::string::npos) return path.substr(0, q_pos);
    return path;
}