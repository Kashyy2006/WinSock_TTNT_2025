#include "config.h"

const int PORT = 8080;
const int BUFFER_SIZE = 4096;

std::map<std::string, std::string> APPS = {
    {"notepad", "notepad.exe"},
    {"mspaint", "mspaint.exe"},
    {"cmd", "cmd.exe"}
};
