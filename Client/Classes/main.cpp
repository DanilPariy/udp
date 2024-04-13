// Client side implementation of UDP client-server model
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
#include "BufferParser.h"
   
#define PORT 8080
   
// Driver code
int main() {
    int sockfd;
    uint8_t buffer[MAX_BUFFER_SIZE];
    const std::string message = "Hello from the client";
    struct sockaddr_in     servaddr;
   
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
   
    memset(&servaddr, 0, sizeof(servaddr));
       
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
       
       
    auto sendRes = sendto(sockfd, message.c_str(), message.size(), /*MSG_CONFIRM*/0, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr));
    if (sendRes > 0)
    {
        std::cout<<"Hello message sent."<<std::endl;
        
        socklen_t len = 0;
        int32_t numBytesReceived = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, MSG_WAITALL, reinterpret_cast<sockaddr*>(&servaddr), &len);
        
        std::cout << "numBytesReceived" << numBytesReceived << std::endl;
        auto packet = BufferParser::parseBuffer<sDoublesPacket>(buffer, numBytesReceived);

        std::cout << "packet index = " << packet.packetIndex<<std::endl;
        for (const auto& d: packet.doubles)
        {
            std::cout << d << ", ";
        }
        std::cout << std::endl;
    }
    else if (sendRes == EAFNOSUPPORT)
    {
        // process error
    }
   
    close(sockfd);
    return 0;
}
