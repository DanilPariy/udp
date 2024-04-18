#include "ServerManager.h"

#include <set>
#include <thread>

#include "RandomHelper.h"
#include "ConfigManager.h"
#include "BufferParser.h"

bool operator<(const ClientUniqueID &lhs, const ClientUniqueID &rhs)
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

ServerManager::ServerManager()
{
}

ServerManager::~ServerManager()
{
    std::lock_guard<std::mutex> guard(mClientsMutex);
    for (auto& [clientUID, clientData]: mClientsData)
    {
        for (auto packet: clientData.clientPackets)
        {
            if (packet)
            {
                delete packet;
            }
        }
        clientData.clientPackets.clear();
    }
    if (mResendThread.joinable())
    {
        mResendThread.join();
    }
    if (mSocket)
    {
        close(mSocket);
    }
}

ServerManager* ServerManager::getInstance()
{
    static ServerManager instance;
    return &instance;
}

bool ServerManager::openSocket()
{
    mSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (mSocket < 0)
    {
        return false;
    }

    ConfigManager::getInstance()->parseConfig("server_config.txt");
    const auto configAddress = ConfigManager::getInstance()->getConfigValue("address");
    auto port = ConfigManager::getInstance()->getConfigValue("port");

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(stoi(port.value_or("8080")));
    auto address = configAddress.value_or("0.0.0.0");
    inet_pton(AF_INET, address.c_str(), &servaddr.sin_addr);

    if (bind(mSocket, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) < 0)
    {
        return false;
    }

    return true;
}

void ServerManager::startSocket()
{
    sockaddr_in cliaddr;
    memset(&cliaddr, 0, sizeof(cliaddr));
    socklen_t len = sizeof(cliaddr);

    mResendThread = std::thread(&ServerManager::backgroundResending, this);

    while (true)
    {
        auto bytesReceived = recvfrom(mSocket, mBuffer, MAX_BUFFER_SIZE, MSG_WAITALL, reinterpret_cast<sockaddr*>(&cliaddr), &len);
        processRequest(std::make_pair(cliaddr, len), mBuffer, bytesReceived);
    }
}

void ServerManager::processRequest(ClientUniqueID aClientUniqueID, uint8_t* aBuffer, ssize_t aBytesAvailable)
{
    PacketBase<eClientMessageType> packetBase;
    ssize_t bytesParsed;
    std::tie(packetBase, bytesParsed) = PacketBase<eClientMessageType>::parseFromBuffer(aBuffer, aBytesAvailable);

    uint8_t* readPtr = aBuffer + bytesParsed;
    aBytesAvailable -= bytesParsed;

    switch (packetBase.getType())
    {
        case eClientMessageType::DOUBLES_RANGE_MAX:
        {
            auto askDoublesResult = BufferParser::parseValueFromBuffer<double>(readPtr, aBytesAvailable);
            makeClientData(aClientUniqueID, askDoublesResult.first);

            std::lock_guard<std::mutex> guard(mClientsMutex);
            auto findIt = mClientsData.find(aClientUniqueID);
            if (findIt != mClientsData.end())
            {
                findIt->second.lastActiveTimeStamp = std::chrono::steady_clock::now();
                for (const auto packet: findIt->second.clientPackets)
                {
                    sendPacketToClient(aClientUniqueID, packet);
                }
            }
            break;
        }
        case eClientMessageType::PACKET_RECEIVED_CONFIRMATION:
        {
            std::lock_guard<std::mutex> guard(mClientsMutex);
            auto packetIndex = packetBase.getPacketID();
            auto findIt = mClientsData.find(aClientUniqueID);
            if (findIt != mClientsData.end())
            {
                findIt->second.lastActiveTimeStamp = std::chrono::steady_clock::now();
                auto& clientPackets = findIt->second.clientPackets;
                if (0 <= packetIndex && packetIndex < clientPackets.size())
                {
                    auto packet = clientPackets[packetIndex];
                    if (packet)
                    {
                        packet->setIsReceived(true);
                    }
                }

                auto findNotReceived = std::find_if(clientPackets.begin(), clientPackets.end(),
                    [](const PacketBase<eServerMessageType>* aPacket){ return aPacket && !aPacket->isReceived(); }
                );
                if (findNotReceived == clientPackets.end())
                {
                    for (auto packet: clientPackets)
                    {
                        if (packet)
                        {
                            delete packet;
                        }
                    }
                    mClientsData.erase(aClientUniqueID);
                }
            }
            break;
        }
        default:
            break;
    }
}

void ServerManager::makeClientData(ClientUniqueID aClientUniqueID, double aMaxRangeValue)
{
    std::lock_guard<std::mutex> guard(mClientsMutex);
    auto findIt = mClientsData.find(aClientUniqueID);
    if (findIt == mClientsData.end())
    {
        auto configValue = ConfigManager::getInstance()->getConfigValue("doubles_count_to_send");
        auto doublesCount = stoi(configValue.value_or("1000000"));

        std::set<double> uniqueDoubles;
        std::vector<double> doubles;
        doubles.reserve(doublesCount);
        while (uniqueDoubles.size() < doublesCount)
        {
            auto value = RandomHelper::random<double>(-aMaxRangeValue, aMaxRangeValue);
            if (uniqueDoubles.insert(value).second)
            {
                doubles.push_back(value);
            }
        }

        std::vector<PacketBase<eServerMessageType>*>& clientData = mClientsData[aClientUniqueID].clientPackets;
        auto addPacket = [&clientData](PacketBase<eServerMessageType>* aPacket)
        {
            if (aPacket)
            {
                aPacket->setPacketID(clientData.size());
                clientData.push_back(aPacket);
            }
        };

        const auto maxDoublesCount = VectorPacket<eServerMessageType, double>::MAX_VECTOR_SIZE;
        auto chunksCount = std::ceil(static_cast<float>(doublesCount)/ maxDoublesCount);

        auto packet = new SingleValuePacket<eServerMessageType, unsigned>();
        if (packet)
        {
            packet->setType(eServerMessageType::PACKETS_COUNT);
            packet->setValue(chunksCount + 1);
            addPacket(packet);
        }

        for (auto chunkIndex = 0; chunkIndex < chunksCount; chunkIndex++)
        {
            std::vector<double> singlePacketDoubles;
            for (auto doubleInChunkIndex = 0; doubleInChunkIndex < maxDoublesCount; doubleInChunkIndex++)
            {
                auto doubleIndex = chunkIndex * maxDoublesCount + doubleInChunkIndex;
                if (0 < doubleIndex && doubleIndex <= doubles.size())
                {
                    singlePacketDoubles.push_back(doubles[doubleIndex]);
                }
            }

            auto packet = new VectorPacket<eServerMessageType, double>();
            if (packet)
            {
                packet->setType(eServerMessageType::DOUBLES_PACKET);
                packet->setValue(std::move(singlePacketDoubles));
                addPacket(packet);
            }
        }
    }
}

void ServerManager::sendPacketToClient(ClientUniqueID aClientUniqueID, const PacketBase<eServerMessageType>* aPacket)
{
    if (aPacket)
    {
        auto bytesWrote = aPacket->writeToBuffer(mBuffer, MAX_BUFFER_SIZE);
        ssize_t sentBytes = sendto(mSocket, mBuffer, bytesWrote, /*MSG_CONFIRM*/0, reinterpret_cast<sockaddr*>(&aClientUniqueID.first), aClientUniqueID.second);
    }
}

void ServerManager::backgroundResending()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> guard(mClientsMutex);
        for (const auto& [clientUID, clientData]: mClientsData)
        {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - clientData.lastActiveTimeStamp);
            if (duration.count() > 100)
            {
                for (const auto packet: clientData.clientPackets)
                {
                    if (packet && !packet->isReceived())
                    {
                        sendPacketToClient(clientUID, packet);
                    }
                }
            }
        }
    }
}