#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    std::string server_ip;
    int result;
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
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    if (!cam.isOpened()) {
        std::cout << "Failed to make connection to cam" << std::endl;
        return 1;
    }
    int imageIndex = 0;
    
    while (true) {
        try {

            cam >> target;

            int width = target.cols;
            int height = target.rows;
            int channels = target.channels();

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

            int rowSize = width * channels * sizeof(uchar);
            buffer = new char[rowSize];
            for (int row = 0; row < height; row++) {
                std::memcpy(buffer, target.ptr(row), rowSize);
                send(client_socket, buffer, rowSize, 0);
            }
            delete[] buffer;

            std::cout << "Message sent" << std::endl;
            // Receive response

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

            cv::Mat out(height, width, CV_8UC(channels));

            rowSize = width * channels * sizeof(uchar);
            buffer = new char[rowSize];
            for (int row = 0; row < height; row++) {
               
                result = recv(client_socket, buffer, rowSize, 0);
                if (result <= 0) throw std::runtime_error("Error during receiving matrix");
                memcpy(out.ptr(row), buffer, rowSize);
            }
            delete[] buffer;

            cv::pyrUp(out, out, cv::Size(out.cols * 2, out.rows * 2));

            std::string fileName = "images/Capture" + std::to_string(imageIndex) + ".jpg";
            //cv::imwrite(fileName, out);
            cv::imshow("Imge", out);
            cv::waitKey(1);
            imageIndex++;
            closesocket(client_socket);
        }
        catch (const std::exception& ex) {
            std::cout << ex.what() << "\n";
        }
    }    
    cam.release();
    WSACleanup();
    return 0;
}