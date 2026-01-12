#include "SerialCommands.h"
#include "DummySensors.h"

SerialCommands::SerialCommands() {
    inputBuffer = "";
}

void SerialCommands::begin() {
    inputBuffer.reserve(128);  // Pre-allocate buffer
    Serial.println(F("\nType 'help' for available commands\n"));
}

bool SerialCommands::available() {
    return Serial.available() > 0;
}

bool SerialCommands::readCommand(Command& cmd) {
    while (Serial.available() > 0) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                // Parse command
                parseCommand(inputBuffer, cmd);
                inputBuffer = "";
                return true;
            }
        } else {
            inputBuffer += c;
        }
    }
    return false;
}

void SerialCommands::parseCommand(const String& input, Command& cmd) {
    // Clear command structure
    cmd.name = "";
    cmd.arg1 = "";
    cmd.arg2 = "";
    cmd.arg3 = "";

    // Trim whitespace
    String trimmed = input;
    trimmed.trim();

    // Split by spaces
    int firstSpace = trimmed.indexOf(' ');
    if (firstSpace == -1) {
        // Single word command
        cmd.name = trimmed;
        cmd.name.toLowerCase();
        return;
    }

    // Extract command name
    cmd.name = trimmed.substring(0, firstSpace);
    cmd.name.toLowerCase();

    // Extract arguments
    String remaining = trimmed.substring(firstSpace + 1);
    remaining.trim();

    int secondSpace = remaining.indexOf(' ');
    if (secondSpace == -1) {
        cmd.arg1 = remaining;
        cmd.arg1.toLowerCase();
        return;
    }

    cmd.arg1 = remaining.substring(0, secondSpace);
    cmd.arg1.toLowerCase();

    remaining = remaining.substring(secondSpace + 1);
    remaining.trim();

    int thirdSpace = remaining.indexOf(' ');
    if (thirdSpace == -1) {
        cmd.arg2 = remaining;
        cmd.arg2.toLowerCase();
        return;
    }

    cmd.arg2 = remaining.substring(0, thirdSpace);
    cmd.arg2.toLowerCase();

    cmd.arg3 = remaining.substring(thirdSpace + 1);
    cmd.arg3.trim();
}

void SerialCommands::printHelp() {
    Serial.println(F("\n========== AVAILABLE COMMANDS =========="));
    Serial.println(F("help                    - Show this help menu"));
    Serial.println(F("send <text>             - Send text message"));
    Serial.println(F("request temp            - Request temperature"));
    Serial.println(F("request humid           - Request humidity"));
    Serial.println(F("request bat             - Request battery voltage"));
    Serial.println(F("request pressure        - Request pressure"));
    Serial.println(F("cmd led on              - LED on command"));
    Serial.println(F("cmd led off             - LED off command"));
    Serial.println(F("cmd led toggle          - LED toggle command"));
    Serial.println(F("stats                   - Show statistics"));
    Serial.println(F("clear                   - Clear statistics"));
    Serial.println(F("========================================\n"));
}

void SerialCommands::printStats(const Statistics& stats) {
    Serial.println(F("\n========== STATISTICS =========="));
    Serial.print(F("Messages Sent:     "));
    Serial.println(stats.messagesSent);

    Serial.print(F("Messages Received: "));
    Serial.println(stats.messagesReceived);

    Serial.print(F("Messages Failed:   "));
    Serial.println(stats.messagesFailed);

    float successRate = 0.0;
    if (stats.messagesSent > 0) {
        successRate = ((float)(stats.messagesSent - stats.messagesFailed) / stats.messagesSent) * 100.0;
    }
    Serial.print(F("Success Rate:      "));
    Serial.print(successRate, 1);
    Serial.println(F("%"));

    Serial.print(F("Retries:           "));
    Serial.println(stats.retries);

    if (stats.rssiCount > 0) {
        float avgRSSI = (float)stats.totalRSSI / stats.rssiCount;
        Serial.print(F("Avg RSSI:          "));
        Serial.print(avgRSSI, 1);
        Serial.println(F(" dBm"));
    }

    Serial.print(F("Uptime:            "));
    Serial.println(getUptime(stats.startTime));

    Serial.println(F("================================\n"));
}

void SerialCommands::clearStats(Statistics& stats) {
    stats.messagesSent = 0;
    stats.messagesReceived = 0;
    stats.messagesFailed = 0;
    stats.retries = 0;
    stats.totalRSSI = 0;
    stats.rssiCount = 0;
    stats.startTime = millis();
    Serial.println(F("[INFO] Statistics cleared"));
}

void SerialCommands::printReceivedMessage(const Message& msg, const char* content) {
    Serial.print(F("["));
    Serial.print(getTimestamp());
    Serial.print(F("] RX << "));

    MessageProtocol protocol;
    Serial.print(protocol.getMessageTypeName(msg.type));
    Serial.print(F(": \""));
    Serial.print(content);
    Serial.print(F("\" [ID: "));
    Serial.print(msg.messageId);
    Serial.print(F(", RSSI: "));
    Serial.print(msg.rssi);
    Serial.print(F(" dBm, SNR: "));
    Serial.print(msg.snr, 1);
    Serial.println(F(" dB]"));
}

void SerialCommands::printSentMessage(const char* type, const char* content, bool success) {
    Serial.print(F("["));
    Serial.print(getTimestamp());
    Serial.print(F("] TX >> "));
    Serial.print(type);
    Serial.print(F(": \""));
    Serial.print(content);
    Serial.print(F("\" ... "));
    Serial.println(success ? F("SENT") : F("FAILED"));
}

void SerialCommands::printSensorData(const SensorData& data) {
    Serial.print(F("[SENSOR] "));
    DummySensors sensors;
    Serial.print(sensors.getSensorName(data.sensorId));
    Serial.print(F(": "));
    Serial.print(data.value, 2);
    Serial.print(F(" "));
    Serial.println(data.unit);
}

void SerialCommands::printCommandExecution(uint8_t cmdId, const char* cmdName) {
    Serial.print(F("[COMMAND] Executed: "));
    Serial.print(cmdName);
    Serial.print(F(" (ID: 0x"));
    Serial.print(cmdId, HEX);
    Serial.println(F(")"));
}

void SerialCommands::printAckReceived(uint16_t msgId, bool success) {
    Serial.print(F("[ACK] Message "));
    Serial.print(msgId);
    Serial.print(F(" "));
    Serial.println(success ? F("acknowledged") : F("failed"));
}

void SerialCommands::printError(const char* message) {
    Serial.print(F("[ERROR] "));
    Serial.println(message);
}

void SerialCommands::printInfo(const char* message) {
    Serial.print(F("[INFO] "));
    Serial.println(message);
}

String SerialCommands::getUptime(unsigned long startTime) {
    unsigned long uptime = (millis() - startTime) / 1000;  // Convert to seconds

    unsigned long hours = uptime / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;

    char buffer[16];
    sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(buffer);
}

float SerialCommands::getAverageRSSI(const Statistics& stats) {
    if (stats.rssiCount == 0) {
        return 0.0;
    }
    return (float)stats.totalRSSI / stats.rssiCount;
}

String SerialCommands::getTimestamp() {
    unsigned long currentMillis = millis();
    unsigned long seconds = currentMillis / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    seconds = seconds % 60;
    minutes = minutes % 60;
    hours = hours % 24;

    char buffer[16];
    sprintf(buffer, "%02lu:%02lu:%02lu.%03lu", hours, minutes, seconds, currentMillis % 1000);
    return String(buffer);
}
