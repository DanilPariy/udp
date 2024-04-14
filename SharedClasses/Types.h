#pragma once

#include <vector>

struct sDoublesPacket
{
    unsigned packetIndex;
    std::vector<double> doubles;

    sDoublesPacket()
        : packetIndex(0)
    {
    }
};

enum class ePacketType
{
    UNDEFINED = 0
    , CLIENT_AKS_DOUBLES
    , SERVER_SENDS_DOUBLES_PACKET
    , CLIENT_SENDS_DOUBLES_PACKET_CONFIRMATION
};