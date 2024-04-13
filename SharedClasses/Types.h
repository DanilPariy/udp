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