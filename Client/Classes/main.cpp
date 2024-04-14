// Client side implementation of UDP client-server model
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>

#include "Constants.h"
#include "BufferParser.h"
#include "Types.h"
#include "ConfigManager.h"

void sendData()
{

}

ssize_t sendDoublesRequest(int aSocket, uint8_t* aBuffer, const sockaddr_in& aServerAddress, double aDoublesRangeMax)
{
    uint8_t* b = aBuffer;

    auto cast = static_cast<uint8_t>(ePacketType::CLIENT_AKS_DOUBLES);
    memcpy(b, &cast, sizeof(cast));
    b += sizeof(cast);

    memcpy(b, &aDoublesRangeMax, sizeof(aDoublesRangeMax));
    b += sizeof(aDoublesRangeMax);

    return sendto(aSocket, aBuffer, b - aBuffer, /*MSG_CONFIRM*/0, reinterpret_cast<const sockaddr*>(&aServerAddress), sizeof(aServerAddress));
}

int main()
{
    int socketResult;
    if ( (socketResult = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
   
    ConfigManager::getInstance()->parseConfig("client_config.txt");
    auto port = ConfigManager::getInstance()->getConfigValue("port");

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(stoi(port.value_or("8080")));
    servaddr.sin_addr.s_addr = INADDR_ANY;
       
       
    uint8_t buffer[MAX_BUFFER_SIZE];
    auto doublesRangeMax = ConfigManager::getInstance()->getConfigValue("doubles_range_max");
    
    auto sendRes = sendDoublesRequest(socketResult, buffer, servaddr, stod(doublesRangeMax.value_or("100")));
    if (sendRes > 0)
    {
        while (true)
        {
            socklen_t len = 0;
            ssize_t numBytesReceived = recvfrom(socketResult, buffer, MAX_BUFFER_SIZE, MSG_WAITALL, reinterpret_cast<sockaddr*>(&servaddr), &len);
            
            auto packet = BufferParser::parseBuffer<sDoublesPacket>(buffer, numBytesReceived);

            std::cout << "packet index = " << packet.first.packetIndex << std::endl;
        }
    }
    else if (sendRes == EAFNOSUPPORT)
    {
        // process error
    }
   
    close(socketResult);
    return 0;
}
