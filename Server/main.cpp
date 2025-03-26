#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudacodec.hpp>
#include <opencv2/cudaobjdetect.hpp>
#include <opencv2/cudaoptflow.hpp>
#include <opencv2/cudastereo.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudafeatures2d.hpp>

#pragma comment(lib, "Ws2_32.lib")

void clientThread(const SOCKET& client_socket) {

    int bytes_sent, bytes_read;
    char* buffer;
    int buffer_size;
    std::vector<uchar> image_buffer;
    try {
        buffer = new char[4];
        // receive width
        bytes_read = recv(client_socket, buffer, 4, 0);
        if (bytes_read <= 0) throw std::runtime_error("Error during receiving buffer size");
        std::memcpy(&buffer_size, buffer, 4);
        delete[] buffer;

        image_buffer.resize(buffer_size);
        buffer = new char[buffer_size];
        
        int total_read = 0;
        while (total_read < buffer_size) {
            bytes_read = recv(client_socket, buffer + total_read, buffer_size - total_read, 0);
            if (bytes_read <= 0) {
                throw std::runtime_error("Error during receiving buffer");
            }
            total_read += bytes_read;
        }
        std::memcpy(image_buffer.data(), buffer, buffer_size);
        delete[] buffer;
        
        cv::Mat in = cv::imdecode(image_buffer, cv::IMREAD_ANYCOLOR);

        //actions
        cv::cuda::GpuMat imgGpu;
        imgGpu.upload(in);

        cv::cuda::cvtColor(imgGpu, imgGpu, cv::COLOR_BGR2GRAY);

        cv::Mat out;
        imgGpu.download(out);

        //send       
        std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 100, cv::IMWRITE_JPEG_OPTIMIZE, 1 };
        bool success = cv::imencode(".jpg", out, image_buffer, params);
        if (!success) {
            throw std::runtime_error("Error during receiving buffer");
        }

        buffer_size = image_buffer.size();

        buffer = new char[4];
        std::memcpy(buffer, &buffer_size, 4);
        send(client_socket, buffer, 4, 0);
        delete[] buffer;

        buffer = new char[buffer_size];
        std::copy(image_buffer.begin(), image_buffer.end(), buffer);

        int total_sent = 0;
        while (total_sent < buffer_size) {
            bytes_sent = send(client_socket, buffer + total_sent, buffer_size - total_sent, 0);
            if (bytes_sent <= 0) {
                throw std::runtime_error("Error during sending buffer");
            }
            total_sent += bytes_sent;
        }
        delete[] buffer;
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
    }
    closesocket(client_socket);
}

int main() {

    std::cout << "Build " << __DATE__ << "  " << __TIME__ << "\n";
    cv::cuda::printCudaDeviceInfo(0);

    WSADATA wsaData;
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

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
    server_addr.sin_port = htons(12345);

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

            client_socket = accept(server_socket, (SOCKADDR*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                throw std::runtime_error("Accept failed: " + WSAGetLastError());
            }

            sockaddr_in* client_info = reinterpret_cast<sockaddr_in*>(&client_addr);
            char* client_ip = inet_ntoa(client_info->sin_addr);
            int client_port = ntohs(client_info->sin_port);

            std::string client_address = std::string(client_ip) + ":" + std::to_string(client_port);

            std::cout << "Client connected with " << client_address << "\n";

            std::thread th(&clientThread, client_socket);
            
            std::stringstream ss;
            ss << th.get_id();
            uint64_t id = std::stoull(ss.str());
            std::cout << "Assigned to thread with id " << id << " (detached)\n";

            th.detach();
        }
        catch (const std::exception& ex) {
            std::cout << ex.what() << "\n";
        }
    }
    // Cleanup
    closesocket(server_socket);
    WSACleanup();
    return 0;
}