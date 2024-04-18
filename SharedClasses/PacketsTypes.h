#pragma once
#include <utility>

using CastPacketType = uint8_t;
using PacketIdType = unsigned;
using ProtocolVersionType = unsigned;

template <typename MessageType>
class PacketBase
{
private:
    ProtocolVersionType mProtocolVersion;
    PacketIdType mPacketID;
    MessageType mType;
    bool mIsReceived;

public:
    PacketBase()
        : mType(static_cast<MessageType>(0))
        , mPacketID(0)
        , mIsReceived(false)
    {
    }
    virtual ~PacketBase()
    {
    }

    virtual unsigned writeToBuffer(uint8_t* aBuffer, unsigned aBytesAvailable) const
    {
        uint8_t* writePtr = aBuffer;
        const uint8_t* const firstInvalidByte = aBuffer + aBytesAvailable;

        if (firstInvalidByte - writePtr >= sizeof(PacketIdType))
        {
            memcpy(writePtr, &mPacketID, sizeof(PacketIdType));
            writePtr += sizeof(PacketIdType);

            auto cast = static_cast<CastPacketType>(mType);
            if (firstInvalidByte - writePtr >= sizeof(CastPacketType))
            {
                memcpy(writePtr, &cast, sizeof(CastPacketType));
                writePtr += sizeof(CastPacketType);
            }
        }

        return writePtr - aBuffer;
    }

    void setType(MessageType aType)
    {
        mType = aType;
    }

    void setPacketID(unsigned aPacketID)
    {
        mPacketID = aPacketID;
    }

    void setIsReceived(bool aIsReceived)
    {
        mIsReceived = aIsReceived;
    }
    bool isReceived() const
    {
        return mIsReceived;
    }
};

template <typename MessageType, typename ValueType>
class SingleValuePacket : public PacketBase<MessageType>
{
private:
    ValueType mValue;

public:
    SingleValuePacket()
    {
    }
    virtual ~SingleValuePacket()
    {
    }

    virtual unsigned writeToBuffer(uint8_t* aBuffer, unsigned aBytesAvailable) const override
    {
        auto bytesWrote = PacketBase<MessageType>::writeToBuffer(aBuffer, aBytesAvailable);

        uint8_t* writePtr = aBuffer + bytesWrote;
        const uint8_t* const firstInvalidByte = aBuffer + aBytesAvailable;

        if (firstInvalidByte - writePtr >= sizeof(ValueType))
        {
            memcpy(writePtr, &mValue, sizeof(ValueType));
            writePtr += sizeof(ValueType);
        }

        return writePtr - aBuffer;
    }

    void setValue(ValueType aValue)
    {
        mValue = aValue;
    }
};

template <typename MessageType, typename ValueType>
class VectorPacket : public PacketBase<MessageType>
{
private:
    std::vector<ValueType> mValue;

public:
    static constexpr unsigned MAX_VECTOR_SIZE = (MAX_BUFFER_SIZE - sizeof(CastPacketType) - sizeof(PacketIdType)) / sizeof(ValueType);

    VectorPacket()
    {
    }
    virtual ~VectorPacket()
    {
    }

    virtual unsigned writeToBuffer(uint8_t* aBuffer, unsigned aBytesAvailable) const override
    {
        auto bytesWrote = PacketBase<MessageType>::writeToBuffer(aBuffer, aBytesAvailable);

        uint8_t* writePtr = aBuffer + bytesWrote;
        const uint8_t* const firstInvalidByte = aBuffer + aBytesAvailable;

        if (firstInvalidByte - writePtr >= sizeof(ValueType))
        {
            auto bytesAvailable = firstInvalidByte - writePtr;
            auto bytesNeeded = static_cast<long>(mValue.size() * sizeof(ValueType));
            auto bytesCanWrite = std::min(bytesAvailable, bytesNeeded);
            std::memcpy(writePtr, mValue.data(), bytesCanWrite);
            writePtr += bytesCanWrite;
        }

        return writePtr - aBuffer;
    }

    void setValue(std::vector<ValueType>&& aValue)
    {
        mValue = aValue;
    }
};