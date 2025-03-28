#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#pragma comment(lib, "Ws2_32.lib")

const std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 100, cv::IMWRITE_JPEG_OPTIMIZE, 1 };

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    std::string server_ip;
    int bytes_sent, bytes_read;
    char* buffer;

    cv::VideoCapture cam(0);
    cv::Mat target;

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
    server_addr.sin_port = htons(12345);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    if (!cam.isOpened()) {
        std::cout << "Failed to make connection to camera" << std::endl;
        return 1;
    }
    
    while (true) {
        try {

            cam >> target;

            std::vector<uchar> image_buffer;

            bool success = cv::imencode(".jpg", target, image_buffer, params);
            if (!success) {
                throw std::runtime_error("Error get bytes of image");
            }

            int buffer_size = image_buffer.size();

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
            //send width
            std::memcpy(buffer, &buffer_size, 4);
            bytes_sent = send(client_socket, buffer, 4, 0);
            if (bytes_sent <= 0) {
                throw std::runtime_error("Error during sending size");
            }
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

            // Receive response

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
                    throw std::runtime_error("Error during receiving matrix");
                }
                total_read += bytes_read;
            }
            
            std::memcpy(image_buffer.data(), buffer, buffer_size);
            delete[] buffer;

            cv::Mat out  = cv::imdecode(image_buffer, cv::IMREAD_ANYCOLOR);

            cv::imshow("Imge", out);
            cv::waitKey(1);

            closesocket(client_socket);
        }
        catch (const std::exception& ex) {
            std::cout << ex.what() << "\n";
        }
    }
    cam.release();
    cv::destroyAllWindows();
    WSACleanup();
    return 0;
}