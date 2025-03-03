#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>
#include <string>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

struct BufferImage {
    int bufferSize;
    char* buffer;
    int width;
    int height;
};

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    std::string server_ip;
    int result;
    char* buffer;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    std::ifstream config("IP.ini");
    if (config.is_open()) {
        config >> server_ip;
    }
    else {
        std::cout << "Enter IP address\n";
        std::cin >> server_ip;
        std::ofstream outConfig("IP.ini");
        outConfig << server_ip;
        outConfig.close();
    }
    config.close();

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    BufferImage data;
    
    while (true) {
        
        data.bufferSize = 20'000;
        data.buffer = new char[data.bufferSize];
        for (int i = 0; i < data.bufferSize; i++) {
            data.buffer[i] = i%10+65;
        }
        data.width = 1920;
        data.height = 1080;

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
        
        buffer = new char[4];
        //send size
        std::memcpy(buffer, &data.bufferSize, 4);
        send(client_socket, buffer, 4, 0);
        //send width
        std::memcpy(buffer, &data.width, 4);
        send(client_socket, buffer, 4, 0);
        //send height
        std::memcpy(buffer, &data.height, 4);
        send(client_socket, buffer, 4, 0);
        //send bytes
        send(client_socket, data.buffer, data.bufferSize, 0);
        std::cout << "Message sent" << std::endl;
        delete[] buffer;
        // Receive response
        
        buffer = new char[4];
        result = recv(client_socket, buffer, 4, 0);
        if (result > 0) {
            std::memcpy(&data.bufferSize, buffer, 4);
        }
        result = recv(client_socket, buffer, 4, 0);
        if (result > 0) {
            std::memcpy(&data.width, buffer, 4);
        }
        result = recv(client_socket, buffer, 4, 0);
        if (result > 0) {
            std::memcpy(&data.height, buffer, 4);
        }
        delete[] data.buffer;
        data.buffer = new char[data.bufferSize];
        result = recv(client_socket, data.buffer, data.bufferSize, 0);
        if (result > 0) {
            std::cout << "Recived: ";
            for (int i = 0; i < data.bufferSize; i++) {
                std::cout << data.buffer[i];
            }
            std::cout << "\n";
        }
        closesocket(client_socket);
        delete[] buffer;

        system("pause");
    }    
    WSACleanup();
    return 0;
}