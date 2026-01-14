#pragma once
#include <stdint.h>

#define TCP_PORT 9000
#define UDP_PORT 9001

#pragma pack(push, 1)
enum class PacketType : uint16_t
{
    LOGIN = 1,
    CHAT = 2,
    CHATIN = 3,
    CHATEXIT = 4,
    POSITION = 5
};

std::string PacketEnumToString(PacketType type){

    switch(type){
        case PacketType::LOGIN: return "LOGIN";
        case PacketType::CHAT: return "CHAT";
        case PacketType::CHATIN: return "CHATIN";
        case PacketType::CHATEXIT: return "CHATEXIT";
        case PacketType::POSITION: return "POSITION";
        dafault: return "UNKNOWN";
    }

}

struct PacketHeader
{
    uint16_t size;
    PacketType type;
};

struct ChatPacket
{
    PacketHeader header;
    int32_t playerId;
    char message[1];
};

#pragma pack(pop)

class PacketReader
{
public:
    PacketReader(char *buffer) : _ptr(buffer) {}

    template <typename T>
    T Read()
    {
        T value = *reinterpret_cast<T *>(_ptr);
        _ptr += sizeof(T);
        return value;
    }

    std::string ReadString(int len)
    {
        std::string str(_ptr, len);
        _ptr += len;
        return str;
    }

private:
    char *_ptr;
};

class PacketWriter
{

public:
    PacketWriter(char *buffer) : _ptr(buffer) {}

    template <typename T>
    void Write(T value)
    {
        *reinterpret_cast<T *>(_ptr) = value;
        _ptr += sizeof(T);
    }

    void WriteString(const std::string &str)
    {
        memcpy(_ptr, str.c_str(), str.length());
        _ptr += str.length();
    }

private:
    char *_ptr;
};