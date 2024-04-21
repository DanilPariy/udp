#pragma once

enum class eServerMessageType
{
    UNDEFINED = 0
    , PROTOCOL_VERSION_ERROR
    , PACKETS_COUNT
    , DOUBLES_PACKET
};

enum class eClientMessageType
{
    UNDEFINED = 0
    , DOUBLES_RANGE_MAX
    , PACKET_RECEIVED_CONFIRMATION
};