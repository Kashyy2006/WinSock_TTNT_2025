#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// --- BIẾN TOÀN CỤC (Không dùng const nữa để có thể thay đổi) ---
std::string SERVER_IP = "127.0.0.1"; // Mặc định
int SERVER_PORT = 6969;              // Mặc định

// Hàm nhập cấu hình
void configure_connection() {
    std::string ip_input;
    std::string port_input;

    std::cout << "======================================\n";
    std::cout << "   REMOTE CONTROL CLIENT SETUP      \n";
    std::cout << "======================================\n";
    
    std::cout << "Nhap Server IP (Enter de dung mac dinh " << SERVER_IP << "): ";
    std::getline(std::cin, ip_input);
    if (!ip_input.empty()) {
        SERVER_IP = ip_input;
    }

    std::cout << "Nhap Server Port (Enter de dung mac dinh " << SERVER_PORT << "): ";
    std::getline(std::cin, port_input);
    if (!port_input.empty()) {
        try {
            SERVER_PORT = std::stoi(port_input);
        } catch (...) {
            std::cout << "(!) Port khong hop le, su dung mac dinh.\n";
        }
    }
    
    std::cout << ">> CAU HINH: " << SERVER_IP << ":" << SERVER_PORT << "\n";
    std::cout << "--------------------------------------\n";
}

// Hàm gửi lệnh HTTP (Giữ nguyên logic, chỉ thay đổi cách dùng biến toàn cục)
void send_command(const std::string& path) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;

    SOCKET client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    // Set timeout (để tránh bị treo nếu sai IP)
    DWORD timeout = 3000; // 3 giây
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());

    std::cout << "Dang ket noi den " << SERVER_IP << ":" << SERVER_PORT << "...\n";

    if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cout << "(!) LOI: Khong the ket noi! Hay kiem tra IP/Port.\n";
        closesocket(client);
        WSACleanup();
        return;
    }

    // Tạo request
    std::string request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + SERVER_IP + "\r\n";
    request += "Connection: close\r\n\r\n";

    send(client, request.c_str(), request.length(), 0);

    // Nhận phản hồi
    char buffer[4096];
    int bytes;
    std::string response = "";
    while ((bytes = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        response += buffer;
    }

    // In kết quả rút gọn
    size_t end_line = response.find("\r\n");
    if (end_line != std::string::npos) {
        std::cout << "<< Server: " << response.substr(0, end_line) << "\n";
    }

    closesocket(client);
    WSACleanup();
    std::cout << "--------------------------------------\n";
}

void show_menu() {
    std::cout << "\n[MENU] Dang ket noi den: " << SERVER_IP << ":" << SERVER_PORT << "\n";
    std::cout << "1. Kiem tra ket noi (Ping)\n";
    std::cout << "2. Xem Apps\n";
    std::cout << "3. Mo Notepad\n";
    std::cout << "4. Dong Notepad\n";
    std::cout << "5. Xem Processes\n";
    std::cout << "6. Keylogger Test\n";
    std::cout << "8. Doi IP/Port Server\n"; // Thêm chức năng đổi lại
    std::cout << "9. Shutdown Server\n";
    std::cout << "0. Thoat\n";
    std::cout << "Lua chon: ";
}

int main() {
    // Gọi hàm cấu hình ngay khi chạy
    configure_connection();

    int choice;
    std::string temp_input; 

    while (true) {
        show_menu();
        // Xử lý nhập liệu an toàn hơn tránh trôi lệnh khi dùng getline ở trên
        std::getline(std::cin, temp_input);
        if (temp_input.empty()) continue;
        
        try {
            choice = std::stoi(temp_input);
        } catch (...) {
            std::cout << "Nhap so hop le!\n";
            continue;
        }

        std::string path;

        switch (choice) {
            case 1: path = "/"; break;
            case 2: path = "/apps"; break;
            case 3: path = "/apps/start?app=notepad"; break;
            case 4: path = "/apps/stop?app=notepad"; break;
            case 5: path = "/processes"; break;
            case 6: 
                std::cout << "Nhap phim: ";
                {
                    std::string key;
                    std::getline(std::cin, key);
                    path = "/keylogger/send?key=" + key;
                }
                break;
            case 8: // Gọi lại hàm cấu hình
                configure_connection();
                continue; 
            case 9: path = "/shutdown"; break;
            case 0: return 0;
            default: std::cout << "Sai lua chon!\n"; continue;
        }

        if (!path.empty()) {
            send_command(path);
        }
    }
    return 0;
}