#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Pin definitions are provided via build flags in platformio.ini
// This header validates that all required pins are defined at compile time

// ===== Module 1 Pin Validation =====
#ifndef LORA1_NSS
    #error "LORA1_NSS not defined. Check platformio.ini build_flags"
#endif

#ifndef LORA1_DIO0
    #error "LORA1_DIO0 not defined. Check platformio.ini build_flags"
#endif

#ifndef LORA1_RESET
    #error "LORA1_RESET not defined. Check platformio.ini build_flags"
#endif

#ifndef LORA1_NAME
    #define LORA1_NAME "trident1"
#endif

// ===== Module 2 Pin Validation =====
#ifndef LORA2_NSS
    #error "LORA2_NSS not defined. Check platformio.ini build_flags"
#endif

#ifndef LORA2_DIO0
    #error "LORA2_DIO0 not defined. Check platformio.ini build_flags"
#endif

#ifndef LORA2_RESET
    #error "LORA2_RESET not defined. Check platformio.ini build_flags"
#endif

#ifndef LORA2_NAME
    #define LORA2_NAME "trident2"
#endif

// ===== Common Validation =====
#ifndef LORA_FREQUENCY
    #error "LORA_FREQUENCY not defined. Check platformio.ini build_flags"
#endif

#ifndef BOARD_NAME
    #error "BOARD_NAME not defined. Check platformio.ini build_flags"
#endif

// ===== LoRa Configuration Parameters =====
#define LORA_SPREADING_FACTOR 7         // SF7-SF12 (7=fast/short, 12=slow/long)
#define LORA_SIGNAL_BANDWIDTH 125E3     // 125 kHz bandwidth
#define LORA_CODING_RATE 5              // 4/5 coding rate
#define LORA_PREAMBLE_LENGTH 8          // Preamble length
#define LORA_SYNC_WORD 0x12             // Private network sync word
#define LORA_TX_POWER 17                // TX power in dBm (2-20)

// ===== Serial Configuration =====
#define SERIAL_BAUD 9600

#endif // BOARD_CONFIG_H
