#pragma once

#include <stdint.h>

typedef struct ThreeDimData
{
    int16_t X;  
    int16_t Y;
    int16_t Z;
} ThreeDimData;

typedef struct IMUData
{
    ThreeDimData Mag;
    ThreeDimData Accel;
    ThreeDimData Gyro;
    uint8_t MagStatus;
    uint8_t AccelStatus;
    uint8_t GyroStatus;
    uint8_t ErrorStatus;
} IMUData;

typedef void (*IMU_CALLBACK)(const IMUData*);

enum IMU_ERROR_STATUS
{
    IMU_OK,
    IMU_ERROR
};

enum IMU_ERROR_STATUS InitIMU(IMU_CALLBACK Callback);
enum IMU_ERROR_STATUS StartIMU();
enum IMU_ERROR_STATUS StopIMU();