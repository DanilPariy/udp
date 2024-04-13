// Server side implementation of UDP client-server model
#include <bits/stdc++.h>
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

#define PORT 8080

// Driver code
int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    
    // Bind the socket with the server address
    if (bind(sockfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    socklen_t len = 0;
    len = sizeof(cliaddr);
    uint8_t buffer[MAX_BUFFER_SIZE];
    ConfigManager::getInstance()->parseConfig("server_config.txt");
    auto value = ConfigManager::getInstance()->getConfigValue("doubles_count_to_send");
    while (1)
    {
        auto n = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, MSG_WAITALL, reinterpret_cast<sockaddr*>(&cliaddr), &len);
        
        sDoublesPacket packet;
        packet.packetIndex = 101;
        packet.doubles.reserve(MAX_DOUBLES_COUNT);
        for (int i = 0; i < MAX_DOUBLES_COUNT; i++)
        {
            auto generatedDouble = RandomHelper::random<double>(-100, 100);
            packet.doubles.push_back(generatedDouble);
        }

        uint8_t * b = buffer;

        uint32_t neDist = htonl(packet.packetIndex);
        memcpy(b, &neDist, sizeof(neDist));
        b += sizeof(neDist);
        
        for (const auto& d: packet.doubles)
        {
            memcpy(b, &d, sizeof(d));
            b += sizeof(d);
        }
        auto toSend = b - buffer;
        ssize_t sentBytes = sendto(sockfd, buffer, toSend, /*MSG_CONFIRM*/0, reinterpret_cast<sockaddr*>(&cliaddr), len);
        if (!sentBytes)
        {
            break;
        }
    }
    
    return 0;
}
