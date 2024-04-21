#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <map>
#include <mutex>
#include <thread>

#include "Constants.h"
#include "Config.h"
#include "PacketsTypes.h"

using ClientUniqueID = std::pair<sockaddr_in, socklen_t>;

bool operator<(const ClientUniqueID &lhs, const ClientUniqueID &rhs);

class UdpServer
{
private:
    struct sClientData
    {
        std::vector<PacketBase<eServerMessageType>*> clientPackets;
        std::chrono::time_point<std::chrono::steady_clock> lastActiveTimeStamp;
    };

    std::unique_ptr<Config> mServerConfig;

    int mSocket;
    uint8_t mBuffer[MAX_BUFFER_SIZE];
    std::mutex mClientsMutex;
    std::map<ClientUniqueID, sClientData> mClientsData;
    
    std::thread mResendThread;
    ProtocolVersionType mServerProtocolVersion;

private:
    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;

public:
    UdpServer();
    ~UdpServer();

    bool openSocket();
    void startSocket();

    void processRequest(ClientUniqueID aClientUniqueID, uint8_t* aBuffer, ssize_t aBytesAvailable);
    void makeClientData(ClientUniqueID aClientUniqueID, double aMaxRangeValue);
    void sendPacketToClient(ClientUniqueID aClientUniqueID, const PacketBase<eServerMessageType>* aPacket);

    void backgroundResending();

};