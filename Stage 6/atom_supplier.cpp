// File: atom_supplier.cpp
// Description: Sends atom ADD commands to the warehouse via TCP or UDS (stream)

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

void print_usage(const char* prog) {
    std::cerr << "Usage:\n";
    std::cerr << "  " << prog << " <HOSTNAME> <PORT>       # TCP mode\n";
    std::cerr << "  " << prog << " -f <UDS_SOCKET_PATH>    # UDS stream mode\n";
}

int main(int argc, char* argv[]) {
    int sockfd = -1;

    // UDS mode: ./atom_supplier -f /tmp/socket_path
    if (argc == 3 && std::string(argv[1]) == "-f") {
        std::string uds_path = argv[2];

        sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket (UDS)");
            return 1;
        }

        sockaddr_un server_addr{};
        server_addr.sun_family = AF_UNIX;
        std::strncpy(server_addr.sun_path, uds_path.c_str(), sizeof(server_addr.sun_path) - 1);

        if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect (UDS)");
            close(sockfd);
            return 1;
        }

        std::cout << "Connected to warehouse via UDS: " << uds_path << std::endl;
    }

    // TCP mode: ./atom_supplier <hostname> <port>
    else if (argc == 3) {
        const char* hostname = argv[1];
        int port = 0;

        try {
            port = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "Invalid port number.\n";
            print_usage(argv[0]);
            return 1;
        }

        struct hostent* server = gethostbyname(hostname);
        if (!server) {
            std::cerr << "Error: No such host.\n";
            return 1;
        }

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket (TCP)");
            return 1;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

        if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect (TCP)");
            close(sockfd);
            return 1;
        }

        std::cout << "Connected to atom warehouse at " << hostname << ":" << port << std::endl;
    }

    // Invalid usage
    else {
        print_usage(argv[0]);
        return 1;
    }

    std::cout << "Enter commands (e.g., ADD HYDROGEN 50). Ctrl+D to quit.\n";
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        line += "\n"; // Ensure newline
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
