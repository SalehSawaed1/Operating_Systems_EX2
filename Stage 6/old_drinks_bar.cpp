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
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>
#include <signal.h>
#include <getopt.h>
#include <sys/un.h>
#include <filesystem>
#include <fstream>
#define BUFFER_SIZE 1024

void save_inventory_to_file(const std::string& filepath);
void load_inventory_from_file(const std::string& filepath);

// === UDS globals ===
int uds_stream_sock = -1, uds_dgram_sock = -1;
std::string uds_stream_path, uds_dgram_path;
std::string save_file_path;

std::map<std::string, int> atoms = {
    {"HYDROGEN", 0},
    {"OXYGEN", 0},
    {"CARBON", 0}
};

std::map<std::string, int> molecules = {
    {"WATER", 0},
    {"CARBON DIOXIDE", 0},
    {"ALCOHOL", 0},
    {"GLUCOSE", 0}
};

std::vector<int> tcp_clients;
int timeout_seconds = 0;


void print_atoms() {
    std::cout << "Current atom counts: ";
    for (const auto& kv : atoms)
        std::cout << kv.first << ": " << kv.second << "  ";
    std::cout << std::endl;
}


void handle_sigint(int) {
    std::cout << "\n[EXIT] Caught Ctrl+C, saving inventory..." << std::endl;
    if (!save_file_path.empty()) {
        save_inventory_to_file(save_file_path);
    }
    exit(0);
}


void timeout_handler(int) {
    std::cout << "\n[TIMEOUT] No activity received within " << timeout_seconds << " seconds. Shutting down.\n";
    exit(0);
}

void reset_alarm() {
    if (timeout_seconds > 0) alarm(timeout_seconds);
}

void save_inventory_to_file(const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out) {
        std::cerr << "[ERROR] Failed to open file for saving: " << filepath << std::endl;
        return;
    }
    for (const auto& pair : atoms) out << pair.first << " " << pair.second << "\n";
    for (const auto& pair : molecules) out << pair.first << " " << pair.second << "\n";
    out.close();
    std::cout << "[INFO] Inventory saved to: " << filepath << std::endl;
}

void load_inventory_from_file(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in) return;
    std::string key;
    int value;
    while (in >> key >> value) {
        if (atoms.count(key)) atoms[key] = value;
        else if (molecules.count(key)) molecules[key] = value;
    }
    std::cout << "[INFO] Inventory loaded from: " << filepath << std::endl;
}

void handle_tcp_command(int client_sock) {
    char buffer[BUFFER_SIZE];
    ssize_t len = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (len <= 0) {
        close(client_sock);
        tcp_clients.erase(std::remove(tcp_clients.begin(), tcp_clients.end(), client_sock), tcp_clients.end());
        return;
    }

    reset_alarm();

    buffer[len] = '\0';
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
        std::cout << "[TCP] Invalid command\n";
    }

    print_atoms();
}

void handle_udp_command(int udp_sock) {
    char buffer[BUFFER_SIZE];
    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);

    ssize_t len = recvfrom(udp_sock, buffer, BUFFER_SIZE - 1, 0, (sockaddr*)&client_addr, &addrlen);
    if (len <= 0) return;

    reset_alarm();

    buffer[len] = '\0';
    std::string cmd(buffer);
    std::string molecule;
    int count = 1;

    std::istringstream iss(cmd);
    std::string deliver_word;
    iss >> deliver_word;
    std::getline(iss, molecule);
    molecule.erase(0, molecule.find_first_not_of(" "));

    size_t pos = molecule.find_last_of(" ");
    if (pos != std::string::npos && isdigit(molecule[pos + 1])) {
        try {
            count = std::stoi(molecule.substr(pos + 1));
            molecule = molecule.substr(0, pos);
        } catch (...) {}
    }

    int delivered = 0;
    for (int i = 0; i < count; ++i) {
        if (molecule == "WATER" && atoms["HYDROGEN"] >= 2 && atoms["OXYGEN"] >= 1) {
            atoms["HYDROGEN"] -= 2;
            atoms["OXYGEN"] -= 1;
            molecules["WATER"]++;
            delivered++;
        } else if (molecule == "CARBON DIOXIDE" && atoms["CARBON"] >= 1 && atoms["OXYGEN"] >= 2) {
            atoms["CARBON"] -= 1;
            atoms["OXYGEN"] -= 2;
            molecules["CARBON DIOXIDE"]++;
            delivered++;
        } else if (molecule == "ALCOHOL" && atoms["CARBON"] >= 2 && atoms["HYDROGEN"] >= 6 && atoms["OXYGEN"] >= 1) {
            atoms["CARBON"] -= 2;
            atoms["HYDROGEN"] -= 6;
            atoms["OXYGEN"] -= 1;
            molecules["ALCOHOL"]++;
            delivered++;
        } else if (molecule == "GLUCOSE" && atoms["CARBON"] >= 6 && atoms["HYDROGEN"] >= 12 && atoms["OXYGEN"] >= 6) {
            atoms["CARBON"] -= 6;
            atoms["HYDROGEN"] -= 12;
            atoms["OXYGEN"] -= 6;
            molecules["GLUCOSE"]++;
            delivered++;
        } else {
            break;
        }
    }

    if (delivered > 0) {
        std::string ok = "OK " + std::to_string(delivered);
        sendto(udp_sock, ok.c_str(), ok.size(), 0, (sockaddr*)&client_addr, addrlen);
        std::cout << "[UDP] Delivered " << delivered << " of " << molecule << std::endl;
    } else {
        sendto(udp_sock, "FAILED", 6, 0, (sockaddr*)&client_addr, addrlen);
        std::cout << "[UDP] FAILED to deliver molecule: " << molecule << std::endl;
    }

    print_atoms();
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


void handle_uds_stream_command(int client_sock) {
    char buffer[BUFFER_SIZE];
    ssize_t len = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (len <= 0) {
        close(client_sock);
        return;
    }
    reset_alarm();
    buffer[len] = '\0';
    std::string type;
    int amount;
    if (sscanf(buffer, "ADD %s %d", buffer, &amount) == 2) {
        type = std::string(buffer);
        if (atoms.count(type)) {
            atoms[type] += amount;
            std::cout << "[UDS-STREAM] Added " << amount << " of " << type << std::endl;
        } else {
            std::cout << "[UDS-STREAM] Invalid atom type: " << type << std::endl;
        }
    } else {
        std::cout << "[UDS-STREAM] Invalid command\n";
    }
    print_atoms();
}

void handle_uds_dgram_command() {
    char buffer[BUFFER_SIZE];
    sockaddr_un client_addr;
    socklen_t addrlen = sizeof(client_addr);
    ssize_t len = recvfrom(uds_dgram_sock, buffer, BUFFER_SIZE - 1, 0, (sockaddr*)&client_addr, &addrlen);
    if (len <= 0) return;

    reset_alarm();

    buffer[len] = '\0';
    std::string cmd(buffer);
    std::string molecule;
    int count = 1;

    std::istringstream iss(cmd);
    std::string deliver_word;
    iss >> deliver_word;
    std::getline(iss, molecule);
    molecule.erase(0, molecule.find_first_not_of(" "));
    size_t pos = molecule.find_last_of(" ");
    if (pos != std::string::npos && isdigit(molecule[pos + 1])) {
        try {
            count = std::stoi(molecule.substr(pos + 1));
            molecule = molecule.substr(0, pos);
        } catch (...) {}
    }

    int delivered = 0;
    for (int i = 0; i < count; ++i) {
        if (molecule == "WATER" && atoms["HYDROGEN"] >= 2 && atoms["OXYGEN"] >= 1) {
            atoms["HYDROGEN"] -= 2;
            atoms["OXYGEN"] -= 1;
            molecules["WATER"]++;
            delivered++;
        } else if (molecule == "CARBON DIOXIDE" && atoms["CARBON"] >= 1 && atoms["OXYGEN"] >= 2) {
            atoms["CARBON"] -= 1;
            atoms["OXYGEN"] -= 2;
            molecules["CARBON DIOXIDE"]++;
            delivered++;
        } else if (molecule == "ALCOHOL" && atoms["CARBON"] >= 2 && atoms["HYDROGEN"] >= 6 && atoms["OXYGEN"] >= 1) {
            atoms["CARBON"] -= 2;
            atoms["HYDROGEN"] -= 6;
            atoms["OXYGEN"] -= 1;
            molecules["ALCOHOL"]++;
            delivered++;
        } else if (molecule == "GLUCOSE" && atoms["CARBON"] >= 6 && atoms["HYDROGEN"] >= 12 && atoms["OXYGEN"] >= 6) {
            atoms["CARBON"] -= 6;
            atoms["HYDROGEN"] -= 12;
            atoms["OXYGEN"] -= 6;
            molecules["GLUCOSE"]++;
            delivered++;
        } else {
            break;
        }
    }

    if (delivered > 0) {
        std::string ok = "OK " + std::to_string(delivered);
        sendto(uds_dgram_sock, ok.c_str(), ok.size(), 0, (sockaddr*)&client_addr, addrlen);
        std::cout << "[UDS-DGRAM] Delivered " << delivered << " of " << molecule << std::endl;
    } else {
        sendto(uds_dgram_sock, "FAILED", 6, 0, (sockaddr*)&client_addr, addrlen);
        std::cout << "[UDS-DGRAM] FAILED to deliver molecule: " << molecule << std::endl;
    }
    print_atoms();
}

int main(int argc, char* argv[]) {
    int tcp_port = -1, udp_port = -1;
    int opt;

    static struct option long_options[] = {
        {"timeout", required_argument, nullptr, 't'},
        {"tcp-port", required_argument, nullptr, 'T'},
        {"udp-port", required_argument, nullptr, 'U'},
        {"oxygen", required_argument, nullptr, 'o'},
        {"carbon", required_argument, nullptr, 'c'},
        {"hydrogen", required_argument, nullptr, 'h'},
        {"stream-path", required_argument, nullptr, 's'},
        {"datagram-path", required_argument, nullptr, 'd'},
        {"save-file", required_argument, nullptr, 'f'},
        {nullptr, 0, nullptr, 0}
    };

    while ((opt = getopt_long(argc, argv, "t:T:U:o:c:h:s:d:f:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 't': timeout_seconds = std::atoi(optarg); break;
            case 'T': tcp_port = std::atoi(optarg); break;
            case 'U': udp_port = std::atoi(optarg); break;
            case 'o': atoms["OXYGEN"] = std::atoi(optarg); break;
            case 'c': atoms["CARBON"] = std::atoi(optarg); break;
            case 'h': atoms["HYDROGEN"] = std::atoi(optarg); break;
            case 's': uds_stream_path = optarg; break;
            case 'd': uds_dgram_path = optarg; break;
            case 'f':
                save_file_path = optarg;
                load_inventory_from_file(save_file_path);
                break;
            default:
                std::cerr << "Usage: " << argv[0]
                          << " -T <tcp_port> -U <udp_port> [-t timeout] [-o O] [-c C] [-h H] [-s stream_path] [-d dgram_path] [-f save_file]\n";
                return 1;
        }
    }

    signal(SIGALRM, timeout_handler);
    signal(SIGINT, handle_sigint);
    reset_alarm();

    // TCP
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in tcp_addr{};
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(tcp_port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    bind(tcp_sock, (sockaddr*)&tcp_addr, sizeof(tcp_addr));
    listen(tcp_sock, 5);

    // UDP
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in udp_addr{};
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(udp_port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    bind(udp_sock, (sockaddr*)&udp_addr, sizeof(udp_addr));

    // UDS STREAM
    if (!uds_stream_path.empty()) {
        uds_stream_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        sockaddr_un stream_addr{};
        stream_addr.sun_family = AF_UNIX;
        strncpy(stream_addr.sun_path, uds_stream_path.c_str(), sizeof(stream_addr.sun_path) - 1);
        unlink(stream_addr.sun_path);
        bind(uds_stream_sock, (sockaddr*)&stream_addr, sizeof(stream_addr));
        listen(uds_stream_sock, 5);
    }

    // UDS DGRAM
    if (!uds_dgram_path.empty()) {
        uds_dgram_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
        sockaddr_un dgram_addr{};
        dgram_addr.sun_family = AF_UNIX;
        strncpy(dgram_addr.sun_path, uds_dgram_path.c_str(), sizeof(dgram_addr.sun_path) - 1);
        unlink(dgram_addr.sun_path);
        bind(uds_dgram_sock, (sockaddr*)&dgram_addr, sizeof(dgram_addr));
    }

    std::cout << "Atom Warehouse (Stage 6) started.\n";
    print_atoms();

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(tcp_sock, &read_fds);
        FD_SET(udp_sock, &read_fds);
        if (uds_stream_sock != -1) FD_SET(uds_stream_sock, &read_fds);
        if (uds_dgram_sock != -1) FD_SET(uds_dgram_sock, &read_fds);
        int max_fd = std::max({STDIN_FILENO, tcp_sock, udp_sock, uds_stream_sock, uds_dgram_sock});

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
                reset_alarm();
            }
        }

        if (FD_ISSET(tcp_sock, &read_fds)) {
            int new_client = accept(tcp_sock, nullptr, nullptr);
            if (new_client >= 0) tcp_clients.push_back(new_client);
        }

        if (FD_ISSET(udp_sock, &read_fds)) {
            handle_udp_command(udp_sock);
        }

        for (int client : tcp_clients) {
            if (FD_ISSET(client, &read_fds)) {
                handle_tcp_command(client);
            }
        }

        if (uds_stream_sock != -1 && FD_ISSET(uds_stream_sock, &read_fds)) {
            int uds_client = accept(uds_stream_sock, nullptr, nullptr);
            if (uds_client >= 0) {
                handle_uds_stream_command(uds_client);
                close(uds_client);
            }
        }

        if (uds_dgram_sock != -1 && FD_ISSET(uds_dgram_sock, &read_fds)) {
            handle_uds_dgram_command();
        }
    }

    if (!save_file_path.empty()) {
        save_inventory_to_file(save_file_path);
    }

    close(tcp_sock);
    close(udp_sock);
    if (uds_stream_sock != -1) {
        close(uds_stream_sock);
        unlink(uds_stream_path.c_str());
    }
    if (uds_dgram_sock != -1) {
        close(uds_dgram_sock);
        unlink(uds_dgram_path.c_str());
    }

    return 0;
}