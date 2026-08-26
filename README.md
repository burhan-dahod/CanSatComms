# CanSatComms Library

A modular, lightweight C++ communications library for CanSat flight software, separating XBee API framing ('0x10' / '0x90') from application-level telemetry and command serialization.

---

## Key Features
* **XBee Framing:** Constructs Transmit Request ('0x10') frames and parses/validates Receive Packet ('0x90') frames with checksum verification.
* **Payload Codecs:** Serializes 'OutTelemetry' structs and deserializes 'CommandMechanism' or 'CommandTelemetry' payloads.
* 
---

##  Payload Specification
* **'OutTelemetry' (6 bytes):** 'seconds_since_epoch' ('uint32_t') + 'mechanisms_deployed_flags' ('uint16_t')
* **'CommandMechanism' (2 bytes):** 'mechanism_id' ('uint8_t') + 'value' ('uint8_t')
* **'CommandTelemetry' (1 byte):** 'is_on' ('bool'/'uint8_t')

---

## 🛠️ Building and Testing

Requires a C++ compiler and **CMake 3.14+** (Google Test is automatically fetched via 'FetchContent').

```bash
# Configure and build
cmake -B build -S .
cmake --build build

# Run tests via CTest
cd build
ctest --output-on-failure
