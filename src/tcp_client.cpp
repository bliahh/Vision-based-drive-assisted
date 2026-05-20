#include "tcp_client.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

TcpServer::TcpServer() : server_fd(-1), client_fd(-1) {}

TcpServer::~TcpServer() {
    closeConnection();
}

bool TcpServer::start(int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[TCP] socket() esuat\n";
        return false;
    }

    int reuse_addr = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    sockaddr_in address{};
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[TCP] bind() esuat\n";
        return false;
    }
    if (listen(server_fd, 1) < 0) {
        std::cerr << "[TCP] listen() esuat\n";
        return false;
    }

    std::cout << "[TCP] Asculta pe portul " << port << "\n";
    return true;
}

bool TcpServer::waitForClient() {
    sockaddr_in client_address{};
    socklen_t address_length = sizeof(client_address);
    client_fd = accept(server_fd, (sockaddr*)&client_address, &address_length);
    if (client_fd < 0) {
        std::cerr << "[TCP] accept() esuat\n";
        return false;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    std::cout << "[TCP] Client conectat\n";
    return true;
}

bool TcpServer::readChunk(std::string& out_chunk) {
    out_chunk.clear();
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes_read > 0) {
        out_chunk.append(buffer, bytes_read);
        return true;
    }
    if (bytes_read == 0) {
        std::cout << "[TCP] Client deconectat\n";
        return false;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return true;  
    }
    std::cerr << "[TCP] Eroare recv()\n";
    return false;
}

void TcpServer::closeConnection() {
    if (client_fd != -1) { close(client_fd); client_fd = -1; }
    if (server_fd != -1) { close(server_fd); server_fd = -1; }
}
