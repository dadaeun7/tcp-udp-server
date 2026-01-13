#include "../Common.h"
#include "NetworkServer.h"
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    NetworkServer server;

    if (server.Init(TCP_PORT))
    {
        server.Run();
    }
    else
    {
        std::cerr << "서버 초기화 실패" << std::endl;
    }
    return 0;
}