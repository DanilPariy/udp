#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <map>

#include "Constants.h"

using ClientUniqueID = std::pair<sockaddr_in, socklen_t>;
using ClientData = std::vector<std::pair<sDoublesPacket, bool>>; // vector< pair<packet, isReceived> >

bool operator<(const ClientUniqueID &lhs, const ClientUniqueID &rhs);

class ServerManager
{
private:
    int mSocket;
    uint8_t mBuffer[MAX_BUFFER_SIZE];
    std::map<ClientUniqueID, ClientData> mClientsData;

private:
    ServerManager();
    ServerManager(const ServerManager&);

public:
    ~ServerManager();

    static ServerManager* getInstance();

    bool openSocket();
    void startSocket();

    void processRequest(ClientUniqueID aClientUniqueID, uint8_t* aBuffer, ssize_t aBytesAvailable);
    void makeClientData(ClientUniqueID aClientUniqueID, double aMaxRangeValue);
    void sendPacketToClient(ClientUniqueID aClientUniqueID, unsigned aPacketIndex);

};