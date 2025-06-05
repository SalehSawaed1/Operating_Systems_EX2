// File: atom_supplier.cpp
// Description: Stage 1 - Client for sending atom commands to atom_warehouse server
// Usage: ./atom_supplier <HOSTNAME> <PORT>

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <HOSTNAME> <PORT>\n";
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

    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // Server address setup
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // Connect to server
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to atom warehouse at " << hostname << ":" << port << std::endl;
    std::cout << "Enter commands (e.g., ADD HYDROGEN 50). Ctrl+D to quit.\n";

    // Input loop
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        line += "\n"; // Ensure newline is sent
        ssize_t sent = send(sockfd, line.c_str(), line.size(), 0);
        if (sent < 0) {
            perror("send");
            break;
        }
    }

    std::cout << "Disconnected.\n";
    close(sockfd);
    return 0;
}
