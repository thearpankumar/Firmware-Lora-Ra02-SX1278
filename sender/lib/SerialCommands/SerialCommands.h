#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include <Arduino.h>
#include "MessageProtocol.h"

// Command result codes
enum CommandResult {
    CMD_OK = 0,
    CMD_ERROR = 1,
    CMD_INVALID = 2,
    CMD_HELP = 3
};

// Command structure
struct Command {
    String name;
    String arg1;
    String arg2;
    String arg3;
};

// Statistics structure
struct Statistics {
    uint32_t messagesSent;
    uint32_t messagesReceived;
    uint32_t messagesFailed;
    uint32_t retries;
    int32_t totalRSSI;
    uint32_t rssiCount;
    unsigned long startTime;
};

class SerialCommands {
public:
    SerialCommands();

    // Initialize serial commands
    void begin();

    // Check if command is available
    bool available();

    // Read and parse command
    bool readCommand(Command& cmd);

    // Print formatted help menu
    void printHelp();

    // Print statistics
    void printStats(const Statistics& stats);

    // Clear statistics
    void clearStats(Statistics& stats);

    // Print received message
    void printReceivedMessage(const Message& msg, const char* content);

    // Print sent message status
    void printSentMessage(const char* type, const char* content, bool success);

    // Print sensor data
    void printSensorData(const SensorData& data);

    // Print command execution
    void printCommandExecution(uint8_t cmdId, const char* cmdName);

    // Print ACK received
    void printAckReceived(uint16_t msgId, bool success);

    // Print error message
    void printError(const char* message);

    // Print info message
    void printInfo(const char* message);

    // Get uptime string
    String getUptime(unsigned long startTime);

    // Get average RSSI
    float getAverageRSSI(const Statistics& stats);

private:
    String inputBuffer;

    // Parse command string into Command structure
    void parseCommand(const String& input, Command& cmd);

    // Get current timestamp string
    String getTimestamp();
};

#endif // SERIAL_COMMANDS_H
