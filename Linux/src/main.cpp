#include <opencv2/opencv.hpp>

#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    int rowSize;
    char* buffer;
    int width, height, channels;
    ssize_t bytes_read;
    
    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation error");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    std::string server_ip;
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
    
    // Convert IPv4/IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        perror("invalid address/ address not supported");
        return -1;
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        return -1;
    }
    
    std::string image_path = cv::samples::findFile("test.jpg");
    cv::Mat img = imread(image_path, cv::IMREAD_COLOR);
 
    if(img.empty())
    {
        std::cout << "Could not read the image: " << image_path << std::endl;
        return 1;
    }

    width = img.cols;
    height = img.rows;
    channels = img.channels();

    // Send message to server
    buffer = new char[4];

    std::memcpy(buffer, &width, 4);
    send(sock, buffer, 4, 0);
    
    std::memcpy(buffer, &height, 4);
    send(sock, buffer, 4, 0);
    
    std::memcpy(buffer, &channels, 4);
    send(sock, buffer, 4, 0);

    delete[] buffer;

    rowSize = width * channels;
    buffer = new char[rowSize];
    for (int row = 0; row < height; row++) {
        std::memcpy(buffer, img.ptr(row), rowSize);
        int total_sent = 0;
        while (total_sent < rowSize) {
            int bytes_sent = send(sock, buffer + total_sent, rowSize - total_sent, 0);
            if (bytes_sent <= 0) {
                std::cout << "Error sending image data\n";
                delete[] buffer;
                return -1;
            }
            total_sent += bytes_sent;
        }
    }
    delete[] buffer;
    
    // Read server response

    buffer = new char[4];
    bytes_read = read(sock, buffer, 4);
    if(bytes_read <= 0) {
        std::cout << "Error geting width\n";
        return -1;
    }
    std::memcpy(&width, buffer, 4);

    bytes_read = read(sock, buffer, 4);
    if(bytes_read <= 0) {
        std::cout << "Error geting height\n";
        return -1;
    }
    std::memcpy(&height, buffer, 4);

    bytes_read = read(sock, buffer, 4);
    if(bytes_read <= 0) {
        std::cout << "Error geting channels\n";
        return -1;
    }
    std::memcpy(&channels, buffer, 4);

    cv::Mat out(height, width, CV_8UC(channels));

    rowSize = width * channels;
    buffer = new char[rowSize];
    for (int row = 0; row < height; row++) {
        int total_read = 0;
        while (total_read < rowSize) {
            int bytes_read = read(sock, buffer + total_read, rowSize - total_read);
            if (bytes_read <= 0) {
                std::cout << "Error getting mat\n";
                delete[] buffer;
                return -1;
            }
            total_read += bytes_read;
        }
        std::memcpy(out.ptr(row), buffer, rowSize);
    }
    delete[] buffer;
    
    close(sock);

    std::vector<int> compression_params;
   
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(50);

    compression_params.push_back(cv::IMWRITE_JPEG_PROGRESSIVE);
    compression_params.push_back(1);

    compression_params.push_back(cv::IMWRITE_JPEG_OPTIMIZE);
    compression_params.push_back(1);

    cv::imwrite("Out.jpg", out, compression_params);
    return 0;
}