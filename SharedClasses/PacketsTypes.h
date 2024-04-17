#pragma once
#include <utility>

using Buffer = std::pair<uint8_t*, unsigned>;
using CastPacketType = uint8_t;
using PacketIdType = unsigned;

template <typename MessageType>
class PacketBase
{
private:
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

    virtual Buffer writeToBuffer(Buffer aBuffer) const
    {
        uint8_t* writePtr = aBuffer.first;
        unsigned bytesAvailable = aBuffer.second;

        if (bytesAvailable >= sizeof(PacketIdType))
        {
            memcpy(writePtr, &mPacketID, sizeof(PacketIdType));
            writePtr += sizeof(PacketIdType);
            bytesAvailable -= sizeof(PacketIdType);

            auto cast = static_cast<CastPacketType>(mType);
            if (bytesAvailable >= sizeof(CastPacketType))
            {
                memcpy(writePtr, &cast, sizeof(CastPacketType));
                writePtr += sizeof(CastPacketType);
                bytesAvailable -= sizeof(CastPacketType);
            }
        }

        return std::make_pair(writePtr, bytesAvailable);
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

    virtual Buffer writeToBuffer(Buffer aBuffer) const override
    {
        auto baseWriteRes = PacketBase<MessageType>::writeToBuffer(aBuffer);

        uint8_t* writePtr = baseWriteRes.first;
        unsigned bytesAvailable = baseWriteRes.second;

        if (bytesAvailable >= sizeof(ValueType))
        {
            memcpy(writePtr, &mValue, sizeof(ValueType));
            writePtr += sizeof(ValueType);
            bytesAvailable -= sizeof(ValueType);
        }

        return std::make_pair(writePtr, bytesAvailable);
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

    virtual Buffer writeToBuffer(Buffer aBuffer) const override
    {
        auto baseWriteRes = PacketBase<MessageType>::writeToBuffer(aBuffer);

        uint8_t* writePtr = baseWriteRes.first;
        unsigned bytesAvailable = baseWriteRes.second;

        if (bytesAvailable >= sizeof(ValueType))
        {
            auto bytesCanWrite = std::min(bytesAvailable, static_cast<unsigned>(mValue.size() * sizeof(ValueType)));
            std::memcpy(writePtr, mValue.data(), bytesCanWrite);
            writePtr += bytesCanWrite;
            bytesAvailable -= bytesCanWrite;
        }

        return std::make_pair(writePtr, bytesAvailable);
    }

    void setValue(std::vector<ValueType>&& aValue)
    {
        mValue = aValue;
    }
};