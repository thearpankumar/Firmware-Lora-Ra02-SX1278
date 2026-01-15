#ifndef DUAL_LORA_COMM_H
#define DUAL_LORA_COMM_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "board_config.h"

// Number of LoRa modules
#define NUM_LORA_MODULES 2

// Module indices
#define MODULE_1 0
#define MODULE_2 1

class DualLoRaComm {
public:
    DualLoRaComm();

    // Initialize both LoRa modules
    bool begin();

    // Send packet via specified module (0 or 1)
    bool sendPacket(uint8_t moduleIndex, const uint8_t* data, size_t length);

    // Get device name for module
    const char* getDeviceName(uint8_t moduleIndex);

    // Print configuration for debugging
    void printConfig();

private:
    // Internal LoRaClass instances
    LoRaClass lora1;
    LoRaClass lora2;

    // Module names
    const char* deviceNames[NUM_LORA_MODULES];

    // Pin configurations
    int nssPins[NUM_LORA_MODULES];
    int dio0Pins[NUM_LORA_MODULES];
    int resetPins[NUM_LORA_MODULES];

    // Initialize a single module
    bool initModule(LoRaClass& lora, int nss, int dio0, int rst, const char* name);

    // Select a module for communication (deselect others)
    void selectModule(uint8_t moduleIndex);

    // Configure LoRa parameters for a module
    void configureModule(LoRaClass& lora);
};

#endif // DUAL_LORA_COMM_H
