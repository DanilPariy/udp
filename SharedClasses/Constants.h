#pragma once

#include "Types.h"

constexpr int MAX_UDP_DATA_BYTES = 65507;

// should have been (MAX_UDP_DATA_BYTES - sizeof(sDoublesPacket::packetIndex)) / sizeof(double); // 1151
constexpr int MAX_DOUBLES_COUNT = 1024;

constexpr int MAX_BUFFER_SIZE = MAX_DOUBLES_COUNT * sizeof(double) + sizeof(sDoublesPacket::packetIndex);