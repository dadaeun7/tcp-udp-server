#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <iostream>
#include "Protocol.h"

#pragma comment(lib, "ws2_32.lib")

#define TCP_PORT 9000
#define UDP_PORT 9001

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(TCP_PORT);

    if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        return -1;
    listen(listenSock, SOMAXCONN);

    SOCKET udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in udpAddr = serverAddr;
    udpAddr.sin_port = htons(UDP_PORT);
    bind(udpSock, (sockaddr *)&udpAddr, sizeof(udpAddr));

    u_long mode = 1;
    ioctlsocket(udpSock, FIONBIO, &mode);

    std::vector<SOCKET> clients;
    std::vector<WSAEVENT> clientEvents;

    WSAEVENT listenEvent = WSACreateEvent();
    WSAEventSelect(listenSock, listenEvent, FD_ACCEPT | FD_CLOSE);
    clients.push_back(listenSock);
    clientEvents.push_back(listenEvent);

    while (true)
    {
        DWORD index = WSAWaitForMultipleEvents((DWORD)clientEvents.size(),
                                               &clientEvents[0], FALSE, WSA_INFINITE, FALSE);

        if (index == WSA_WAIT_FAILED)
            break;

        int i = index - WSA_WAIT_EVENT_0;
        WSANETWORKEVENTS networkEvents;
        WSAEnumNetworkEvents(clients[i], clientEvents[i], &networkEvents);

        if (networkEvents.lNetworkEvents & FD_ACCEPT)
        {
            SOCKET clientSocket = accept(clients[i], NULL, NULL);
            WSAEVENT clientEvent = WSACreateEvent();
            WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_CLOSE);
            clients.push_back(clientSocket);
            clientEvents.push_back(clientEvent);
            std::cout << "Client joined via WSAEventSelect\n";
        }

        if (networkEvents.lNetworkEvents & FD_READ)
        {
            char buf[1024];
            int len = recv(clients[i], buf, sizeof(buf), 0);
            if (len > 0)
            {
                PacketHeader *header = (PacketHeader *)buf;

                if (header->type == PacketType::CHAT)
                {
                    ChatPacket *chatPkt = (ChatPacket *)buf;
                    std::cout << "[Chat] Player " << chatPkt->playerId << ": "
                              << chatPkt->message << std::endl;

                    for (size_t j = 0; j < clients.size(); ++j)
                    {
                        if (j == 0 || j == i)
                            continue;

                        int sent = send(clients[j], buf, header->size, 0);
                        if (sent == SOCKET_ERROR)
                        {
                            int errCode = WSAGetLastError();

                            if (errCode == WSAECONNRESET || errCode == WSAECONNABORTED)
                            {
                                std::cerr << "[Error] Client " << clients[j]
                                          << "connection lost. (Code: " << errCode << ")" << std::endl;
                            }
                            else
                            {
                                std::cerr << "[Error] Send failed to " << clients[j] << " (Code: "
                                          << errCode << ")" << std::endl;
                            }

                            closesocket(clients[j]);
                            WSACloseEvent(clientEvents[j]);

                            clients.erase(clients.begin() + j);
                            clientEvents.erase(clientEvents.begin() + j);
                        }
                    }
                }
            }
        }

        if (networkEvents.lNetworkEvents & FD_CLOSE)
        {
            closesocket(clients[i]);
            WSACloseEvent(clientEvents[i]);
        }
    }
}