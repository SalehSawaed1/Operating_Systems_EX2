// Stage 3: atom_warehouse.cpp
// Fully compliant: handles TCP + UDP + console input to simulate drinks + accept TCP ADD commands

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <map>
#include <algorithm>
#include <vector>
#include <fcntl.h>
#include <sstream>


#define BUFFER_SIZE 1024

std::map<std::string, int> atoms = {
    {"HYDROGEN", 0},
    {"OXYGEN", 0},
    {"CARBON", 0}
};

std::map<std::string, int> molecules = {
    {"WATER", 0},            // H2O: needs 2 H, 1 O
    {"CARBON DIOXIDE", 0},  // CO2: needs 1 C, 2 O
    {"ALCOHOL", 0},         // C2H6O: needs 2 C, 6 H, 1 O
    {"GLUCOSE", 0}          // C6H12O6
};

std::vector<int> tcp_clients;

void print_atoms() {
    std::cout << "Current atom counts: ";
    for (const auto& kv : atoms) {
        std::cout << kv.first << ": " << kv.second << "  ";
    }
    std::cout << std::endl;
}

void handle_tcp_command(int client_sock) {
    char buffer[BUFFER_SIZE];
    ssize_t len = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (len <= 0) {
        close(client_sock);
        tcp_clients.erase(std::remove(tcp_clients.begin(), tcp_clients.end(), client_sock), tcp_clients.end());
        return;
    }

    buffer[len] = '\0';
    std::string cmd(buffer);
    std::string type;
    int amount;

    if (sscanf(buffer, "ADD %s %d", buffer, &amount) == 2) {
        type = std::string(buffer);
        if (atoms.count(type)) {
            atoms[type] += amount;
            std::cout << "[TCP] Added " << amount << " of " << type << std::endl;
        } else {
            std::cout << "[TCP] Invalid atom type: " << type << std::endl;
        }
    } else {
        std::cout << "[TCP] Invalid command" << std::endl;
    }
    print_atoms();
}

void handle_udp_command(int udp_sock) {
    char buffer[BUFFER_SIZE];
    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);

    ssize_t len = recvfrom(udp_sock, buffer, BUFFER_SIZE - 1, 0,
                           (sockaddr*)&client_addr, &addrlen);
    if (len <= 0) return;

    buffer[len] = '\0';
    std::string cmd(buffer);
    std::string molecule;
    int count = 1; // default

    // Extract command and count (if provided)
    std::istringstream iss(cmd);
    std::string deliver_word;
    iss >> deliver_word;
    std::getline(iss, molecule);
    molecule.erase(0, molecule.find_first_not_of(" ")); // remove leading space

    size_t pos = molecule.find_last_of(" ");
    if (pos != std::string::npos && isdigit(molecule[pos + 1])) {
        try {
            count = std::stoi(molecule.substr(pos + 1));
            molecule = molecule.substr(0, pos); // remove number part
        } catch (...) {
            count = 1;
        }
    }

    int delivered = 0;

    for (int i = 0; i < count; ++i) {
        if (molecule == "WATER") {
            if (atoms["HYDROGEN"] >= 2 && atoms["OXYGEN"] >= 1) {
                atoms["HYDROGEN"] -= 2;
                atoms["OXYGEN"] -= 1;
                molecules["WATER"]++;
                delivered++;
            } else break;
        } else if (molecule == "CARBON DIOXIDE") {
            if (atoms["CARBON"] >= 1 && atoms["OXYGEN"] >= 2) {
                atoms["CARBON"] -= 1;
                atoms["OXYGEN"] -= 2;
                molecules["CARBON DIOXIDE"]++;
                delivered++;
            } else break;
        } else if (molecule == "ALCOHOL") {
            if (atoms["CARBON"] >= 2 && atoms["HYDROGEN"] >= 6 && atoms["OXYGEN"] >= 1) {
                atoms["CARBON"] -= 2;
                atoms["HYDROGEN"] -= 6;
                atoms["OXYGEN"] -= 1;
                molecules["ALCOHOL"]++;
                delivered++;
            } else break;
        } else if (molecule == "GLUCOSE") {
            if (atoms["CARBON"] >= 6 && atoms["HYDROGEN"] >= 12 && atoms["OXYGEN"] >= 6) {
                atoms["CARBON"] -= 6;
                atoms["HYDROGEN"] -= 12;
                atoms["OXYGEN"] -= 6;
                molecules["GLUCOSE"]++;
                delivered++;
            } else break;
        } else {
            std::string err = "Invalid command";
            sendto(udp_sock, err.c_str(), err.size(), 0, (sockaddr*)&client_addr, addrlen);
            return;
        }
    }

    if (delivered > 0) {
        std::string ok = "OK " + std::to_string(delivered);
        sendto(udp_sock, ok.c_str(), ok.size(), 0, (sockaddr*)&client_addr, addrlen);
    } else {
        sendto(udp_sock, "FAILED", 6, 0, (sockaddr*)&client_addr, addrlen);
    }
}


void handle_console_command(const std::string& input) {
    if (input.find("GEN SOFT DRINK") == 0) {
        int count = std::min({molecules["WATER"], molecules["CARBON DIOXIDE"], molecules["GLUCOSE"]});
        std::cout << "You can make " << count << " SOFT DRINK(s)\n";
    } else if (input.find("GEN VODKA") == 0) {
        int count = std::min({molecules["WATER"], molecules["ALCOHOL"], molecules["GLUCOSE"]});
        std::cout << "You can make " << count << " VODKA(s)\n";
    } else if (input.find("GEN CHAMPAGNE") == 0) {
        int count = std::min({molecules["WATER"], molecules["CARBON DIOXIDE"], molecules["ALCOHOL"]});
        std::cout << "You can make " << count << " CHAMPAGNE(s)\n";
    } else {
        std::cout << "Unknown command.\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <TCP_PORT> <UDP_PORT>\n";
        return 1;
    }

    int tcp_port = std::atoi(argv[1]);
    int udp_port = std::atoi(argv[2]);

    // Setup TCP socket
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in tcp_addr{};
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(tcp_port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    bind(tcp_sock, (sockaddr*)&tcp_addr, sizeof(tcp_addr));
    listen(tcp_sock, 5);

    // Setup UDP socket
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in udp_addr{};
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(udp_port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    bind(udp_sock, (sockaddr*)&udp_addr, sizeof(udp_addr));

    std::cout << "Atom Warehouse (Stage 3) started.\n";

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(tcp_sock, &read_fds);
        FD_SET(udp_sock, &read_fds);

        int max_fd = std::max({STDIN_FILENO, tcp_sock, udp_sock});
        for (int client : tcp_clients) {
            FD_SET(client, &read_fds);
            max_fd = std::max(max_fd, client);
        }

        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input[256];
            if (fgets(input, sizeof(input), stdin)) {
                std::string command(input);
                command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());
                handle_console_command(command);
            }
        }

        if (FD_ISSET(tcp_sock, &read_fds)) {
            int new_client = accept(tcp_sock, nullptr, nullptr);
            tcp_clients.push_back(new_client);
        }

        if (FD_ISSET(udp_sock, &read_fds)) {
            handle_udp_command(udp_sock);
        }

        for (int client : tcp_clients) {
            if (FD_ISSET(client, &read_fds)) {
                handle_tcp_command(client);
            }
        }
    }

    close(tcp_sock);
    close(udp_sock);
    return 0;
}