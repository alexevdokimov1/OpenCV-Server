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

void printServerIPs() {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    std::cout << "Waiting for connections\n";

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        // Check for IPv4 addresses
        if (ifa->ifa_addr->sa_family == AF_INET) {
            void *tmp = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmp, addr, INET_ADDRSTRLEN);
            std::cout << " - " << addr << ":8080 (" << ifa->ifa_name << ")\n";
        }
    }
    freeifaddrs(ifaddr);
}

void clientThread(const int& client_socket) {

    int result;
    char* buffer;
    int width, height, channels;
    try {
        buffer = new char[4];
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

        int rowSize = width * channels;
        buffer = new char[rowSize];
        for (int row = 0; row < height; row++) {
            int total_read = 0;
            while (total_read < rowSize) {
                int bytes_read = recv(client_socket, buffer + total_read, rowSize - total_read, 0);
                if (bytes_read <= 0) {
                    throw std::runtime_error("Error during receiving matrix");
                }
                total_read += bytes_read;
            }
            memcpy(in.ptr(row), buffer, rowSize);
        }
        delete[] buffer;

        cv::Mat out;
        cv::cvtColor(in,out, cv::COLOR_RGB2GRAY);

        //send
        width = out.cols;
        height = out.rows;
        channels = out.channels();

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
            std::memcpy(buffer, out.ptr(row), rowSize);
            int total_sent = 0;
            while (total_sent < rowSize) {
                int bytes_sent = send(client_socket, buffer + total_sent, rowSize - total_sent, 0);
                if (bytes_sent <= 0) {
                    throw std::runtime_error("Error during sending matrix");
                }
                total_sent += bytes_sent;
            }
        }
        delete[] buffer;
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
    }
    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Attach socket to port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(12345);
    
    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for incoming connections
    if (listen(server_fd, 100) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printServerIPs();

    while(true){    
        try{
            std::cout << "Server listening\n";
        
            // Accept incoming connection
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);

            std::cout << "Client connected with " << client_ip << ":" << ntohs(address.sin_port) << "\n";

            std::thread th(&clientThread, new_socket);

            std::stringstream ss;
            ss << th.get_id();
            uint64_t id = std::stoull(ss.str());
            std::cout << "Assigned to thread with id " << id << " (detached)\n";

            th.detach();
        }
        catch(const std::exception& ex){
            std::cout << ex.what();
        }

    }
    close(server_fd);
    return 0;
}