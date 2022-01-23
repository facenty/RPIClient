#include "bme280.hpp"

#include <boost/log/trivial.hpp>
#include <wiringPiI2C.h>


namespace
{
  constexpr int bme280Address = 0x77;
  constexpr uint meanSeaLevelPressure = 1013;

#define BME280_REGISTER_DIG_T1        0x88
#define BME280_REGISTER_DIG_T2        0x8A
#define BME280_REGISTER_DIG_T3        0x8C
#define BME280_REGISTER_DIG_P1        0x8E
#define BME280_REGISTER_DIG_P2        0x90
#define BME280_REGISTER_DIG_P3        0x92
#define BME280_REGISTER_DIG_P4        0x94
#define BME280_REGISTER_DIG_P5        0x96
#define BME280_REGISTER_DIG_P6        0x98
#define BME280_REGISTER_DIG_P7        0x9A
#define BME280_REGISTER_DIG_P8        0x9C
#define BME280_REGISTER_DIG_P9        0x9E
#define BME280_REGISTER_DIG_H1        0xA1
#define BME280_REGISTER_DIG_H2        0xE1
#define BME280_REGISTER_DIG_H3        0xE3
#define BME280_REGISTER_DIG_H4        0xE4
#define BME280_REGISTER_DIG_H5        0xE5
#define BME280_REGISTER_DIG_H6        0xE7
#define BME280_REGISTER_CHIPID        0xD0
#define BME280_REGISTER_VERSION       0xD1
#define BME280_REGISTER_SOFTRESET     0xE0
#define BME280_RESET                  0xB6
#define BME280_REGISTER_CAL26         0xE1
#define BME280_REGISTER_CONTROLHUMID  0xF2
#define BME280_REGISTER_CONTROL       0xF4
#define BME280_REGISTER_CONFIG        0xF5
#define BME280_REGISTER_PRESSUREDATA  0xF7
#define BME280_REGISTER_TEMPDATA      0xFA
#define BME280_REGISTER_HUMIDDATA     0xFD

}

bool Bme280::Init() {
  auto fd = wiringPiI2CSetup(bme280Address);
  if (fd < 0) {
    BOOST_LOG_TRIVIAL(fatal) << "Failed to open i2c device";
    return false;
  }

  i2cFd_.reset(fd);

  // wiringPiI2CWriteReg8(i2cFd_.get(), BME280_REGISTER_SOFTRESET, 0xB6);
  ReadCalibrationData();
  wiringPiI2CWriteReg8(i2cFd_.get(), 0xf2, 0x01);   // humidity oversampling x 1
  wiringPiI2CWriteReg8(i2cFd_.get(), 0xf4, 0x25);

  return true;

}

void Bme280::ReadCalibrationData() {
  calibrationData_.digT1 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_T1);
  calibrationData_.digT2 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_T2);
  calibrationData_.digT3 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_T3);

  calibrationData_.digP1 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P1);
  calibrationData_.digP2 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P2);
  calibrationData_.digP3 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P3);
  calibrationData_.digP4 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P4);
  calibrationData_.digP5 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P5);
  calibrationData_.digP6 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P6);
  calibrationData_.digP7 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P7);
  calibrationData_.digP8 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P8);
  calibrationData_.digP9 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_P9);

  calibrationData_.digH1 = wiringPiI2CReadReg8(i2cFd_.get(), BME280_REGISTER_DIG_H1);
  calibrationData_.digH2 = wiringPiI2CReadReg16(i2cFd_.get(), BME280_REGISTER_DIG_H2);
  calibrationData_.digH3 = wiringPiI2CReadReg8(i2cFd_.get(), BME280_REGISTER_DIG_H3);
  calibrationData_.digH4 = (wiringPiI2CReadReg8(i2cFd_.get(), BME280_REGISTER_DIG_H4) << 4) | (wiringPiI2CReadReg8(i2cFd_.get(), BME280_REGISTER_DIG_H4 + 1) & 0xF);
  calibrationData_.digH5 = (wiringPiI2CReadReg8(i2cFd_.get(), BME280_REGISTER_DIG_H5 + 1) << 4) | (wiringPiI2CReadReg8(i2cFd_.get(), BME280_REGISTER_DIG_H5) >> 4);
  calibrationData_.digH6 = wiringPiI2CReadReg8(i2cFd_.get(), BME280_REGISTER_DIG_H6);
}

Bme280::SensorsData Bme280::ReadSensorsData() {

  wiringPiI2CWrite(i2cFd_.get(), 0xf7);

  uint8_t pmsb = wiringPiI2CRead(i2cFd_.get());
  uint8_t plsb = wiringPiI2CRead(i2cFd_.get());
  uint8_t pxsb = wiringPiI2CRead(i2cFd_.get());

  uint8_t tmsb = wiringPiI2CRead(i2cFd_.get());
  uint8_t tlsb = wiringPiI2CRead(i2cFd_.get());
  uint8_t txsb = wiringPiI2CRead(i2cFd_.get());

  uint8_t hmsb = wiringPiI2CRead(i2cFd_.get());
  uint8_t hlsb = wiringPiI2CRead(i2cFd_.get());

  uint32_t temperature = 0;
  temperature = (temperature | tmsb) << 8;
  temperature = (temperature | tlsb) << 8;
  temperature = (temperature | txsb) >> 4;
  uint32_t pressure = 0;
  pressure = (pressure | pmsb) << 8;
  pressure = (pressure | plsb) << 8;
  pressure = (pressure | pxsb) >> 4;
  uint32_t humidity = 0;
  humidity = (humidity | hmsb) << 8;
  humidity = (humidity | hlsb);

  CalculateCalibration(temperature);
  SensorsData sensorsData;
  sensorsData.temperature = CompensateTemperature();
  sensorsData.pressure = CompensatePressure(pressure);
  sensorsData.humidity = CompensateHumidity(humidity);
  return sensorsData;
}

void Bme280::CalculateCalibration(int32_t rawTemperature) {


  int32_t var1 = ((((rawTemperature >> 3) - ((int32_t)calibrationData_.digT1 << 1))) *
    ((int32_t)calibrationData_.digT2)) >> 11;

  int32_t var2 = (((((rawTemperature >> 4) - ((int32_t)calibrationData_.digT1)) *
    ((rawTemperature >> 4) - ((int32_t)calibrationData_.digT1))) >> 12) *
    ((int32_t)calibrationData_.digT3)) >> 14;

  calibration_ = var1 + var2;
}

float Bme280::CompensateTemperature() {
  float temp = (calibration_ * 5 + 128) >> 8;
  return temp / 100;
}

float Bme280::CompensatePressure(int32_t rawpressure) {
  int64_t var1, var2, p;

  var1 = ((int64_t)calibration_) - 128000;
  var2 = var1 * var1 * (int64_t)calibrationData_.digP6;
  var2 = var2 + ((var1 * (int64_t)calibrationData_.digP5) << 17);
  var2 = var2 + (((int64_t)calibrationData_.digP4) << 35);
  var1 = ((var1 * var1 * (int64_t)calibrationData_.digP3) >> 8) +
    ((var1 * (int64_t)calibrationData_.digP2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calibrationData_.digP1) >> 33;

  if (var1 == 0) {
    return 0;
  }
  p = 1048576 - rawpressure;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)calibrationData_.digP9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)calibrationData_.digP8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)calibrationData_.digP7) << 4);
  return (float)p / 256;

}

float Bme280::CompensateHumidity(int32_t rawHumidity) {
  int32_t v_x1_u32r;

  v_x1_u32r = (calibration_ - ((int32_t)76800));

  v_x1_u32r = (((((rawHumidity << 14) - (((int32_t)calibrationData_.digH4) << 20) -
    (((int32_t)calibrationData_.digH5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
    (((((((v_x1_u32r * ((int32_t)calibrationData_.digH6)) >> 10) *
      (((v_x1_u32r * ((int32_t)calibrationData_.digH3)) >> 11) + ((int32_t)32768))) >> 10) +
      ((int32_t)2097152)) * ((int32_t)calibrationData_.digH2) + 8192) >> 14));

  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
    ((int32_t)calibrationData_.digH1)) >> 4));

  v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
  v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
  float h = (v_x1_u32r >> 12);
  return  h / 1024.0;
}

