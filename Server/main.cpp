#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);
    long long int value;
    int messageSize = sizeof(long long int);
    char* buffer;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostent* host = gethostbyname(hostname);
        if (host) {
            for (int i = 0; host->h_addr_list[i] != nullptr; i++) {
                std::cout << "Server IP: " << inet_ntoa(*(struct in_addr*)host->h_addr_list[i]) << "\n";
            }
        }
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    
    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // Bind socket
    if (bind(server_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    while (true) {
            std::cout << "Waiting for connection...\n";

            // Accept client connection
            client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                closesocket(server_socket);
                WSACleanup();
                return 1;
            }

            std::cout << "Client connected!" << std::endl;

            buffer = new char[messageSize];
            
            // Receive message
            int result = recv(client_socket, buffer, messageSize, 0);
            if (result > 0) {
                std::memcpy(&value, buffer, messageSize);
                std::cout << "Received: " << value << std::endl;

                value *= 100;

                delete[] buffer;
                buffer = new char[messageSize];
                std::memcpy(buffer, &value, messageSize);
                send(client_socket, buffer, messageSize, 0);
            }
            delete[] buffer;
            closesocket(client_socket);
    }

    // Cleanup
    closesocket(server_socket);
    WSACleanup();
    return 0;
}