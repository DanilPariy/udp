#pragma once

#include <Types.h>

class BufferParser
{
public:
    template<typename ParseDataType>
    static std::pair<ParseDataType, ssize_t> parseValueFromBuffer(uint8_t* aBuffer, ssize_t aBytesAvailable)
    {
        ParseDataType result;

        const uint8_t * readPtr = aBuffer;

        if (aBytesAvailable >= sizeof(ParseDataType))
        {
            memcpy(&result, readPtr, sizeof(ParseDataType));
            readPtr += sizeof(ParseDataType);
            aBytesAvailable -= sizeof(ParseDataType);
        }

        return std::make_pair(result, readPtr - aBuffer);
    }

    template<typename VectorValueType>
    static std::pair<std::vector<VectorValueType>, ssize_t> parseVectorFromBuffer(uint8_t* aBuffer, ssize_t aBytesAvailable)
    {
        std::vector<VectorValueType> result;

        const uint8_t * readPtr = aBuffer;

        if (aBytesAvailable >= sizeof(VectorValueType))
        {
            auto elemsCanRead = aBytesAvailable / sizeof(VectorValueType);
            auto bytesCanRead = elemsCanRead * sizeof(VectorValueType);
            result.resize(elemsCanRead);
            std::memcpy(result.data(), readPtr, bytesCanRead);
            readPtr += bytesCanRead;
            aBytesAvailable -= bytesCanRead;
        }

        return std::make_pair(result, readPtr - aBuffer);
    };
};