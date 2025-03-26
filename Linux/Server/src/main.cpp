#include <opencv2/opencv.hpp>

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h> 
#include <thread>
#include <fstream>

#include "logger.hpp"

Logger logger("logfile.txt");

void printServerIPs() {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    logger.log(INFO, "Connection open:");

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            void *tmp = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmp, addr, INET_ADDRSTRLEN);
            std::stringstream ss;
            ss << " - " << addr << ":12345 (" << ifa->ifa_name << ")";
            logger.log(INFO, ss.str());
            ss.clear();
        }
    }
    freeifaddrs(ifaddr);
}

void clientThread(const int& client_socket) {

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
        cv::cvtColor(in, in, cv::COLOR_BGR2GRAY);

        cv::Mat out(in);

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
        logger.log(ERROR, ex.what());
    }
    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        logger.log(ERROR, "socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        logger.log(ERROR, "setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(12345);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        logger.log(ERROR, "bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 100) < 0) {
        logger.log(ERROR, "listen");
        exit(EXIT_FAILURE);
    }

    printServerIPs();
    
    logger.log(INFO, "Server listening");

    while(true){    
        try{
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                throw std::runtime_error("Error during acception");
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
            
            std::stringstream ss;
            ss << "Client connected with " << client_ip << ":" << ntohs(address.sin_port);
            logger.log(INFO, ss.str());
            std::thread th(&clientThread, new_socket);
            th.detach();
        }
        catch(const std::exception& ex){
            logger.log(ERROR, ex.what());
        }
    }
    close(server_fd);
    return 0;
}