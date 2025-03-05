#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#pragma comment(lib, "Ws2_32.lib")

int main() {

    std::cout << "Build " << __DATE__ << "  " << __TIME__ << "\n";

    WSADATA wsaData;
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    int result;
    char* buffer;
    int width, height, channels;

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

            buffer = new char[4];
            // receive width
            result = recv(client_socket, buffer, 4, 0);
            if (result <= 0) throw std::runtime_error("Error during receiving width");
            std::memcpy(&width, buffer, 4);
            // receive height
            result = recv(client_socket, buffer, 4, 0);
            if (result <= 0) throw std::runtime_error("Error during receiving height");
            std::memcpy(&height, buffer, 4);
            // receive height
            result = recv(client_socket, buffer, 4, 0);
            if (result <= 0) throw std::runtime_error("Error during receiving channels");
            std::memcpy(&channels, buffer, 4);

            delete[] buffer;

            cv::Mat in(height, width, CV_8UC(channels));

            int rowSize = width * channels * sizeof(uchar);
            buffer = new char[rowSize];
            for (int row = 0; row < height; row++) {
                result = recv(client_socket, buffer, rowSize, 0);
                if (result <= 0) throw std::runtime_error("Error during receiving matrix");
                memcpy(in.ptr(row), buffer, rowSize);
            }
            delete[] buffer;
            
            std::cout << "Recived " << width << "x" << height << " image with " << channels << " channels\n";

            //actions

            cvtColor(in, in, cv::COLOR_RGB2GRAY);

            //send
            width = in.cols;
            height = in.rows;
            channels = in.channels();

            buffer = new char[4];
            //send width
            std::memcpy(buffer, &width, 4);
            send(client_socket, buffer, 4, 0);
            //send height
            std::memcpy(buffer, &height, 4);
            send(client_socket, buffer, 4, 0);
            //send channels
            std::memcpy(buffer, &channels, 4);
            send(client_socket, buffer, 4, 0);
            //send bytes
            delete[] buffer;

            rowSize = width * channels * sizeof(uchar);
            buffer = new char[rowSize];
            for (int row = 0; row < height; row++) {
                std::memcpy(buffer, in.ptr(row), rowSize);
                send(client_socket, buffer, rowSize, 0);
            }
            delete[] buffer;
        }
        catch (const std::exception& ex) {
            std::cout << ex.what() << "\n";
        }
        closesocket(client_socket);
    }

    // Cleanup
    closesocket(server_socket);
    WSACleanup();
    return 0;
}