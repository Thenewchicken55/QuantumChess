#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <functional>
#include <thread>
#include <array>

enum class NetworkRole {
    None,
    Host,
    Client
};

enum class NetMsgType {
    None,
    Move,
    Quantum,
    Collapse,
    Miss,
    Chat,
    Disconnect
};

struct NetMessage {
    NetMsgType type = NetMsgType::None;
    int data[6] = {-1, -1, -1, -1, -1, -1};
    std::string text;
};

class NetworkManager {
    int listenSocket = -1;
    int clientSocket = -1;
    NetworkRole role = NetworkRole::None;
    std::thread listenThread;
    bool running = false;

    // Per-instance receive buffer. Was previously a function-static buffer,
    // which is shared across instances and not thread-safe.
    std::array<char, 4096> partialBuf{};
    int partialLen = 0;

    int connectSocket(const char* host, int port);

public:
    NetworkManager() = default;
    ~NetworkManager();

    bool startHost(int port = 5555);
    bool connectToHost(const char* host, int port = 5555);
    void disconnect();
    bool isConnected() const;
    NetworkRole getRole() const { return role; }

    bool sendMessage(const NetMessage& msg);
    NetMessage receiveMessage();
};

#endif
