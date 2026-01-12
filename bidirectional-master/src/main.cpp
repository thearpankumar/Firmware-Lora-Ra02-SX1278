#include <Arduino.h>
#include "LoRaComm.h"
#include "MessageProtocol.h"
#include "DummySensors.h"
#include "SerialCommands.h"
#include "board_config.h"

// ===== State Machine =====
enum State {
    STATE_IDLE,
    STATE_TX_WAIT_ACK,
    STATE_RX_PROCESSING,
    STATE_ERROR
};

// ===== Global Objects =====
LoRaComm loraComm;
MessageProtocol protocol;
DummySensors sensors;
SerialCommands serialCmd;

// ===== State Variables =====
State currentState = STATE_IDLE;
Statistics stats = {0, 0, 0, 0, 0, 0, 0};

// ===== Pending Message Tracking =====
uint16_t pendingMessageId = 0;
unsigned long txTimestamp = 0;
uint8_t retryCount = 0;
size_t lastTxLength = 0;  // Store actual packet length for retries

// ===== Configuration =====
const unsigned long ACK_TIMEOUT = 2000;  // 2 seconds
const uint8_t MAX_RETRIES = 3;
const unsigned long RETRY_DELAYS[] = {500, 1000, 2000};  // Exponential backoff

// ===== Buffers =====
uint8_t txBuffer[MSG_MAX_PACKET_SIZE];
uint8_t rxBuffer[MSG_MAX_PACKET_SIZE];
Message lastTxMessage;
Message lastRxMessage;

// ===== Function Prototypes =====
void handleIdle();
void handleTxWaitAck();
void handleRxProcessing();
void handleError();
void processSerialCommand();
void sendTextMessage(const char* text);
void sendSensorRequest(uint8_t sensorId);
void sendCommand(uint8_t cmdId);
void sendAck(uint16_t msgId, uint8_t status);
void checkLoRaReceive();
bool retryMessage();

void setup() {
    // Initialize Serial
    Serial.begin(SERIAL_BAUD);
    delay(1500);

    // Print banner
    Serial.println(F("\n\n"));
    Serial.println(F("===================================="));
    Serial.println(F("  LoRa Ra-02 SX1278 Firmware"));
    Serial.println(F("  Bidirectional Communication"));
    Serial.println(F("===================================="));
    Serial.print(F("Board: "));
    Serial.println(BOARD_NAME);
    Serial.println();

    // Initialize LoRa
    Serial.println(F("Initializing LoRa module..."));
    if (!loraComm.begin()) {
        Serial.println(F("\nFATAL: LoRa initialization failed!"));
        Serial.println(F("System halted. Check wiring and reset board."));
        while (1) {
            delay(1000);
        }
    }

    // Initialize sensors
    sensors.begin();
    Serial.println(F("Dummy sensors initialized"));

    // Initialize serial commands
    serialCmd.begin();

    // Initialize statistics
    stats.startTime = millis();

    // Print ready message
    Serial.println();
    Serial.println(F("===================================="));
    Serial.println(F("  System Ready"));
    Serial.println(F("===================================="));
    Serial.println(F("Request-Response Pattern Active"));
    Serial.println(F("- Text messages"));
    Serial.println(F("- Sensor data (dummy)"));
    Serial.println(F("- Commands"));
    Serial.println(F("- ACK with retries"));
    Serial.println(F("===================================="));

    serialCmd.printHelp();
}

void loop() {
    // Check for incoming LoRa messages
    checkLoRaReceive();

    // Process serial commands (if in IDLE state)
    if (currentState == STATE_IDLE && serialCmd.available()) {
        processSerialCommand();
    }

    // Update state machine
    switch (currentState) {
        case STATE_IDLE:
            handleIdle();
            break;

        case STATE_TX_WAIT_ACK:
            handleTxWaitAck();
            break;

        case STATE_RX_PROCESSING:
            handleRxProcessing();
            break;

        case STATE_ERROR:
            handleError();
            break;
    }

    // Small delay
    delay(10);
}

void handleIdle() {
    // Nothing to do, waiting for command or incoming message
}

void handleTxWaitAck() {
    // Check timeout
    unsigned long elapsed = millis() - txTimestamp;

    if (elapsed >= ACK_TIMEOUT) {
        // Timeout occurred
        if (retryCount < MAX_RETRIES) {
            // Retry
            serialCmd.printInfo("ACK timeout, retrying...");
            delay(RETRY_DELAYS[retryCount]);
            retryMessage();
        } else {
            // Max retries exceeded
            serialCmd.printError("Max retries exceeded, message failed");
            stats.messagesFailed++;
            pendingMessageId = 0;
            currentState = STATE_IDLE;
        }
    }
}

void handleRxProcessing() {
    // Process received message based on type
    switch (lastRxMessage.type) {
        case MSG_TEXT: {
            // Print text message
            char textBuffer[MSG_MAX_PAYLOAD + 1];
            memcpy(textBuffer, lastRxMessage.payload, lastRxMessage.payloadLength);
            textBuffer[lastRxMessage.payloadLength] = '\0';
            serialCmd.printReceivedMessage(lastRxMessage, textBuffer);

            // Send ACK
            sendAck(lastRxMessage.messageId, ACK_OK);
            break;
        }

        case MSG_SENSOR_REQUEST: {
            if (lastRxMessage.payloadLength >= 1) {
                uint8_t sensorId = lastRxMessage.payload[0];

                Serial.print(F("[RX] Sensor request: "));
                Serial.println(sensors.getSensorName(sensorId));

                // Send ACK first
                sendAck(lastRxMessage.messageId, ACK_OK);

                // Wait for ACK to be fully transmitted and received
                delay(100);

                // Read sensor and send response
                float value = sensors.readSensorById(sensorId);
                const char* unit = sensors.getSensorUnit(sensorId);

                size_t len = protocol.encodeSensorResponse(sensorId, value, unit, txBuffer);
                if (len > 0 && loraComm.sendPacket(txBuffer, len)) {
                    Serial.print(F("[TX] Sensor response: "));
                    Serial.print(value, 2);
                    Serial.print(F(" "));
                    Serial.println(unit);
                }
            }
            break;
        }

        case MSG_SENSOR_RESPONSE: {
            // Parse sensor response
            SensorData data;
            if (protocol.parseSensorResponse(lastRxMessage.payload, lastRxMessage.payloadLength, data)) {
                serialCmd.printSensorData(data);
                sendAck(lastRxMessage.messageId, ACK_OK);
            } else {
                serialCmd.printError("Failed to parse sensor response");
                sendAck(lastRxMessage.messageId, ACK_ERROR);
            }
            break;
        }

        case MSG_COMMAND: {
            if (lastRxMessage.payloadLength >= 1) {
                uint8_t cmdId = lastRxMessage.payload[0];
                const char* cmdName = protocol.getCommandName(cmdId);

                serialCmd.printCommandExecution(cmdId, cmdName);

                // Send ACK
                sendAck(lastRxMessage.messageId, ACK_OK);
            }
            break;
        }

        case MSG_ACK: {
            // Check if this ACK is for our pending message
            if (lastRxMessage.payloadLength >= 3) {
                uint16_t ackedMsgId = (lastRxMessage.payload[0] << 8) | lastRxMessage.payload[1];
                uint8_t status = lastRxMessage.payload[2];

                if (ackedMsgId == pendingMessageId) {
                    // Our message was acknowledged
                    serialCmd.printAckReceived(ackedMsgId, status == ACK_OK);
                    pendingMessageId = 0;
                    currentState = STATE_IDLE;
                }
            }
            break;
        }

        case MSG_NACK: {
            serialCmd.printError("Received NACK");
            break;
        }

        default:
            serialCmd.printError("Unknown message type");
            break;
    }

    // Return to idle
    if (currentState == STATE_RX_PROCESSING) {
        currentState = STATE_IDLE;
    }
}

void handleError() {
    serialCmd.printError("System error, resetting to idle");
    currentState = STATE_IDLE;
}

void processSerialCommand() {
    Command cmd;
    if (!serialCmd.readCommand(cmd)) {
        return;
    }

    // Process command
    if (cmd.name == "help") {
        serialCmd.printHelp();
    }
    else if (cmd.name == "send") {
        if (cmd.arg1.length() > 0) {
            // Reconstruct full text (arg1 + arg2 + arg3)
            String fullText = cmd.arg1;
            if (cmd.arg2.length() > 0) fullText += " " + cmd.arg2;
            if (cmd.arg3.length() > 0) fullText += " " + cmd.arg3;
            sendTextMessage(fullText.c_str());
        } else {
            serialCmd.printError("Usage: send <text>");
        }
    }
    else if (cmd.name == "request") {
        if (cmd.arg1 == "temp") {
            sendSensorRequest(SENSOR_TEMPERATURE);
        } else if (cmd.arg1 == "humid") {
            sendSensorRequest(SENSOR_HUMIDITY);
        } else if (cmd.arg1 == "bat") {
            sendSensorRequest(SENSOR_BATTERY);
        } else if (cmd.arg1 == "pressure") {
            sendSensorRequest(SENSOR_PRESSURE);
        } else {
            serialCmd.printError("Usage: request [temp|humid|bat|pressure]");
        }
    }
    else if (cmd.name == "cmd") {
        if (cmd.arg1 == "led" && cmd.arg2 == "on") {
            sendCommand(CMD_LED_ON);
        } else if (cmd.arg1 == "led" && cmd.arg2 == "off") {
            sendCommand(CMD_LED_OFF);
        } else if (cmd.arg1 == "led" && cmd.arg2 == "toggle") {
            sendCommand(CMD_LED_TOGGLE);
        } else {
            serialCmd.printError("Usage: cmd led [on|off|toggle]");
        }
    }
    else if (cmd.name == "stats") {
        serialCmd.printStats(stats);
    }
    else if (cmd.name == "clear") {
        serialCmd.clearStats(stats);
    }
    else {
        serialCmd.printError("Unknown command. Type 'help' for list.");
    }
}

void sendTextMessage(const char* text) {
    size_t len = protocol.encodeText(text, txBuffer);
    if (len == 0) {
        serialCmd.printError("Failed to encode message");
        return;
    }

    // Decode to get message ID
    Message msg;
    if (protocol.decode(txBuffer, len, msg)) {
        pendingMessageId = msg.messageId;
    }

    // Store packet length for retries
    lastTxLength = len;
    Serial.print(F("[DEBUG] Stored packet length: "));
    Serial.println(lastTxLength);

    if (loraComm.sendPacket(txBuffer, len)) {
        serialCmd.printSentMessage("TEXT", text, true);
        stats.messagesSent++;
        txTimestamp = millis();
        retryCount = 0;
        currentState = STATE_TX_WAIT_ACK;
    } else {
        serialCmd.printSentMessage("TEXT", text, false);
        stats.messagesFailed++;
    }
}

void sendSensorRequest(uint8_t sensorId) {
    size_t len = protocol.encodeSensorRequest(sensorId, txBuffer);
    if (len == 0) {
        serialCmd.printError("Failed to encode sensor request");
        return;
    }

    // Decode to get message ID
    Message msg;
    if (protocol.decode(txBuffer, len, msg)) {
        pendingMessageId = msg.messageId;
    }

    // Store packet length for retries
    lastTxLength = len;

    const char* sensorName = sensors.getSensorName(sensorId);

    if (loraComm.sendPacket(txBuffer, len)) {
        serialCmd.printSentMessage("SENSOR_REQ", sensorName, true);
        stats.messagesSent++;
        txTimestamp = millis();
        retryCount = 0;
        currentState = STATE_TX_WAIT_ACK;
    } else {
        serialCmd.printSentMessage("SENSOR_REQ", sensorName, false);
        stats.messagesFailed++;
    }
}

void sendCommand(uint8_t cmdId) {
    size_t len = protocol.encodeCommand(cmdId, nullptr, 0, txBuffer);
    if (len == 0) {
        serialCmd.printError("Failed to encode command");
        return;
    }

    // Decode to get message ID
    Message msg;
    if (protocol.decode(txBuffer, len, msg)) {
        pendingMessageId = msg.messageId;
    }

    // Store packet length for retries
    lastTxLength = len;

    const char* cmdName = protocol.getCommandName(cmdId);

    if (loraComm.sendPacket(txBuffer, len)) {
        serialCmd.printSentMessage("COMMAND", cmdName, true);
        stats.messagesSent++;
        txTimestamp = millis();
        retryCount = 0;
        currentState = STATE_TX_WAIT_ACK;
    } else {
        serialCmd.printSentMessage("COMMAND", cmdName, false);
        stats.messagesFailed++;
    }
}

void sendAck(uint16_t msgId, uint8_t status) {
    size_t len = protocol.encodeAck(msgId, status, txBuffer);
    if (len > 0) {
        loraComm.sendPacket(txBuffer, len);
        Serial.print(F("[TX] ACK sent for message "));
        Serial.println(msgId);
    }
}

void checkLoRaReceive() {
    int packetSize = loraComm.receivePacket(rxBuffer, sizeof(rxBuffer));

    if (packetSize > 0) {
        // Debug: Print received packet details
        Serial.print(F("[DEBUG] Received packet: "));
        Serial.print(packetSize);
        Serial.print(F(" bytes, RSSI: "));
        Serial.println(loraComm.getRSSI());

        // Update statistics
        stats.messagesReceived++;
        stats.totalRSSI += loraComm.getRSSI();
        stats.rssiCount++;

        // Decode message
        if (protocol.decode(rxBuffer, packetSize, lastRxMessage)) {
            // Store RSSI and SNR
            lastRxMessage.rssi = loraComm.getRSSI();
            lastRxMessage.snr = loraComm.getSNR();

            // If we're waiting for ACK and received an ACK, handle in TX_WAIT_ACK state
            if (currentState == STATE_TX_WAIT_ACK && lastRxMessage.type == MSG_ACK) {
                // Process immediately
                handleRxProcessing();
            }
            // Otherwise, process in next cycle
            else if (currentState == STATE_IDLE) {
                currentState = STATE_RX_PROCESSING;
            }
        } else {
            serialCmd.printError("Failed to decode packet (checksum error?)");
        }
    }
}

bool retryMessage() {
    // Resend last message
    if (pendingMessageId == 0 || lastTxLength == 0) {
        Serial.print(F("[DEBUG] Retry failed: pendingMessageId="));
        Serial.print(pendingMessageId);
        Serial.print(F(", lastTxLength="));
        Serial.println(lastTxLength);
        return false;
    }

    retryCount++;
    stats.retries++;

    // Debug: Print retry details
    Serial.print(F("[DEBUG] Retry #"));
    Serial.print(retryCount);
    Serial.print(F(" - Sending "));
    Serial.print(lastTxLength);
    Serial.print(F(" bytes (msgID: "));
    Serial.print(pendingMessageId);
    Serial.println(F(")"));

    // Resend with correct packet length
    if (loraComm.sendPacket(txBuffer, lastTxLength)) {
        serialCmd.printInfo("Message retransmitted");
        txTimestamp = millis();
        return true;
    } else {
        serialCmd.printError("Retry transmission failed");
        return false;
    }
}
