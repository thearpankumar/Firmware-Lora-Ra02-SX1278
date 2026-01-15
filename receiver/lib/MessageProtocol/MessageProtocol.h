#ifndef MESSAGE_PROTOCOL_H
#define MESSAGE_PROTOCOL_H

#include <Arduino.h>

// Protocol Constants
#define MSG_START_BYTE 0xAA
#define MSG_MAX_PAYLOAD 250
#define MSG_HEADER_SIZE 5  // START + MSG_ID(2) + TYPE + LENGTH
#define MSG_CHECKSUM_SIZE 1
#define MSG_MAX_PACKET_SIZE (MSG_HEADER_SIZE + MSG_MAX_PAYLOAD + MSG_CHECKSUM_SIZE)

// Message Types
enum MessageType {
    MSG_TEXT = 0x01,           // Text message
    MSG_SENSOR_REQUEST = 0x02, // Request sensor data
    MSG_SENSOR_RESPONSE = 0x03,// Sensor data response
    MSG_COMMAND = 0x04,        // Control command
    MSG_ACK = 0x05,            // Acknowledgment
    MSG_NACK = 0x06            // Negative acknowledgment
};

// Sensor IDs
enum SensorId {
    SENSOR_TEMPERATURE = 0x01,
    SENSOR_HUMIDITY = 0x02,
    SENSOR_BATTERY = 0x03,
    SENSOR_PRESSURE = 0x04
};

// Command IDs
enum CommandId {
    CMD_LED_ON = 0x01,
    CMD_LED_OFF = 0x02,
    CMD_LED_TOGGLE = 0x03
};

// ACK Status Codes
enum AckStatus {
    ACK_OK = 0x00,
    ACK_ERROR = 0x01,
    ACK_INVALID = 0x02,
    ACK_CHECKSUM_FAIL = 0x03
};

// Message Structure
struct Message {
    uint16_t messageId;
    MessageType type;
    uint8_t payload[MSG_MAX_PAYLOAD];
    uint8_t payloadLength;
    int rssi;
    float snr;
};

// Sensor Response Data
struct SensorData {
    uint8_t sensorId;
    float value;
    char unit[16];
    char deviceName[32];  // Device identifier (e.g., "trident1", "trident2")
};

class MessageProtocol {
public:
    MessageProtocol();

    // ===== Encoding Methods =====

    // Encode text message
    size_t encodeText(const char* text, uint8_t* buffer);

    // Encode sensor request
    size_t encodeSensorRequest(uint8_t sensorId, uint8_t* buffer);

    // Encode sensor response (legacy - no device name)
    size_t encodeSensorResponse(uint8_t sensorId, float value, const char* unit, uint8_t* buffer);

    // Encode sensor response with device name
    size_t encodeSensorResponseWithDevice(const char* deviceName, uint8_t sensorId, float value, const char* unit, uint8_t* buffer);

    // Encode command
    size_t encodeCommand(uint8_t cmdId, const uint8_t* params, size_t paramLen, uint8_t* buffer);

    // Encode ACK/NACK
    size_t encodeAck(uint16_t msgId, uint8_t status, uint8_t* buffer);

    // ===== Decoding Methods =====

    // Decode received packet into Message structure
    bool decode(const uint8_t* buffer, size_t length, Message& msg);

    // ===== Utility Methods =====

    // Generate unique message ID
    uint16_t generateMessageId();

    // Calculate XOR checksum
    uint8_t calculateChecksum(const uint8_t* data, size_t length);

    // Verify checksum
    bool verifyChecksum(const uint8_t* data, size_t length);

    // Get message type name (for debugging)
    const char* getMessageTypeName(MessageType type);

    // Get sensor name
    const char* getSensorName(uint8_t sensorId);

    // Get command name
    const char* getCommandName(uint8_t cmdId);

    // Parse sensor response payload (legacy - no device name)
    bool parseSensorResponse(const uint8_t* payload, uint8_t payloadLength, SensorData& data);

    // Parse sensor response with device name
    bool parseSensorResponseWithDevice(const uint8_t* payload, uint8_t payloadLength, SensorData& data);

private:
    uint16_t lastMessageId;

    // Internal encoding helper
    size_t encodePacket(MessageType type, const uint8_t* payload, size_t payloadLength, uint8_t* buffer);
};

#endif // MESSAGE_PROTOCOL_H
