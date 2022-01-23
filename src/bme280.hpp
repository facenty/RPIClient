#ifndef BME280_HPP
#define BME280_HPP

#include <stdint.h>
#include <memory>

#include "scopedFd.hpp"

class Bme280 {
public:


    Bme280() = default;
    bool Init();

    struct SensorsData {
        float temperature = 0.0;
        float pressure = 0.0;
        float humidity = 0.0;
    };
    SensorsData ReadSensorsData();

private:

    void ReadCalibrationData();
    void CalculateCalibration(int32_t rawTemperature);
    float CompensateTemperature();
    float CompensatePressure(int32_t rawpressure);
    float CompensateHumidity(int32_t rawHumidity);

    struct Bme280CalibData
    {
        uint16_t digT1 = 0;
        int16_t  digT2 = 0;
        int16_t  digT3 = 0;

        uint16_t digP1 = 0;
        int16_t  digP2 = 0;
        int16_t  digP3 = 0;
        int16_t  digP4 = 0;
        int16_t  digP5 = 0;
        int16_t  digP6 = 0;
        int16_t  digP7 = 0;
        int16_t  digP8 = 0;
        int16_t  digP9 = 0;

        uint8_t  digH1 = 0;
        int16_t  digH2 = 0;
        uint8_t  digH3 = 0;
        int16_t  digH4 = 0;
        int16_t  digH5 = 0;
        int8_t   digH6 = 0;
    };

    int32_t calibration_;
    Bme280CalibData calibrationData_;
    ScopedFd i2cFd_;
};

#endif // GPRS_HPP