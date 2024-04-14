#pragma once

#include <bits/stdc++.h>
#include <Types.h>

class BufferParser
{
public:
    template<typename ParseDataType>
    static std::pair<ParseDataType, ssize_t> parseBuffer(uint8_t* aBuffer, ssize_t aBytesAvailable)
    {
        ParseDataType result;

        const uint8_t * readPtr = aBuffer;
        const uint8_t * firstInvalidByte = aBuffer + aBytesAvailable;

        if (firstInvalidByte - readPtr >= sizeof(result))
        {
            memcpy(&result, readPtr, sizeof(result));
            readPtr += sizeof(result);
            aBytesAvailable -= sizeof(result);
        }

        return std::make_pair(result, readPtr - aBuffer);
    }

    template<>
    std::pair<sDoublesPacket, ssize_t> parseBuffer(uint8_t* aBuffer, ssize_t aBytesAvailable)
    {
        sDoublesPacket packet;

        const uint8_t * readPtr = aBuffer;
        const uint8_t * firstInvalidByte = aBuffer + aBytesAvailable;

        if (firstInvalidByte - readPtr >= sizeof(packet.packetIndex))
        {
            uint32_t neDist;
            memcpy(&neDist, readPtr, sizeof(neDist));
            readPtr += sizeof(neDist);
            packet.packetIndex = neDist;

            while (firstInvalidByte - readPtr >= sizeof(double))
            {
                double d;
                memcpy(&d, readPtr, sizeof(d));
                readPtr += sizeof(double);
                packet.doubles.push_back(d);
            }
        }

        return std::make_pair(packet, readPtr - aBuffer);
    };
};