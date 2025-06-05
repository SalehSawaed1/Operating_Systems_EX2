// File: molecule_supplier.cpp
// Description: Stage 2 - Combined TCP + UDP server for atom management and molecule delivery
// Usage: ./molecule_supplier <TCP_PORT> <UDP_PORT>

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>

constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_CLIENTS = 100;

struct AtomStock {
    unsigned int carbon = 0;
    unsigned int oxygen = 0;
    unsigned int hydrogen = 0;

    bool consume(const std::string& molecule, unsigned int count) {
        unsigned int c = 0, o = 0, h = 0;
        if (molecule == "WATER") { h = 2 * count; o = 1 * count; }
        else if (molecule == "CARBON DIOXIDE") { c = 1 * count; o = 2 * count; }
        else if (molecule == "GLUCOSE") { c = 6 * count; h = 12 * count; o = 6 * count; }
        else if (molecule == "ALCOHOL") { c = 2 * count; h = 6 * count; o = 1 * count; }
        else return false;

        if (carbon >= c && hydrogen >= h && oxygen >= o) {
            carbon -= c;
            hydrogen -= h;
            oxygen -= o;
            return true;
        }
        return false;
    }

    void addAtoms(const std::string& type, unsigned int count) {
        if (type == "CARBON") carbon += count;
        else if (type == "OXYGEN") oxygen += count;
        else if (type == "HYDROGEN") hydrogen += count;
    }

    void printStock() const {
        std::cout << "Hydrogen: " << hydrogen << "\n"
                  << "Oxygen: " << oxygen << "\n"
                  << "Carbon: " << carbon << std::endl;
    }
};

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <TCP_PORT> <UDP_PORT>\n";
        return EXIT_FAILURE;
    }

    int tcp_port = std::stoi(argv[1]);
    int udp_port = std::stoi(argv[2]);

    // TCP setup
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) { perror("tcp socket"); return 1; }

    sockaddr_in tcp_addr{};
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(tcp_port);
    int optval = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    if (bind(tcp_sock, (sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("tcp bind"); return 1;
    }

    if (listen(tcp_sock, SOMAXCONN) < 0) {
        perror("tcp listen"); return 1;
    }

    setNonBlocking(tcp_sock);

    // UDP setup
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) { perror("udp socket"); return 1; }

    sockaddr_in udp_addr{};
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(udp_port);

    if (bind(udp_sock, (sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("udp bind"); return 1;
    }

    std::vector<pollfd> fds = {
        {tcp_sock, POLLIN, 0},
        {udp_sock, POLLIN, 0}
    };

    AtomStock stock;
    char buffer[BUFFER_SIZE];

    std::cout << "Server running. TCP port: " << tcp_port << ", UDP port: " << udp_port << std::endl;

    while (true) {
        int activity = poll(fds.data(), fds.size(), -1);
        if (activity < 0) {
            perror("poll");
            break;
        }

        for (size_t i = 0; i < fds.size(); ++i) {
            if (!(fds[i].revents & POLLIN)) continue;

            if (fds[i].fd == tcp_sock) {
                int client_fd = accept(tcp_sock, NULL, NULL);
                if (client_fd >= 0) {
                    setNonBlocking(client_fd);
                    fds.push_back({client_fd, POLLIN, 0});
                }
            } else if (fds[i].fd == udp_sock) {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                ssize_t len = recvfrom(udp_sock, buffer, BUFFER_SIZE - 1, 0,
                                       (sockaddr*)&client_addr, &client_len);
                if (len < 0) continue;

                buffer[len] = '\0';
                std::string cmd(buffer);
                std::string molecule;
                unsigned int amount;
                char moltype[64];

                if (sscanf(cmd.c_str(), "DELIVER %63s %u", moltype, &amount)== 2) {
                    molecule = moltype;
                    if (stock.consume(molecule, amount)) {
                        sendto(udp_sock, "SUCCESS\n", 8, 0, (sockaddr*)&client_addr, client_len);
                        stock.printStock();
                    } else {
                        sendto(udp_sock, "FAILED\n", 7, 0, (sockaddr*)&client_addr, client_len);
                    }
                } else {
                    std::cerr << "Invalid UDP command: " << cmd;
                    sendto(udp_sock, "Invalid command\n", 17, 0, (sockaddr*)&client_addr, client_len);
                }
            } else {
                int client_fd = fds[i].fd;
                ssize_t count = read(client_fd, buffer, BUFFER_SIZE - 1);
                if (count <= 0) {
                    close(client_fd);
                    fds.erase(fds.begin() + i);
                    --i;
                } else {
                    buffer[count] = '\0';
                    std::string cmd(buffer);
                    std::string type;
                    unsigned int amount;
                    if (sscanf(cmd.c_str(), "ADD %s %u", buffer, &amount) == 2) {
                        type = buffer;
                        if (type == "CARBON" || type == "OXYGEN" || type == "HYDROGEN") {
                            stock.addAtoms(type, amount);
                            stock.printStock();
                        } else {
                            std::cerr << "Invalid atom type: " << type << std::endl;
                        }
                    } else {
                        std::cerr << "Invalid TCP command: " << cmd;
                    }
                }
            }
        }
    }

    close(tcp_sock);
    close(udp_sock);
    return 0;
}