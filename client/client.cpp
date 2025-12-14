// client.cpp
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET client = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("192.168.1.10"); // IP SERVER

    connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr));

    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: 192.168.1.10\r\n"
        "Connection: close\r\n\r\n";

    send(client, request, strlen(request), 0);

    char buffer[4096];
    int bytes;
    while ((bytes = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        std::cout << buffer;
    }

    closesocket(client);
    WSACleanup();
    return 0;
}
