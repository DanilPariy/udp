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
#include "ConfigManager.h"
#include "PacketsTypes.h"

ssize_t sendDoublesRequest(int aSocket, uint8_t* aBuffer, const sockaddr_in& aServerAddress, double aDoublesRangeMax)
{
    SingleValuePacket<eClientMessageType, double> packet;
    packet.setType(eClientMessageType::DOUBLES_RANGE_MAX);
    packet.setValue(aDoublesRangeMax);

    uint8_t* b = packet.writeToBuffer(std::make_pair(aBuffer, MAX_BUFFER_SIZE)).first;
    return sendto(aSocket, aBuffer, b - aBuffer, /*MSG_CONFIRM*/0, reinterpret_cast<const sockaddr*>(&aServerAddress), sizeof(aServerAddress));
}

ssize_t sendPacketReceivedConfirmation(int aSocket, uint8_t* aBuffer, const sockaddr_in& aServerAddress, unsigned aPacketID)
{
    PacketBase<eClientMessageType> packet;
    packet.setType(eClientMessageType::PACKET_RECEIVED_CONFIRMATION);
    packet.setPacketID(aPacketID);

    uint8_t* b = packet.writeToBuffer(std::make_pair(aBuffer, MAX_BUFFER_SIZE)).first;
    return sendto(aSocket, aBuffer, b - aBuffer, /*MSG_CONFIRM*/0, reinterpret_cast<const sockaddr*>(&aServerAddress), sizeof(aServerAddress));
}

int main()
{
    int socketResult = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketResult < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
   
    ConfigManager::getInstance()->parseConfig("client_config.txt");
    const auto configAddress = ConfigManager::getInstance()->getConfigValue("address");
    const auto port = ConfigManager::getInstance()->getConfigValue("port");

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(stoi(port.value_or("8080")));
    auto address = configAddress.value_or("0.0.0.0");
    inet_pton(AF_INET, address.c_str(), &servaddr.sin_addr);

    uint8_t buffer[MAX_BUFFER_SIZE];
    auto doublesRangeMax = ConfigManager::getInstance()->getConfigValue("doubles_range_max");
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    auto sendRes = sendDoublesRequest(socketResult, buffer, servaddr, stod(doublesRangeMax.value_or("100")));
    if (sendRes > 0)
    {
        auto th = std::thread([socketResult, &servaddr, &buffer]()
            {
                unsigned packetsCount = 0;
                std::set<PacketIdType> receivedPacketsIDs;
                std::vector<double> receivedDoubles;
                while (true)
                {
                    socklen_t len = 0;
                    ssize_t numBytesReceived = recvfrom(socketResult, buffer, MAX_BUFFER_SIZE, MSG_WAITALL, reinterpret_cast<sockaddr*>(&servaddr), &len);
                    uint8_t* readPtr = buffer;
                    auto parseIdResult = BufferParser::parseValueFromBuffer<PacketIdType>(readPtr, numBytesReceived);
                    auto bytesRead = parseIdResult.second;
                    readPtr += bytesRead;
                    numBytesReceived -= bytesRead;

                    auto parseMessageTypeResult = BufferParser::parseValueFromBuffer<CastPacketType>(readPtr, numBytesReceived);
                    bytesRead = parseMessageTypeResult.second;
                    readPtr += bytesRead;
                    numBytesReceived -= bytesRead;

                    auto casted = static_cast<eServerMessageType>(parseMessageTypeResult.first);

                    if (casted == eServerMessageType::PACKETS_COUNT)
                    {
                        receivedPacketsIDs.insert(parseIdResult.first);
                        packetsCount = BufferParser::parseValueFromBuffer<unsigned>(readPtr, numBytesReceived).first;
                        sendPacketReceivedConfirmation(socketResult, buffer, servaddr, parseIdResult.first);
                    }
                    else if (casted == eServerMessageType::DOUBLES_PACKET)
                    {
                        auto insertResult = receivedPacketsIDs.insert(parseIdResult.first);
                        if (insertResult.second)
                        {
                            auto vector = BufferParser::parseVectorFromBuffer<double>(readPtr, numBytesReceived).first;
                            std::copy(vector.begin(), vector.end(), std::back_inserter(receivedDoubles));
                        }
                        sendPacketReceivedConfirmation(socketResult, buffer, servaddr, parseIdResult.first);
                    }

                    if (receivedPacketsIDs.size() == packetsCount && packetsCount != 0)
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

                        break;
                    }
                }
            }
        );

        th.join();
    }
   
    close(socketResult);
    return 0;
}
