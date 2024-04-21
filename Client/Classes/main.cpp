// Client side implementation of UDP client-server model
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <set>
#include <thread>

#include "Constants.h"
#include "BufferParser.h"
#include "Types.h"
#include "Config.h"
#include "PacketsTypes.h"

Config mClientConfig;

ssize_t sendDoublesRequest(int aSocket, uint8_t* aBuffer, const sockaddr_in& aServerAddress, double aDoublesRangeMax)
{
    SingleValuePacket<eClientMessageType, double> packet;
    packet.setType(eClientMessageType::DOUBLES_RANGE_MAX);
    packet.setValue(aDoublesRangeMax);
    packet.setProtocolVersion(stoi(mClientConfig.getConfigValue("protocol_version").value_or("0")));

    auto bytesWrote = packet.writeToBuffer(aBuffer, MAX_BUFFER_SIZE);
    return sendto(aSocket, aBuffer, bytesWrote, /*MSG_CONFIRM*/0, reinterpret_cast<const sockaddr*>(&aServerAddress), sizeof(aServerAddress));
}

ssize_t sendPacketReceivedConfirmation(int aSocket, uint8_t* aBuffer, const sockaddr_in& aServerAddress, unsigned aPacketID)
{
    PacketBase<eClientMessageType> packet;
    packet.setType(eClientMessageType::PACKET_RECEIVED_CONFIRMATION);
    packet.setPacketID(aPacketID);
    packet.setProtocolVersion(stoi(mClientConfig.getConfigValue("protocol_version").value_or("0")));

    auto bytesWrote = packet.writeToBuffer(aBuffer, MAX_BUFFER_SIZE);
    return sendto(aSocket, aBuffer, bytesWrote, /*MSG_CONFIRM*/0, reinterpret_cast<const sockaddr*>(&aServerAddress), sizeof(aServerAddress));
}

void processMessages(int aSocket, uint8_t* aBuffer, sockaddr_in& aServerAddress)
{
    unsigned totalPacketsCount = 0;
    std::set<PacketIdType> receivedPacketsIDs;
    std::vector<double> receivedDoubles;
    bool isProcessing = true;
    while (isProcessing)
    {
        socklen_t len = 0;
        ssize_t bytesAvailable = recvfrom(aSocket, aBuffer, MAX_BUFFER_SIZE, MSG_WAITALL, reinterpret_cast<sockaddr*>(&aServerAddress), &len);
        
        PacketBase<eServerMessageType> packetBase;
        ssize_t bytesParsed;
        std::tie(packetBase, bytesParsed) = PacketBase<eServerMessageType>::parseFromBuffer(aBuffer, bytesAvailable);

        auto readPtr = aBuffer + bytesParsed;
        bytesAvailable -= bytesParsed;

        switch (packetBase.getType())
        {
            case eServerMessageType::PROTOCOL_VERSION_ERROR:
            {
                std::cout << "Server has newer protocol version (" << packetBase.getProtocolVersion() << ").\n";
                sendPacketReceivedConfirmation(aSocket, aBuffer, aServerAddress, packetBase.getPacketID());
                isProcessing = false;
                break;
            }
            case eServerMessageType::PACKETS_COUNT:
            {
                auto insertResult = receivedPacketsIDs.insert(packetBase.getPacketID());
                if (insertResult.second)
                {
                    totalPacketsCount = BufferParser::parseValueFromBuffer<unsigned>(readPtr, bytesAvailable).first;
                }
                sendPacketReceivedConfirmation(aSocket, aBuffer, aServerAddress, packetBase.getPacketID());
                break;
            }
            case eServerMessageType::DOUBLES_PACKET:
            {
                auto insertResult = receivedPacketsIDs.insert(packetBase.getPacketID());
                if (insertResult.second)
                {
                    auto vector = BufferParser::parseVectorFromBuffer<double>(readPtr, bytesAvailable).first;
                    std::copy(vector.begin(), vector.end(), std::back_inserter(receivedDoubles));
                }
                sendPacketReceivedConfirmation(aSocket, aBuffer, aServerAddress, packetBase.getPacketID());
                break;
            }
            default:
                break;
        }

        if (receivedPacketsIDs.size() == totalPacketsCount && totalPacketsCount != 0)
        {
            std::sort(receivedDoubles.begin(), receivedDoubles.end(), std::greater<double>());

            std::ofstream file("data.bin", std::ios::binary);
            if (file.is_open())
            {
                file.write(reinterpret_cast<const char*>(receivedDoubles.data()), receivedDoubles.size() * sizeof(double));
                if (!file.good())
                {
                    std::cerr << "Error occurred while writing to file\n";
                }
                file.close();
            }
            
            isProcessing = false;
        }
    }
}

int main()
{
    int socketResult = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketResult < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    mClientConfig.parseConfig("client_config.txt");
    const auto configAddress = mClientConfig.getConfigValue("address");
    const auto port = mClientConfig.getConfigValue("port");

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(stoi(port.value_or("8080")));
    auto address = configAddress.value_or("0.0.0.0");
    inet_pton(AF_INET, address.c_str(), &servaddr.sin_addr);
    
    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto th = std::thread([socketResult, &servaddr]()
        {
            uint8_t buffer[MAX_BUFFER_SIZE];
            auto doublesRangeMax = mClientConfig.getConfigValue("doubles_range_max");
            auto sendRes = sendDoublesRequest(socketResult, buffer, servaddr, stod(doublesRangeMax.value_or("100")));
            if (sendRes > 0)
            {
                processMessages(socketResult, buffer, servaddr);
            }
        }
    );

    th.join();
   
    close(socketResult);
    return 0;
}
