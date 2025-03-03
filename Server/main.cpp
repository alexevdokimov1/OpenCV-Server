#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

struct BufferImage {
    int bufferSize;
    char* buffer;
    int width;
    int height;
};

int main() {

    std::cout << "Build " << __DATE__ << "  " << __TIME__ << "\n";
    WSADATA wsaData;
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);
    int result;
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
        try {
            std::cout << "Waiting for connection...\n";

            // Accept client connection
            client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                throw std::runtime_error("Accept failed: " + WSAGetLastError());
            }

            std::cout << "Client connected!" << std::endl;

            BufferImage data;

            // Receive message
            buffer = new char[4];
            //get size
            result = recv(client_socket, buffer, 4, 0);
            if (result <= 0) throw std::runtime_error("Error during receiving bytes count");
            std::memcpy(&data.bufferSize, buffer, 4);

            //get width
            result = recv(client_socket, buffer, 4, 0);
            if (result <= 0) throw std::runtime_error("Error during receiving width");
            std::memcpy(&data.width, buffer, 4);

            //get height
            result = recv(client_socket, buffer, 4, 0);
            if (result <= 0) throw std::runtime_error("Error during receiving height");
            std::memcpy(&data.height, buffer, 4);

            delete[] buffer;

            data.buffer = new char[data.bufferSize];
            result = recv(client_socket, data.buffer, data.bufferSize, 0);
            if (result <= 0) throw std::runtime_error("Error during receiving bytes");
            std::cout << "Recived " << data.bufferSize << " bytes with " << data.width << "x" << data.height << "\n";

            //some actions

            buffer = new char[4];
            std::memcpy(buffer, &data.bufferSize, 4);
            send(client_socket, buffer, 4, 0);

            std::memcpy(buffer, &data.width, 4);
            send(client_socket, buffer, 4, 0);

            std::memcpy(buffer, &data.height, 4);
            send(client_socket, buffer, 4, 0);

            delete[] buffer;
            send(client_socket, data.buffer, data.bufferSize, 0);
            
        }
        catch (const std::exception& ex) {
            std::cout << ex.what();
        }
        closesocket(client_socket);
    }

    // Cleanup
    closesocket(server_socket);
    WSACleanup();
    return 0;
}