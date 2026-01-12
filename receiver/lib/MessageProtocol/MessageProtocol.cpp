#include "MessageProtocol.h"

MessageProtocol::MessageProtocol() : lastMessageId(0) {
    // Seed random number generator with microsecond timestamp
    randomSeed(micros());
}

// ===== Message ID Generation =====

uint16_t MessageProtocol::generateMessageId() {
    lastMessageId++;
    if (lastMessageId == 0) {
        lastMessageId = 1;  // Skip 0
    }
    return lastMessageId;
}

// ===== Checksum Calculation =====

uint8_t MessageProtocol::calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

bool MessageProtocol::verifyChecksum(const uint8_t* data, size_t length) {
    if (length < MSG_HEADER_SIZE + MSG_CHECKSUM_SIZE) {
        return false;
    }

    uint8_t receivedChecksum = data[length - 1];
    uint8_t calculatedChecksum = calculateChecksum(data, length - 1);

    return (receivedChecksum == calculatedChecksum);
}

// ===== Internal Encoding Helper =====

size_t MessageProtocol::encodePacket(MessageType type, const uint8_t* payload, size_t payloadLength, uint8_t* buffer) {
    if (payloadLength > MSG_MAX_PAYLOAD) {
        return 0;  // Payload too large
    }

    uint16_t msgId = generateMessageId();
    size_t index = 0;

    // START byte
    buffer[index++] = MSG_START_BYTE;

    // Message ID (2 bytes, big-endian)
    buffer[index++] = (msgId >> 8) & 0xFF;
    buffer[index++] = msgId & 0xFF;

    // Type
    buffer[index++] = (uint8_t)type;

    // Length
    buffer[index++] = (uint8_t)payloadLength;

    // Payload
    for (size_t i = 0; i < payloadLength; i++) {
        buffer[index++] = payload[i];
    }

    // Checksum
    uint8_t checksum = calculateChecksum(buffer, index);
    buffer[index++] = checksum;

    return index;
}

// ===== Encoding Methods =====

size_t MessageProtocol::encodeText(const char* text, uint8_t* buffer) {
    size_t textLen = strlen(text);
    if (textLen > MSG_MAX_PAYLOAD) {
        textLen = MSG_MAX_PAYLOAD;
    }

    return encodePacket(MSG_TEXT, (const uint8_t*)text, textLen, buffer);
}

size_t MessageProtocol::encodeSensorRequest(uint8_t sensorId, uint8_t* buffer) {
    uint8_t payload[1];
    payload[0] = sensorId;

    return encodePacket(MSG_SENSOR_REQUEST, payload, 1, buffer);
}

size_t MessageProtocol::encodeSensorResponse(uint8_t sensorId, float value, const char* unit, uint8_t* buffer) {
    uint8_t payload[64];
    size_t index = 0;

    // Sensor ID
    payload[index++] = sensorId;

    // Value (4 bytes float)
    memcpy(&payload[index], &value, sizeof(float));
    index += sizeof(float);

    // Unit string
    size_t unitLen = strlen(unit);
    if (unitLen > 58) {  // Leave room for sensor ID + float
        unitLen = 58;
    }
    memcpy(&payload[index], unit, unitLen);
    index += unitLen;
    payload[index++] = '\0';  // Null terminator

    return encodePacket(MSG_SENSOR_RESPONSE, payload, index, buffer);
}

size_t MessageProtocol::encodeCommand(uint8_t cmdId, const uint8_t* params, size_t paramLen, uint8_t* buffer) {
    uint8_t payload[MSG_MAX_PAYLOAD];
    size_t index = 0;

    // Command ID
    payload[index++] = cmdId;

    // Parameters
    if (params != nullptr && paramLen > 0) {
        size_t maxParams = MSG_MAX_PAYLOAD - 1;
        if (paramLen > maxParams) {
            paramLen = maxParams;
        }
        memcpy(&payload[index], params, paramLen);
        index += paramLen;
    }

    return encodePacket(MSG_COMMAND, payload, index, buffer);
}

size_t MessageProtocol::encodeAck(uint16_t msgId, uint8_t status, uint8_t* buffer) {
    uint8_t payload[3];
    size_t index = 0;

    // Original message ID (2 bytes, big-endian)
    payload[index++] = (msgId >> 8) & 0xFF;
    payload[index++] = msgId & 0xFF;

    // Status code
    payload[index++] = status;

    return encodePacket(MSG_ACK, payload, index, buffer);
}

// ===== Decoding Methods =====

bool MessageProtocol::decode(const uint8_t* buffer, size_t length, Message& msg) {
    // Check minimum packet size
    if (length < MSG_HEADER_SIZE + MSG_CHECKSUM_SIZE) {
        return false;
    }

    // Verify start byte
    if (buffer[0] != MSG_START_BYTE) {
        return false;
    }

    // Verify checksum
    if (!verifyChecksum(buffer, length)) {
        return false;
    }

    size_t index = 0;

    // Skip START byte
    index++;

    // Extract message ID (2 bytes, big-endian)
    msg.messageId = (buffer[index] << 8) | buffer[index + 1];
    index += 2;

    // Extract type
    msg.type = (MessageType)buffer[index++];

    // Extract length
    msg.payloadLength = buffer[index++];

    // Check payload length validity
    if (msg.payloadLength > MSG_MAX_PAYLOAD) {
        return false;
    }

    // Check total length
    if (length != (size_t)(MSG_HEADER_SIZE + msg.payloadLength + MSG_CHECKSUM_SIZE)) {
        return false;
    }

    // Extract payload
    for (uint8_t i = 0; i < msg.payloadLength; i++) {
        msg.payload[i] = buffer[index++];
    }

    // Initialize RSSI and SNR (will be updated by caller)
    msg.rssi = 0;
    msg.snr = 0.0;

    return true;
}

// ===== Utility Methods =====

const char* MessageProtocol::getMessageTypeName(MessageType type) {
    switch (type) {
        case MSG_TEXT: return "TEXT";
        case MSG_SENSOR_REQUEST: return "SENSOR_REQ";
        case MSG_SENSOR_RESPONSE: return "SENSOR_RESP";
        case MSG_COMMAND: return "COMMAND";
        case MSG_ACK: return "ACK";
        case MSG_NACK: return "NACK";
        default: return "UNKNOWN";
    }
}

const char* MessageProtocol::getSensorName(uint8_t sensorId) {
    switch (sensorId) {
        case SENSOR_TEMPERATURE: return "Temperature";
        case SENSOR_HUMIDITY: return "Humidity";
        case SENSOR_BATTERY: return "Battery";
        case SENSOR_PRESSURE: return "Pressure";
        default: return "Unknown";
    }
}

const char* MessageProtocol::getCommandName(uint8_t cmdId) {
    switch (cmdId) {
        case CMD_LED_ON: return "LED_ON";
        case CMD_LED_OFF: return "LED_OFF";
        case CMD_LED_TOGGLE: return "LED_TOGGLE";
        default: return "UNKNOWN";
    }
}

bool MessageProtocol::parseSensorResponse(const uint8_t* payload, uint8_t payloadLength, SensorData& data) {
    if (payloadLength < 6) {  // Minimum: sensor ID + float + null terminator
        return false;
    }

    size_t index = 0;

    // Extract sensor ID
    data.sensorId = payload[index++];

    // Extract value (4 bytes float)
    memcpy(&data.value, &payload[index], sizeof(float));
    index += sizeof(float);

    // Extract unit string
    size_t unitLen = 0;
    while (index < payloadLength && payload[index] != '\0' && unitLen < 15) {
        data.unit[unitLen++] = payload[index++];
    }
    data.unit[unitLen] = '\0';

    return true;
}
