//#include <bits/stdc++.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>

#include "Constants.h"
#include "RandomHelper.h"
#include "ConfigManager.h"
#include "BufferParser.h"

#define PORT 8080

bool operator<(const std::pair<sockaddr_in, socklen_t> &lhs, const std::pair<sockaddr_in, socklen_t> &rhs)
{
    // Compare sockaddr_in structures first
    if (lhs.first.sin_family < rhs.first.sin_family) return true;
    if (lhs.first.sin_family > rhs.first.sin_family) return false;

    // Compare sockaddr_in addresses
    if (lhs.first.sin_addr.s_addr < rhs.first.sin_addr.s_addr) return true;
    if (lhs.first.sin_addr.s_addr > rhs.first.sin_addr.s_addr) return false;

    // Compare sockaddr_in ports
    if (lhs.first.sin_port < rhs.first.sin_port) return true;
    if (lhs.first.sin_port > rhs.first.sin_port) return false;

    // If sockaddr_in structures are equal, compare socklen_t
    return lhs.second < rhs.second;
}

using ClientUniqueID = std::pair<sockaddr_in, socklen_t>;
using ClientData = std::vector<std::pair<sDoublesPacket, bool>>; // vector< pair<packet, isReceived> >

class ServerManager
{
private:
    int mSocket;
    uint8_t mBuffer[MAX_BUFFER_SIZE];
    std::map<ClientUniqueID, ClientData> mClientsData;

private:
    ServerManager()
    {
    }
    ServerManager(const ServerManager&);

public:
    ~ServerManager()
    {
        if (mSocket)
        {
            close(mSocket);
        }
    }

    static ServerManager* getInstance()
    {
        static ServerManager instance;
        return &instance;
    }

    bool openSocket()
    {
        mSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (mSocket < 0)
        {
            return false;
        }

        sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(PORT);
    
        if (bind(mSocket, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) < 0)
        {
            return false;
        }

        return true;
    }

    void startSocket()
    {
        sockaddr_in cliaddr;
        memset(&cliaddr, 0, sizeof(cliaddr));
        socklen_t len = sizeof(cliaddr);

        while (true)
        {
            auto bytesReceived = recvfrom(mSocket, mBuffer, MAX_BUFFER_SIZE, MSG_WAITALL, reinterpret_cast<sockaddr*>(&cliaddr), &len);
            processRequest(std::make_pair(cliaddr, len), mBuffer, bytesReceived);
        }
    }

    void processRequest(ClientUniqueID aClientUniqueID, uint8_t* aBuffer, ssize_t aBytesAvailable)
    {
        auto parseResult = BufferParser::parseBuffer<uint8_t>(aBuffer, aBytesAvailable);

        auto packetTypeCasted = static_cast<ePacketType>(parseResult.first);
        auto bytesRead = parseResult.second;

        aBuffer += bytesRead;
        aBytesAvailable -= bytesRead;

        switch (packetTypeCasted)
        {
            case ePacketType::CLIENT_AKS_DOUBLES:
            {
                auto askDoublesResult = BufferParser::parseBuffer<double>(aBuffer, aBytesAvailable);
                makeClientData(aClientUniqueID, askDoublesResult.first);
                auto findIt = mClientsData.find(aClientUniqueID);
                if (findIt != mClientsData.end())
                {
                    for (auto i = 0; i < findIt->second.size(); i++)
                    {
                        sendPacketToClient(aClientUniqueID, i);
                    }
                }
                break;
            }
            case ePacketType::CLIENT_SENDS_DOUBLES_PACKET_CONFIRMATION:
            {
                auto confirmationResult = BufferParser::parseBuffer<uint32_t>(aBuffer, aBytesAvailable);
                break;
            }
            default:
                break;
        }
    }

    void makeClientData(ClientUniqueID aClientUniqueID, double aMaxRangeValue)
    {
        auto findIt = mClientsData.find(aClientUniqueID);
        if (findIt == mClientsData.end())
        {
            ConfigManager::getInstance()->parseConfig("server_config.txt");
            auto configValue = ConfigManager::getInstance()->getConfigValue("doubles_count_to_send");
            auto doublesCount = stoi(configValue.value_or("1000000"));
            for (auto i = 0; i <= doublesCount; i++)
            {
                ClientData& clientData = mClientsData[aClientUniqueID];
                auto addPacket = [&clientData]()
                {
                    clientData.push_back(std::make_pair(sDoublesPacket(), false));
                    clientData.back().first.packetIndex = clientData.size() - 1;
                    clientData.back().first.doubles.reserve(MAX_DOUBLES_COUNT);
                };

                if (clientData.empty())
                {
                    addPacket();
                }
                
                if (clientData.back().first.doubles.size() == MAX_DOUBLES_COUNT)
                {
                    addPacket();
                }

                auto& doubles = clientData.back().first.doubles;
                doubles.push_back(RandomHelper::random<double>(-aMaxRangeValue, aMaxRangeValue));
            }
        }
    }

    void sendPacketToClient(ClientUniqueID aClientUniqueID, unsigned aPacketIndex)
    {
        auto findIt = mClientsData.find(aClientUniqueID);
        if (findIt != mClientsData.end())
        {
            const auto& clientData = findIt->second;
            if (0 <= aPacketIndex && aPacketIndex < clientData.size())
            {
                const auto& packet = clientData[aPacketIndex].first;

                uint8_t* writePtr = mBuffer;

                uint32_t packetIndex = packet.packetIndex;
                memcpy(writePtr, &packetIndex, sizeof(packetIndex));
                writePtr += sizeof(packetIndex);

                for (const auto& d: packet.doubles)
                {
                    memcpy(writePtr, &d, sizeof(d));
                    writePtr += sizeof(d);
                }

                auto toSend = writePtr - mBuffer;
                ssize_t sentBytes = sendto(mSocket, mBuffer, toSend, /*MSG_CONFIRM*/0, reinterpret_cast<sockaddr*>(&aClientUniqueID.first), aClientUniqueID.second);
            }
        }
    }
};

int main()
{
    if (!ServerManager::getInstance()->openSocket())
    {
        perror("socket creation/binding failed");
        exit(EXIT_FAILURE);
    }

    ServerManager::getInstance()->startSocket();
    
    return 0;
}
