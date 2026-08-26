#ifndef COMMS_HPP
#define COMMS_HPP

#include <vector>
#include <cstdint>
#include "payload.hpp"

//----------------------

// Transmit CanSat -> Ground Station
std::vector<uint8_t> serialize_telemetry(const OutTelemetry& data);
std::vector<uint8_t> build_transmit_request(const std::vector<uint8_t>& payload);

//----------------------

// Receive Ground Station -> CanSat
CommandMechanism deserialize_mechanism(const std::vector<uint8_t>& bytes);
CommandTelemetry deserialize_telemetry(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> parse_receive_packet(const std::vector<uint8_t>& frame);
//----------------------

// Deserialize transmission from the CanSat to the ground station
OutTelemetry deserialize_out_telemetry(const std::vector<uint8_t>& bytes);

#endif // COMMS_HPP