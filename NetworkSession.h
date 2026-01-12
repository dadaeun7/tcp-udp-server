#pragma once
#include "Common.h"
#include <stdint.h>

class NetworkSession
{
public:
    SOCKET socket;
    WSAEVENT hEvent;
    int32_t playerId;

    NetworkSession(SOCKET s, WSAEVENT e) : socket(s), hEvent(e), playerId(0) {}
    ~NetworkSession()
    {
        if (socket != INVALID_SOCKET)
            closesocket(socket);
        if (hEvent != WSA_INVALID_EVENT)
            WSACloseEvent(hEvent);
    }
};