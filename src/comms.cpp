#include "comms.hpp"
#include <iostream>

std::vector<uint8_t> serialize_telemetry(const OutTelemetry& data) {
    std::vector<uint8_t> payload;
    
    // Add the Message ID (0x01) so the receiver knows what this is
    payload.push_back(0x01);
    
    // Slice the 32-bit epoch time into 4 separate bytes - little endian order (least signifcant byte first)
    payload.push_back(data.seconds_since_epoch & 0xFF);           // Byte 1
    payload.push_back((data.seconds_since_epoch >> 8) & 0xFF);    // Byte 2
    payload.push_back((data.seconds_since_epoch >> 16) & 0xFF);   // Byte 3
    payload.push_back((data.seconds_since_epoch >> 24) & 0xFF);   // Byte 4
    
    // Slice the 16-bit flags into 2 separate bytes - little endian order
    payload.push_back(data.mechanisms_deployed_flags & 0xFF);         // Byte 1
    payload.push_back((data.mechanisms_deployed_flags >> 8) & 0xFF);  // Byte 2
    
    return payload;
}

CommandMechanism deserialize_mechanism(const std::vector<uint8_t>& bytes) {
    CommandMechanism cmd;
    
    // bytes[0] is the Message ID (0x02), so we skip it.
    
    // Grab the first data byte
    cmd.mechanism_id = bytes[1]; 
    
    // Grab the second data byte
    cmd.value = bytes[2];        
    
    return cmd;
}

CommandTelemetry deserialize_telemetry(const std::vector<uint8_t>& bytes) {
    CommandTelemetry cmd;
    
    // bytes[0] is the Message ID (0x03) - skip it!
    
    // Convert the incoming byte to a boolean (non-zero is true, 0 is false)
    cmd.is_on = (bytes[1] != 0);
    
    return cmd;
}

std::vector<uint8_t> build_transmit_request(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> frame;
    
    // Start delimiter (always 7E based on the documentation)
    frame.push_back(0x7E);
    
    // Calculate length
    // Header fields after length bytes: Frame Type(1) + Frame ID(1) + 64bit Addr(8) + 16bit Addr(2) + Radius(1) + Options(1) = 14 bytes
    uint16_t length = 14 + payload.size();
    frame.push_back((length >> 8) & 0xFF); // Length MSB
    frame.push_back(length & 0xFF);        // Length LSB
    
    // Variable to track sum for the checksum calculation
    uint32_t checksum_sum = 0;
    
    // Helper lambda to append a byte and add it to the checksum tally
    auto append_byte = [&](uint8_t b) {
        frame.push_back(b);
        checksum_sum += b;
    };
    
    // Frame Type (0x10)
    append_byte(0x10);
    
    // Frame ID (0x01)
    append_byte(0x01);
    
    // 64-bit Destination Address (Broadcast: 0x000000000000FFFF)
    for (int i = 0; i < 6; ++i) append_byte(0x00);
    append_byte(0xFF);
    append_byte(0xFF);
    
    // 16-bit Reserved (0xFFFE)
    append_byte(0xFF);
    append_byte(0xFE);
    
    // Broadcast Radius (0x00 = max hops)
    append_byte(0x00);
    
    // Options (0x00)
    append_byte(0x00);
    
    // Append the actual serialized payload bytes
    for (uint8_t b : payload) {
        append_byte(b);
    }
    
    // =Compute Checksum: 0xFF - (lower 8 bits of the sum)
    uint8_t checksum = 0xFF - (checksum_sum & 0xFF);
    frame.push_back(checksum);
    
    return frame;
}

#include <stdexcept>

#include <stdexcept>

std::vector<uint8_t> parse_receive_packet(const std::vector<uint8_t>& frame) {
    // Must be at least 16 bytes to reach offset 15 plus the checksum byte
    if (frame.size() < 16) {
        throw std::runtime_error("Error: Frame is too short to be a valid 0x90 receive packet.");
    }

    // Validate Start Delimiter (Offset 0)
    if (frame[0] != 0x7E) {
        throw std::runtime_error("Error: Invalid start delimiter. Expected 0x7E.");
    }

    // Extract and Validate Length (Offsets 1-2, Big-Endian)
    uint16_t length = ((uint16_t)frame[1] << 8) | frame[2];
    
    // length + 4 accounts for the start delimiter, length bytes, and checksum byte
    if (frame.size() != static_cast<size_t>(length + 4)) {
        throw std::runtime_error("Error: Frame size does not match the packet length header.");
    }

    // Validate Frame Type (Offset 3)
    if (frame[3] != 0x90) {
        throw std::runtime_error("Error: Invalid frame type. Expected 0x90 for Receive Packet.");
    }

    // Validate Checksum (0xFF minus the 8-bit sum of bytes from offset 3 to EOF-1)
    uint32_t checksum_sum = 0;
    for (size_t i = 3; i < frame.size() - 1; ++i) {
        checksum_sum += frame[i];
    }
    uint8_t calculated_checksum = 0xFF - (checksum_sum & 0xFF);
    uint8_t received_checksum = frame.back();

    if (calculated_checksum != received_checksum) {
        throw std::runtime_error("Error: Checksum mismatch! Data is corrupted.");
    }

    // Extract the Received Data (Payload starting at Offset 15 up to EOF)
    size_t payload_start = 15;
    size_t payload_end = frame.size() - 1; // Exclusive upper bound for vector range

    if (payload_start > payload_end) {
        return {}; // Empty payload
    }

    return std::vector<uint8_t>(frame.begin() + payload_start, frame.begin() + payload_end);
}

OutTelemetry deserialize_out_telemetry(const std::vector<uint8_t>& bytes) {
    OutTelemetry data;
    
    // bytes[0] is the Message ID (0x01) - skip it!
    
    // Stitch 4 bytes back into the 32-bit timestamp
    data.seconds_since_epoch = 
        ((uint32_t)bytes[1]) |               // Byte 1 stays at the bottom
        ((uint32_t)bytes[2] << 8) |          // Byte 2 shifts left by 8 spaces
        ((uint32_t)bytes[3] << 16) |         // Byte 3 shifts left by 16 spaces
        ((uint32_t)bytes[4] << 24);          // Byte 4 shifts left by 24 spaces
        
    // Stitch 2 bytes back into the 16-bit flags
    data.mechanisms_deployed_flags = 
        ((uint16_t)bytes[5]) |               // Byte 1 stays at the bottom
        ((uint16_t)bytes[6] << 8);           // Byte 2 shifts left by 8 spaces
        
    return data;
}