#pragma once
#include "NetworkSession.h"

class NetworkServer
{
private:
    SOCKET _listenSocket = INVALID_SOCKET;
    WSAEVENT _listenEvent = WSA_INVALID_EVENT;

    std::vector<std::shared_ptr<NetworkSession>> _sessions;

public:
    NetworkServer() = default;
    ~NetworkServer() = default;

    bool Init(uint16_t port);
    void Run();
    void Stop();
    void SendChat(int index, int32_t id, const std::string &msg);

private:
    void HandleAccept();
    void HandleReceive(int index);
    void HandleClose(int index);
    void Broadcast(const char *data, int size, int exceptIndex);
    bool InitailizeNetwork();
};
