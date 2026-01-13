#include "NetworkServer.h"
#include <iostream>

bool NetworkServer::InitailizeNetwork()
{
    WSADATA wsa;

    int result = WSAStartup(MAKEWORD(2, 2), &wsa);

    if (result != 0)
    {
        switch (result)
        {
        case WSASYSNOTREADY:
            std::cerr << "네트워크 서브 시스템이 준비되어 있지 않습니다." << std::endl;
            break;
        case WSAVERNOTSUPPORTED:
            std::cerr << "요청한 Winsock 버전이 지원하지 않습니다." << std::endl;
            break;
        case WSAEINPROGRESS:
            std::cerr << "차단되는 Winsock 1.1 작업이 진행 중입니다." << std::endl;
            break;
        case WSAEPROCLIM:
            std::cerr << "Winsock 사용 제한(프로세스 수)에 도달했습니다." << std::endl;
            break;
        default:
            std::cerr << "알 수 없는 에러 발생: " << result << std::endl;
        }

        return false;
    }
    return true;
}

bool NetworkServer::Init(uint16_t port)
{
    if (!InitailizeNetwork())
        return false;

    _listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_listenSocket == INVALID_SOCKET)
        return false;

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(_listenSocket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
        return false;
    if (listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR)
        return false;

    _listenEvent = WSACreateEvent();
    WSAEventSelect(_listenSocket, _listenEvent, FD_ACCEPT | FD_CLOSE);
    _sessions.push_back(std::make_shared<NetworkSession>(_listenSocket, _listenEvent));
    return true;
}

void NetworkServer::Run()
{
    std::cout << "Server is Running... " << std::endl;

    while (true)
    {
        std::vector<WSAEVENT> events;
        for (auto &s : _sessions)
            events.push_back(s->hEvent);

        if (events.empty())
            return;

        DWORD index = WSAWaitForMultipleEvents((DWORD)events.size(), &events[0], FALSE, WSA_INFINITE, FALSE);

        if (index == WSA_WAIT_FAILED)
            break;

        if (index == WSA_WAIT_TIMEOUT)
            break;

        int i = index - WSA_WAIT_EVENT_0;
        WSANETWORKEVENTS netEvs;
        WSAEnumNetworkEvents(_sessions[i]->socket, _sessions[i]->hEvent, &netEvs);

        if (netEvs.lNetworkEvents & FD_ACCEPT)
            HandleAccept();
        if (netEvs.lNetworkEvents & FD_READ)
            HandleReceive(i);
        if (netEvs.lNetworkEvents & FD_CLOSE)
            HandleClose(i);
    }
}

void NetworkServer::HandleAccept()
{
    sockaddr_in clientAddr = {};
    int addrLen = sizeof(clientAddr);
    SOCKET clientSock = accept(_listenSocket, (sockaddr *)&clientAddr, &addrLen);

    if (clientSock == INVALID_SOCKET)
    {
        int errCode = WSAGetLastError();
        std::cerr << "Accept Failed: " << errCode << std::endl;
        return;
    }

    if (clientSock != INVALID_SOCKET)
    {
        WSAEVENT clientEv = WSACreateEvent();
        WSAEventSelect(clientSock, clientEv, FD_READ | FD_CLOSE);
        _sessions.push_back(std::make_shared<NetworkSession>(clientSock, clientEv));

        char ip[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clientAddr.sin_addr, ip, INET_ADDRSTRLEN) != nullptr)
        {
            std::cout << "[Aceept] Client Connected " << ip << " (Total: " << _sessions.size() - 1 << ")" << std::endl;
        }
    }
}

void NetworkServer::HandleReceive(int index)
{
    PacketHeader header;
    int headerLen = recv(_sessions[index]->socket, (char *)&header, sizeof(header), 0);

    if (headerLen <= 0)
    {
        HandleClose(index);
        return;
    }

    int payloadSize = header.size - sizeof(PacketHeader);
    std::vector<char> payload(payloadSize);
    recv(_sessions[index]->socket, payload.data(), payloadSize, 0);

    PacketReader reader(payload.data());

    int32_t playerId = reader.Read<int32_t>();
    int msgLen = payloadSize - sizeof(int32_t);
    std::string receivedMsg = reader.ReadString(msgLen);

    std::cout << "Player: " << playerId << ": " << receivedMsg << std::endl;

    std::vector<char> fullPacket;
    fullPacket.reserve(header.size);

    char *hPtr = reinterpret_cast<char *>(&header);
    fullPacket.insert(fullPacket.end(), hPtr, hPtr + sizeof(PacketHeader));

    fullPacket.insert(fullPacket.end(), payload.begin(), payload.end());

    std::cout << "fullPacket.size(): " << fullPacket.size() << std::endl;

    Broadcast(fullPacket.data(), (int)fullPacket.size(), index);
}

void NetworkServer::SendChat(int index, int32_t id, const std::string &msg)
{

    uint16_t msgLen = static_cast<uint16_t>(msg.length());
    uint16_t totalSize = sizeof(PacketHeader) + sizeof(int32_t) + msgLen;

    std::vector<char> buffer(totalSize);

    PacketWriter writer(buffer.data());

    PacketHeader h = {totalSize, PacketType::CHAT};

    writer.Write<PacketHeader>(h);
    writer.Write<int32_t>(id);
    writer.WriteString(msg);

    send(_sessions[index]->socket, buffer.data(), totalSize, 0);
}

void NetworkServer::HandleClose(int index)
{

    closesocket(_sessions[index]->socket);
    WSACloseEvent(_sessions[index]->hEvent);

    _sessions.erase(_sessions.begin() + index);
    std::cout << "[Disconnect] Client index " << index << " Left." << std::endl;
}

void NetworkServer::Broadcast(const char *data, int size, int exceptIndex)
{
    for (size_t i = 1; i < _sessions.size(); ++i)
    {
        if (i == (size_t)exceptIndex)
            continue;

        if (send(_sessions[i]->socket, data, size, 0) == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK)
            {
                std::cerr << "Send Error: " << err << std::endl;
            }
        }
    }
}

void NetworkServer::Stop()
{
    _sessions.clear();
    WSACleanup();
}