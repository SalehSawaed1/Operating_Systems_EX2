// File: molecule_requester.cpp
// Description: Sends molecule requests via UDP or UDS-DGRAM to warehouse

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

constexpr int BUFFER_SIZE = 1024;

void print_usage(const char* prog) {
    std::cerr << "Usage:\n";
    std::cerr << "  " << prog << " <HOSTNAME> <PORT>       # UDP mode\n";
    std::cerr << "  " << prog << " -f <UDS_SOCKET_PATH>    # UDS datagram mode\n";
}

int main(int argc, char* argv[]) {
    int sockfd = -1;
    sockaddr_storage server_addr{};
    socklen_t server_addr_len = 0;
    bool is_uds = false;

    if (argc == 3 && std::string(argv[1]) == "-f") {
        // UDS-DGRAM mode
        is_uds = true;
        std::string uds_path = argv[2];

        sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket (UDS)");
            return 1;
        }

        // Bind to a unique path to receive replies
        std::string client_path = "/tmp/molecule_client_" + std::to_string(getpid());
        sockaddr_un client_addr{};
        client_addr.sun_family = AF_UNIX;
        std::strncpy(client_addr.sun_path, client_path.c_str(), sizeof(client_addr.sun_path) - 1);
        unlink(client_path.c_str());
        if (bind(sockfd, (sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
            perror("bind (UDS client)");
            return 1;
        }

        // Destination
        sockaddr_un* dest = (sockaddr_un*)&server_addr;
        dest->sun_family = AF_UNIX;
        std::strncpy(dest->sun_path, uds_path.c_str(), sizeof(dest->sun_path) - 1);
        server_addr_len = sizeof(sockaddr_un);

        std::cout << "Connected to warehouse via UDS-DGRAM: " << uds_path << std::endl;
    }

    else if (argc == 3) {
        // UDP mode
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

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket (UDP)");
            return 1;
        }

        sockaddr_in* udp_addr = (sockaddr_in*)&server_addr;
        udp_addr->sin_family = AF_INET;
        udp_addr->sin_port = htons(port);
        std::memcpy(&udp_addr->sin_addr.s_addr, server->h_addr, server->h_length);
        server_addr_len = sizeof(sockaddr_in);

        std::cout << "Connected to warehouse via UDP: " << hostname << ":" << port << std::endl;
    }

    else {
        print_usage(argv[0]);
        return 1;
    }

    // Interaction
    std::cout << "Enter molecule requests (e.g., DELIVER WATER 2). Ctrl+D to quit.\n";
    std::string line;
    char buffer[BUFFER_SIZE];

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        // Send
        ssize_t sent = sendto(sockfd, line.c_str(), line.length(), 0,
                              (sockaddr*)&server_addr, server_addr_len);
        if (sent < 0) {
            perror("sendto");
            continue;
        }

        // Receive
        ssize_t len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, nullptr, nullptr);
        if (len < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[len] = '\0';
        std::cout << "Server response: " << buffer << std::endl;
    }

    close(sockfd);
    if (is_uds) {
        std::string client_path = "/tmp/molecule_client_" + std::to_string(getpid());
        unlink(client_path.c_str());
    }

    return 0;
}
