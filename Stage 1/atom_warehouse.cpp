// File: src/atom_warehouse.cpp
// Description: Stage 1 - TCP atom warehouse server with I/O multiplexing
// Usage: ./atom_warehouse <TCP_PORT>

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <cstdlib>
#include <cstdint>  // for uint64_t

constexpr int MAX_CLIENTS = 100;
constexpr int BUFFER_SIZE = 1024;

class AtomWarehouse {
private:
    uint64_t carbon = 0;
    uint64_t oxygen = 0;
    uint64_t hydrogen = 0;

public:
    void addAtoms(const std::string& type, uint64_t count) {
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
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <TCP_PORT>\n";
        return EXIT_FAILURE;
    }

    int port = std::stoi(argv[1]);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    setNonBlocking(server_fd);

    std::vector<pollfd> fds = {{server_fd, POLLIN, 0}};
    AtomWarehouse warehouse;
    char buffer[BUFFER_SIZE];

    std::cout << "Atom warehouse server started on port " << port << std::endl;

    while (true) {
        int activity = poll(fds.data(), fds.size(), -1);
        if (activity < 0) {
            perror("poll");
            break;
        }

        for (size_t i = 0; i < fds.size(); ++i) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd) {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                    if (client_fd >= 0) {
                        setNonBlocking(client_fd);
                        fds.push_back({client_fd, POLLIN, 0});
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

                        char atom_type[BUFFER_SIZE];
                        uint64_t amount;
                        if (sscanf(cmd.c_str(), "ADD %s %lu", atom_type, &amount) == 2){
                            std::string type = atom_type;
                            if (type == "CARBON" || type == "OXYGEN" || type == "HYDROGEN") {
                                warehouse.addAtoms(type, amount);
                                warehouse.printStock();
                            } else {
                                std::cerr << "Invalid atom type: " << type << std::endl;
                            }
                        } else {
                            std::cerr << "Invalid command: " << cmd;
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
