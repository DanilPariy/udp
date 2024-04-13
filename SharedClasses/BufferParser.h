#pragma once

#include <bits/stdc++.h>
#include <Types.h>

class BufferParser
{
public:
    template<typename ParseDataType>
    static ParseDataType parseBuffer(const uint8_t* aBuffer, unsigned aBytesAvailable);

    template<>
    sDoublesPacket parseBuffer(const uint8_t* aBuffer, unsigned aBytesAvailable)
    {
        sDoublesPacket packet;

        const uint8_t * readPtr = aBuffer;
        const uint8_t * firstInvalidByte = aBuffer + aBytesAvailable;  // pointer to the first invalid byte

        if (firstInvalidByte - readPtr >= sizeof(packet.packetIndex))
        {
            uint32_t neDist;
            memcpy(&neDist, readPtr, sizeof(neDist));
            readPtr += sizeof(neDist);
            packet.packetIndex = ntohl(neDist);  // convert from big-endian back to local-endian

            while (firstInvalidByte - readPtr >= sizeof(double))
            {
                double d;
                memcpy(&d, readPtr, sizeof(d));
                readPtr += sizeof(double);
                packet.doubles.push_back(d);
            }
        }

        return std::move(packet);
    };
};