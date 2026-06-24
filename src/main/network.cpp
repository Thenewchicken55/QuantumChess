#include "network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <chrono>
#include <thread>

NetworkManager::~NetworkManager() {
    disconnect();
}

static int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool NetworkManager::startHost(int port) {
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) return false;

    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(listenSocket);
        listenSocket = -1;
        return false;
    }

    listen(listenSocket, 1);
    setNonBlocking(listenSocket);
    role = NetworkRole::Host;
    running = true;

    return true;
}

bool NetworkManager::connectToHost(const char* host, int port) {
    clientSocket = connectSocket(host, port);
    if (clientSocket < 0) return false;
    setNonBlocking(clientSocket);
    role = NetworkRole::Client;
    return true;
}

int NetworkManager::connectSocket(const char* host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    hostent* server = gethostbyname(host);
    if (!server) {
        close(sock);
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(port);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

void NetworkManager::disconnect() {
    running = false;
    if (listenThread.joinable())
        listenThread.join();
    if (clientSocket >= 0) {
        close(clientSocket);
        clientSocket = -1;
    }
    if (listenSocket >= 0) {
        close(listenSocket);
        listenSocket = -1;
    }
    role = NetworkRole::None;
}

bool NetworkManager::isConnected() const {
    return clientSocket >= 0;
}

bool NetworkManager::sendMessage(const NetMessage& msg) {
    int fd = clientSocket;
    if (fd < 0) return false;

    char buf[256];
    int n = 0;

    switch (msg.type) {
    case NetMsgType::Move:
        n = snprintf(buf, sizeof(buf), "MOVE|%d|%d|%d|%d\n",
                     msg.data[0], msg.data[1], msg.data[2], msg.data[3]);
        break;
    case NetMsgType::Quantum:
        n = snprintf(buf, sizeof(buf), "QUANTUM|%d|%d|%d|%d|%d|%d\n",
                     msg.data[0], msg.data[1], msg.data[2],
                     msg.data[3], msg.data[4], msg.data[5]);
        break;
    case NetMsgType::Collapse:
        n = snprintf(buf, sizeof(buf), "COLLAPSE|%d|%d\n",
                     msg.data[0], msg.data[1]);
        break;
    case NetMsgType::Miss:
        n = snprintf(buf, sizeof(buf), "MISS\n");
        break;
    case NetMsgType::Chat:
        n = snprintf(buf, sizeof(buf), "CHAT|%s\n", msg.text.c_str());
        break;
    case NetMsgType::Disconnect:
        n = snprintf(buf, sizeof(buf), "DISCONNECT\n");
        break;
    default:
        return false;
    }

    if (n <= 0) return false;
    int sent = send(fd, buf, n, 0);
    return sent == n;
}

NetMessage NetworkManager::receiveMessage() {
    NetMessage msg;

    if (role == NetworkRole::Host) {
        if (clientSocket < 0) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            int newSock = accept(listenSocket, (sockaddr*)&clientAddr, &clientLen);
            if (newSock >= 0) {
                setNonBlocking(newSock);
                clientSocket = newSock;
            }
        }
    }

    int fd = clientSocket;
    if (fd < 0) return msg;

    static char partialBuf[4096];
    static int partialLen = 0;

    int n = recv(fd, partialBuf + partialLen, sizeof(partialBuf) - partialLen - 1, 0);
    if (n <= 0) return msg;

    partialLen += n;
    partialBuf[partialLen] = '\0';

    char* newline = strchr(partialBuf, '\n');
    if (!newline) return msg;

    *newline = '\0';
    int lineLen = (int)(newline - partialBuf);
    std::string line(partialBuf, lineLen);
    memmove(partialBuf, newline + 1, partialLen - lineLen - 1);
    partialLen -= lineLen + 1;

    if (line.compare(0, 5, "MOVE|") == 0) {
        msg.type = NetMsgType::Move;
        sscanf(line.c_str(), "MOVE|%d|%d|%d|%d",
               &msg.data[0], &msg.data[1], &msg.data[2], &msg.data[3]);
    } else if (line.compare(0, 8, "QUANTUM|") == 0) {
        msg.type = NetMsgType::Quantum;
        sscanf(line.c_str(), "QUANTUM|%d|%d|%d|%d|%d|%d",
               &msg.data[0], &msg.data[1], &msg.data[2],
               &msg.data[3], &msg.data[4], &msg.data[5]);
    } else if (line.compare(0, 9, "COLLAPSE|") == 0) {
        msg.type = NetMsgType::Collapse;
        sscanf(line.c_str(), "COLLAPSE|%d|%d", &msg.data[0], &msg.data[1]);
    } else if (line == "MISS") {
        msg.type = NetMsgType::Miss;
    } else if (line == "DISCONNECT") {
        msg.type = NetMsgType::Disconnect;
    }

    return msg;
}
