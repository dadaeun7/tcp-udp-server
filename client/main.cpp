#include <thread>
#include <string>
#include "../Common.h"
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

void ClientRun(SOCKET serverSocket, WSAEVENT hEvent)
{
    while (true)
    {
        WSAWaitForMultipleEvents(1, &hEvent, FALSE, WSA_INFINITE, FALSE);

        WSANETWORKEVENTS netEvs;
        WSAEnumNetworkEvents(serverSocket, hEvent, &netEvs);

        if (netEvs.lNetworkEvents & FD_READ)
        {
            PacketHeader header;

            int recvLen = recv(serverSocket, (char *)&header, sizeof(header), 0);
            if (recvLen <= 0)
                break;

            int payloadSize = header.size - sizeof(PacketHeader);

            if (payloadSize <= 0)
                return;

            std::vector<char> payload(payloadSize);
            recv(serverSocket, payload.data(), payloadSize, 0);

            PacketReader reader(payload.data());
            int32_t senderId = reader.Read<int32_t>();
            int msgLen = payloadSize - sizeof(int32_t);
            std::string msg = reader.ReadString(msgLen);

            std::cout << "[받은 메시지] 유저아이디: " << senderId << " >> " << msg << std::endl;
            std::cout << "입력: ";
        }

        if (netEvs.lNetworkEvents & FD_CLOSE)
        {
            std::cout << "\n 서버와의 연결이 끊어졌습니다." << std::endl;
            break;
        }
    }
}

void SendSocket(SOCKET serverSocket, std::vector<char>& buffer){
    send(serverSocket, buffer.data(),(int)buffer.size(), 0);
}
void ClientSend(SOCKET serverSocket, int32_t playerId, std::string& msg){
    std::vector<char> buffer;

    uint16_t totalSize = sizeof(PacketHeader) + sizeof(int32_t) + msg.length();
    PacketWriter write(buffer.data());
    write.Write<PacketHeader>({totalSize, PacketType::CHAT});
    write.Write<int32_t>(playerId);
    write.WriteString(msg);

    SendSocket(serverSocket, buffer);
}

void ClientExit(SOCKET serverSocket, int32_t playerId)
{
    std::vector<char> buffer;
    PacketWriter write(buffer.data());

    uint16_t totalSize = sizeof(PacketHeader) + sizeof(int32_t);


    write.Write<PacketHeader>({totalSize, PacketType::CHATEXIT});
    write.Write(playerId);

    SendSocket(serverSocket, buffer);
}

void ClientIn(SOCKET serverSocket, int32_t playerId)
{
    std::vector<char> buffer;
    uint16_t totalSize = sizeof(PacketHeader) + sizeof(int32_t);
    
    PacketWriter write(buffer.data());
    write.Write<PacketHeader>({totalSize, PacketType::CHATIN});
    write.Write<int32_t>(playerId);

    SendSocket(serverSocket, buffer);
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "서버 접속 실패" << std::endl;
        return -1;
    }

    WSAEVENT hEvent = WSACreateEvent();
    WSAEventSelect(serverSocket, hEvent, FD_READ | FD_CLOSE);

    int32_t myId;
    std::cout << "사용할 고유 ID를 입력하세요: ";
    std::cin >> myId;
    std::cin.ignore();

    std::thread recvThread(ClientRun, serverSocket, hEvent);

    /* 입장 Message 발송 */
    ClientIn(serverSocket, myId);

    while (true)
    {
        std::string input;
        std::cout << "입력: ";
        std::getline(std::cin, input);

        if (input == "exit")
            /* 퇴장 Message 발송 */
            ClientExit(serverSocket, myId);
            break;
        if (input.empty())
            continue;
        /* 일반 채팅 Message 발송 */
        ClientSend(serverSocket, myId, input);
    }

    closesocket(serverSocket);
    WSACloseEvent(hEvent);
    WSACleanup();
    if (recvThread.joinable())
        recvThread.join();

    return 0;
}