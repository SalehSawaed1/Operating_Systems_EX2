// File: molecule_requester.cpp
// Description: UDP client for requesting molecules from supplier_molecule server

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

constexpr int BUFFER_SIZE = 1024;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <HOSTNAME> <UDP_PORT>\n";
        return 1;
    }

    const char* hostname = argv[1];
    int port = std::stoi(argv[2]);

    // Resolve hostname
    struct hostent* server = gethostbyname(hostname);
    if (!server) {
        std::cerr << "Error: No such host.\n";
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    std::cout << "Connected to supplier_molecule at " << hostname << ":" << port << std::endl;
    std::cout << "Enter molecule requests (e.g., DELIVER WATER 2). Ctrl+D to quit.\n";

    std::string line;
    char buffer[BUFFER_SIZE];
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        // Send request
        sendto(sockfd, line.c_str(), line.length(), 0, (sockaddr*)&server_addr, sizeof(server_addr));

        // Receive response
        socklen_t addr_len = sizeof(server_addr);
        ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (sockaddr*)&server_addr, &addr_len);
        if (len < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[len] = '\0';
        std::cout << "Server response: " << buffer;
    }

    close(sockfd);
    return 0;
}
