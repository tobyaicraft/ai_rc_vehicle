/**********************************************************************************************************************
 * \file DrvMpu9250.c
 * \brief MPU-9250 9-axis IMU sensor driver
 *
 * - QSPI3 (DrvSpi) SPI communication
 * - Gyro: +/-500 deg/s, Accel: +/-2g, DLPF: 41Hz, Sample rate: 100Hz
 * - Complementary Filter (alpha=0.96) for Roll/Pitch/Yaw
 * - UART output: "R:+012.3,P:-005.7,Y:+045.2\r\n"
 *********************************************************************************************************************/
#include "DrvMpu9250.h"
#include "DrvSpi.h"
#include "DrvUart.h"
#include <math.h>

/******************************************************************************/
/*                           MPU-9250 Registers                               */
/******************************************************************************/
#define MPU_REG_SMPLRT_DIV      0x19u
#define MPU_REG_CONFIG          0x1Au
#define MPU_REG_GYRO_CONFIG     0x1Bu
#define MPU_REG_ACCEL_CONFIG    0x1Cu
#define MPU_REG_ACCEL_CONFIG2   0x1Du
#define MPU_REG_ACCEL_XOUT_H   0x3Bu
#define MPU_REG_USER_CTRL       0x6Au
#define MPU_REG_PWR_MGMT_1     0x6Bu
#define MPU_REG_PWR_MGMT_2     0x6Cu
#define MPU_REG_WHO_AM_I       0x75u

#define MPU_WHO_AM_I_MPU9250   0x71u
#define MPU_WHO_AM_I_MPU6500   0x70u

/* Sensitivity */
#define ACCEL_SENSITIVITY      16384.0f  /* +/-2g default */
#define GYRO_SENSITIVITY       65.5f     /* +/-500 deg/s */

/* Complementary Filter */
#define FILTER_ALPHA           0.96f
#define DT                     0.01f     /* 10ms (100Hz) */

/* Yaw drift suppression */
#define YAW_DEADZONE           1.5f
#define YAW_DECAY              0.998f

#define RAD_TO_DEG             57.2957795f

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static boolean s_initialized = FALSE;

static sint16  s_accelRaw[3];
static sint16  s_gyroRaw[3];

static float32 s_roll  = 0.0f;
static float32 s_pitch = 0.0f;
static float32 s_yaw   = 0.0f;

/* Calibration offsets */
static float32 s_rollOffset  = 0.0f;
static float32 s_pitchOffset = 0.0f;
static float32 s_gyroOffsetRoll  = 0.0f;
static float32 s_gyroOffsetPitch = 0.0f;
static float32 s_gyroOffsetYaw   = 0.0f;
static boolean s_calibrated  = FALSE;
static uint16  s_calibCount  = 0u;

/******************************************************************************/
/*                           Static Functions                                 */
/******************************************************************************/
static void delay_ms(uint32 ms)
{
    volatile uint32 count;
    uint32 i;
    for (i = 0u; i < ms; i++)
    {
        for (count = 0u; count < 20000u; count++) {}
    }
}

static boolean writeRegVerify(uint8 regAddr, uint8 data)
{
    uint8 retry;
    for (retry = 0u; retry < 5u; retry++)
    {
        DrvSpi_WriteReg(regAddr, data);
        delay_ms(5);
        if (DrvSpi_ReadReg(regAddr) == data) return TRUE;
    }
    return FALSE;
}

static void parseSensorData(const uint8 *buf)
{
    s_accelRaw[0] = (sint16)((uint16)buf[0]  << 8u) | buf[1];
    s_accelRaw[1] = (sint16)((uint16)buf[2]  << 8u) | buf[3];
    s_accelRaw[2] = (sint16)((uint16)buf[4]  << 8u) | buf[5];
    s_gyroRaw[0]  = (sint16)((uint16)buf[8]  << 8u) | buf[9];
    s_gyroRaw[1]  = (sint16)((uint16)buf[10] << 8u) | buf[11];
    s_gyroRaw[2]  = (sint16)((uint16)buf[12] << 8u) | buf[13];
}

static void updateAttitude(void)
{
    float32 ax = (float32)s_accelRaw[0] / ACCEL_SENSITIVITY;
    float32 ay = (float32)s_accelRaw[1] / ACCEL_SENSITIVITY;
    float32 az = (float32)s_accelRaw[2] / ACCEL_SENSITIVITY;

    float32 rollRate  = -(float32)s_gyroRaw[1] / GYRO_SENSITIVITY;
    float32 pitchRate = -(float32)s_gyroRaw[0] / GYRO_SENSITIVITY;
    float32 yawRate   =  (float32)s_gyroRaw[2] / GYRO_SENSITIVITY;

    /* Calibration: collect 100 samples (1 sec) at startup */
    if (!s_calibrated)
    {
        float32 rollAcc  = atan2f(-ax, az) * RAD_TO_DEG;
        float32 pitchAcc = atan2f(ay, sqrtf(ax * ax + az * az)) * RAD_TO_DEG;

        s_rollOffset      += rollAcc;
        s_pitchOffset     += pitchAcc;
        s_gyroOffsetRoll  += rollRate;
        s_gyroOffsetPitch += pitchRate;
        s_gyroOffsetYaw   += yawRate;
        s_calibCount++;

        if (s_calibCount >= 100u)
        {
            s_rollOffset      /= 100.0f;
            s_pitchOffset     /= 100.0f;
            s_gyroOffsetRoll  /= 100.0f;
            s_gyroOffsetPitch /= 100.0f;
            s_gyroOffsetYaw   /= 100.0f;
            s_calibrated = TRUE;
            s_roll = 0.0f; s_pitch = 0.0f; s_yaw = 0.0f;
        }
        return;
    }

    /* Gyro bias correction */
    rollRate  -= s_gyroOffsetRoll;
    pitchRate -= s_gyroOffsetPitch;
    yawRate   -= s_gyroOffsetYaw;

    /* Accel-based angle (with offset correction) */
    float32 rollAcc  = atan2f(-ax, az) * RAD_TO_DEG - s_rollOffset;
    float32 pitchAcc = atan2f(ay, sqrtf(ax * ax + az * az)) * RAD_TO_DEG - s_pitchOffset;

    /* Complementary Filter: Roll/Pitch */
    s_roll  = FILTER_ALPHA * (s_roll  + rollRate  * DT) + (1.0f - FILTER_ALPHA) * rollAcc;
    s_pitch = FILTER_ALPHA * (s_pitch + pitchRate * DT) + (1.0f - FILTER_ALPHA) * pitchAcc;

    /* Yaw: deadzone only (no decay) */
    if (yawRate > YAW_DEADZONE || yawRate < -YAW_DEADZONE)
    {
        s_yaw += yawRate * DT;
    }
}

static void floatToStr(float32 val, char *buf)
{
    sint32 intPart;
    uint32 absInt, fracPart;

    buf[0] = (val < 0.0f) ? '-' : '+';
    if (val < 0.0f) val = -val;

    intPart  = (sint32)val;
    fracPart = (uint32)((val - (float32)intPart) * 10.0f + 0.5f);
    if (fracPart >= 10u) { fracPart = 0u; intPart++; }

    absInt = (uint32)intPart;
    if (absInt > 999u) absInt = 999u;

    buf[1] = (char)('0' + (absInt / 100u));
    buf[2] = (char)('0' + ((absInt / 10u) % 10u));
    buf[3] = (char)('0' + (absInt % 10u));
    buf[4] = '.';
    buf[5] = (char)('0' + fracPart);
    buf[6] = '\0';
}

/******************************************************************************/
/*                           Public Functions                                 */
/******************************************************************************/
void DrvMpu9250_Init(void)
{
    DrvSpi_WriteReg(MPU_REG_PWR_MGMT_1, 0x80u);
    delay_ms(200);

    writeRegVerify(MPU_REG_PWR_MGMT_1, 0x01u);
    delay_ms(50);

    writeRegVerify(MPU_REG_USER_CTRL, 0x10u);
    delay_ms(10);

    writeRegVerify(MPU_REG_PWR_MGMT_2, 0x00u);
    delay_ms(10);

    writeRegVerify(MPU_REG_GYRO_CONFIG, 0x08u);
    writeRegVerify(MPU_REG_ACCEL_CONFIG, 0x08u);
    writeRegVerify(MPU_REG_CONFIG, 0x03u);
    writeRegVerify(MPU_REG_ACCEL_CONFIG2, 0x03u);
    writeRegVerify(MPU_REG_SMPLRT_DIV, 0x09u);

    delay_ms(50);

    s_initialized = DrvMpu9250_IsReady();
    if (s_initialized)
    {
        DrvUart_SendString("MPU-9250 OK!\r\n");
    }
    else
    {
        DrvUart_SendString("MPU-9250 FAIL!\r\n");
    }
}

boolean DrvMpu9250_IsReady(void)
{
    uint8 id = DrvSpi_ReadReg(MPU_REG_WHO_AM_I);
    return (id == MPU_WHO_AM_I_MPU9250 || id == MPU_WHO_AM_I_MPU6500) ? TRUE : FALSE;
}

void DrvMpu9250_ReadSensors(void)
{
    uint8 buf[14];

    if (!s_initialized) return;

    /* Read 2 bytes at a time (QSPI3 FIFO limitation) */
    DrvSpi_ReadBurst(0x3Bu, &buf[0],  2u);
    DrvSpi_ReadBurst(0x3Du, &buf[2],  2u);
    DrvSpi_ReadBurst(0x3Fu, &buf[4],  2u);
    buf[6] = 0u; buf[7] = 0u;
    DrvSpi_ReadBurst(0x43u, &buf[8],  2u);
    DrvSpi_ReadBurst(0x45u, &buf[10], 2u);
    DrvSpi_ReadBurst(0x47u, &buf[12], 2u);

    parseSensorData(buf);
    updateAttitude();
}

float32 DrvMpu9250_GetRoll(void)  { return s_roll;  }
float32 DrvMpu9250_GetPitch(void) { return s_pitch; }
float32 DrvMpu9250_GetYaw(void)   { return s_yaw;   }

void DrvMpu9250_SendUart(void)
{
    char str[7];

    if (!s_initialized) return;

    DrvUart_SendString("R:");
    floatToStr(s_roll, str);
    DrvUart_SendString(str);

    DrvUart_SendString(",P:");
    floatToStr(s_pitch, str);
    DrvUart_SendString(str);

    DrvUart_SendString(",Y:");
    floatToStr(s_yaw, str);
    DrvUart_SendString(str);

    DrvUart_SendString("\r\n");
}
