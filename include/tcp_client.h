#pragma once
#include <string>

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    bool start(int port);
    bool waitForClient();
    bool readChunk(std::string& out_chunk);
    void closeConnection();

private:
    int server_fd;
    int client_fd;
};
