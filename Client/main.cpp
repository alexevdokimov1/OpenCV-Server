#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    std::string server_ip;
    long long int value;
    int messageSize = sizeof(long long int);
    char* buffer;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    std::cout << "Enter IP address\n";
    std::cin >> server_ip;

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    
    while (true) {

        // Send message
        std::cout << "Enter number\n";
        std::cin >> value;

        // Create socket
        client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        // Connect to server
        if (connect(client_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
            closesocket(client_socket);
            WSACleanup();
            return 1;
        }

        buffer = new char[messageSize];
        std::memcpy(buffer, &value, messageSize);

        send(client_socket, buffer, messageSize, 0);
        std::cout << "Message sent" << std::endl;

        // Receive response
        delete[] buffer;
        buffer = new char[messageSize];
        int result = recv(client_socket, buffer, messageSize, 0);
        if (result > 0) {
            std::memcpy(&value, buffer, messageSize);
            std::cout << "Server response: " << value << std::endl;
        }
        closesocket(client_socket);
        delete[] buffer;
    }    
    WSACleanup();
    return 0;
}