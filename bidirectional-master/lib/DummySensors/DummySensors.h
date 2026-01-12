#ifndef DUMMY_SENSORS_H
#define DUMMY_SENSORS_H

#include <Arduino.h>

class DummySensors {
public:
    DummySensors();

    // Initialize sensors (seed random number generator)
    void begin();

    // Read temperature in Celsius
    float readTemperature();

    // Read humidity in percentage
    float readHumidity();

    // Read battery voltage in volts
    float readBatteryVoltage();

    // Read atmospheric pressure in hPa
    float readPressure();

    // Get sensor value by ID (for generic requests)
    float readSensorById(uint8_t sensorId);

    // Get sensor name by ID
    const char* getSensorName(uint8_t sensorId);

    // Get sensor unit by ID
    const char* getSensorUnit(uint8_t sensorId);

private:
    // Base values for realistic drift simulation
    float temperatureBase;
    float humidityBase;
    float batteryBase;
    float pressureBase;

    // Random variation helper
    float addVariation(float base, float minVal, float maxVal, float variationAmount);
};

#endif // DUMMY_SENSORS_H
