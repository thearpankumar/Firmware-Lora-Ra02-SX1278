#ifndef LORA_COMM_H
#define LORA_COMM_H

#include <Arduino.h>
#include <LoRa.h>

class LoRaComm {
public:
    LoRaComm();

    // Initialize LoRa module with board-specific pins
    bool begin();

    // Send raw packet data
    bool sendPacket(const uint8_t* data, size_t length);

    // Receive packet data (non-blocking)
    // Returns number of bytes received, 0 if no packet
    int receivePacket(uint8_t* buffer, size_t maxLength);

    // Check if a packet is available
    bool isPacketAvailable();

    // Get signal strength of last received packet
    int getRSSI();

    // Get signal-to-noise ratio of last received packet
    float getSNR();

    // Check if currently transmitting
    bool isTransmitting();

    // Set receive callback (interrupt-driven)
    void onReceive(void (*callback)(int));

    // Get current configuration info
    void printConfig();

private:
    int lastRSSI;
    float lastSNR;
};

#endif // LORA_COMM_H
