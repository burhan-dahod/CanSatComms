#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include "comms.hpp"
#include "payload.hpp"

TEST(PayloadSerialization, SerializesOutTelemetryCorrectly) {
    // Create a payload with known data
    OutTelemetry telemetry;
    telemetry.seconds_since_epoch = 1724636159; // Hex: 0x66CBCEFF
    telemetry.mechanisms_deployed_flags = 3;    // Hex: 0x0003

    // Serialize function should return a vector of bytes
    std::vector<uint8_t> result = serialize_telemetry(telemetry);

    // Define the exact byte array we expect to get back
    // Message ID: 0x01
    // Epoch (Little-Endian): FF DB CB 66
    // Flags (Little-Endian): 03 00
    std::vector<uint8_t> expected = {
        0x01, 
        0xFF, 0xDB, 0xCB, 0x66, 
        0x03, 0x00
    };

    // Assert that the vectors are completely identical
    EXPECT_EQ(result, expected);
}


TEST(PayloadDeserialization, DeserializesCommandMechanismCorrectly) {
    // Create a raw byte vector simulating an incoming radio transmission
    // 0x02 = Message ID
    // 0x05 = Mechanism ID (5)
    // 0xB4 = Value (180 in decimal)
    std::vector<uint8_t> incoming_bytes = {0x02, 0x05, 0xB4};

    // Pass the bytes to your function
    CommandMechanism result = deserialize_mechanism(incoming_bytes);

    // Assert: Verify the struct holds the exact numbers we expect
    EXPECT_EQ(result.mechanism_id, 5);
    EXPECT_EQ(result.value, 180);
}

TEST(PayloadDeserialization, DeserializesCommandTelemetryCorrectly) {
    // Simulate an incoming packet from the Ground Station
    // 0x03 = Message ID for CommandTelemetry
    // 0x01 = is_on = true (or use 0x00 for false)
    std::vector<uint8_t> incoming_bytes = {0x03, 0x01};

    CommandTelemetry result = deserialize_telemetry(incoming_bytes);

    // Assert
    EXPECT_TRUE(result.is_on);
}

TEST(PayloadDeserialization, DeserializesOutTelemetryCorrectly) {
    // The exact byte array we expect from the radio
    std::vector<uint8_t> incoming_bytes = {
        0x01, 
        0xFF, 0xDB, 0xCB, 0x66, 
        0x03, 0x00
    };

    // Pass the bytes to function
    OutTelemetry result = deserialize_out_telemetry(incoming_bytes);

    // Assert: Verify the struct holds the exact original numbers
    EXPECT_EQ(result.seconds_since_epoch, 1724636159);
    EXPECT_EQ(result.mechanisms_deployed_flags, 3);
}

TEST(XBeeFraming, BuildsTransmitRequest0x10Correctly) {
    // A simple test payload (e.g., Message ID 0x01, 2 bytes of data)
    std::vector<uint8_t> payload = {0x01, 0xAA, 0xBB};

    // Build the 0x10 frame around it
    std::vector<uint8_t> frame = build_transmit_request(payload);

    // Assert: 
    // Check start delimiter
    EXPECT_EQ(frame[0], 0x7E);
    
    // Check length: 14 header bytes + 3 payload bytes = 17 (0x0011)
    EXPECT_EQ(frame[1], 0x00);
    EXPECT_EQ(frame[2], 0x11);
    
    // Check Frame Type is 0x10 at index 3
    EXPECT_EQ(frame[3], 0x10);
    
    // Verify the total frame size is Start(1) + Length(2) + Header/Payload(17) + Checksum(1) = 21 bytes
    EXPECT_EQ(frame.size(), 21);
}

#include <gtest/gtest.h>
#include "comms.hpp"
#include <stdexcept>

// HAPPY PATH TEST
TEST(XBeeParsing, ParsesValidReceivePacket0x90) {
    // Construct a valid 0x90 frame according to the offset specification
    // Offset 0:    Start Delimiter (0x7E)
    // Offset 1-2:  Length (0x00, 0x0F -> 15 bytes following length)
    // Offset 3:    Frame Type (0x90)
    // Offset 4-11: 64-bit Source Address (8 bytes of 0x00)
    // Offset 12-13: 16-bit Source Address (0xFF, 0xFE)
    // Offset 14:   Receive Options (0x00)
    // Offset 15+:  Payload data (0x02, 0x05, 0xB4)
    std::vector<uint8_t> raw_frame = {
        0x7E, 
        0x00, 0x0F, 
        0x90, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0xFF, 0xFE, 
        0x00, 
        0x02, 0x05, 0xB4  //payload data
    };

    // Automatically calculate and append the correct checksum (EOF)
    uint32_t checksum_sum = 0;
    for (size_t i = 3; i < raw_frame.size(); ++i) {
        checksum_sum += raw_frame[i];
    }
    uint8_t valid_checksum = 0xFF - (checksum_sum & 0xFF);
    raw_frame.push_back(valid_checksum);

    // Parse the frame
    std::vector<uint8_t> payload = parse_receive_packet(raw_frame);

    // Verify the inner payload was cleanly extracted
    ASSERT_EQ(payload.size(), 3);
    EXPECT_EQ(payload[0], 0x02);
    EXPECT_EQ(payload[1], 0x05);
    EXPECT_EQ(payload[2], 0xB4);
}

// ERROR HANDLING TESTS
TEST(XBeeParsing, RejectsTooShortFrames) {
    // Frame is less than 16 bytes minimum
    std::vector<uint8_t> short_frame = {0x7E, 0x00, 0x01, 0x90};
    EXPECT_THROW(parse_receive_packet(short_frame), std::runtime_error);
}

TEST(XBeeParsing, RejectsInvalidStartDelimiter) {
    // Starts with 0x7F instead of 0x7E
    std::vector<uint8_t> bad_start_frame = {
        0x7F, 0x00, 0x0F, 0x90, 
        0, 0, 0, 0, 0, 0, 0, 0, 
        0xFF, 0xFE, 0x00, 
        1, 2, 3, 0x00
    };
    EXPECT_THROW(parse_receive_packet(bad_start_frame), std::runtime_error);
}

TEST(XBeeParsing, RejectsMismatchedLength) {
    // Length header claims a size that doesn't match the actual vector size
    std::vector<uint8_t> bad_length_frame = {
        0x7E, 
        0xFF, 0xFF, // a massive length
        0x90, 
        0, 0, 0, 0, 0, 0, 0, 0, 
        0xFF, 0xFE, 0x00, 
        1, 2, 3, 0x00
    };
    EXPECT_THROW(parse_receive_packet(bad_length_frame), std::runtime_error);
}

TEST(XBeeParsing, RejectsInvalidFrameType) {
    // Frame type is 0x88 instead of 0x90
    std::vector<uint8_t> raw_frame = {
        0x7E, 
        0x00, 0x0F, 
        0x88, // Wrong Frame Type
        0, 0, 0, 0, 0, 0, 0, 0, 
        0xFF, 0xFE, 
        0x00, 
        1, 2, 3
    };
    
    // Calculate checksum so it fails on frame type, not checksum
    uint32_t sum = 0;
    for (size_t i = 3; i < raw_frame.size(); ++i) sum += raw_frame[i];
    raw_frame.push_back(0xFF - (sum & 0xFF));

    EXPECT_THROW(parse_receive_packet(raw_frame), std::runtime_error);
}

TEST(XBeeParsing, RejectsCorruptedChecksum) {
    // Valid frame structure, but we tamper with the final checksum byte
    std::vector<uint8_t> raw_frame = {
        0x7E, 
        0x00, 0x0F, 
        0x90, 
        0, 0, 0, 0, 0, 0, 0, 0, 
        0xFF, 0xFE, 
        0x00, 
        1, 2, 3, 
        0xEE // <-- Incorrect checksum on purpose
    };
    EXPECT_THROW(parse_receive_packet(raw_frame), std::runtime_error);
}