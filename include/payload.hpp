#ifndef PAYLOAD_HPP
#define PAYLOAD_HPP

#include <cstdint>

// Prevent the compiler from adding empty "padding" bytes between variables
#pragma pack(push, 1)

// Transmit Message ID: 0x01
struct OutTelemetry {
    uint32_t seconds_since_epoch;
    uint16_t mechanisms_deployed_flags;
};

// Receive Message ID: 0x02
struct CommandMechanism {
    uint8_t mechanism_id;
    uint8_t value;
};

// Receive Message ID: 0x03
struct CommandTelemetry {
    bool is_on;
};

// Restore default compiler padding behavior
#pragma pack(pop)

#endif // PAYLOAD_HPP