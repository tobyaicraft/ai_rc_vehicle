/**********************************************************************************************************************
 * \file DrvMpu9250.h
 * \brief MPU-9250 9-axis IMU sensor driver
 *
 * - SPI communication via QSPI3 (DrvSpi)
 * - Complementary Filter for Roll/Pitch/Yaw
 *********************************************************************************************************************/
#ifndef DRVMPU9250_H
#define DRVMPU9250_H

#include "Ifx_Types.h"

void    DrvMpu9250_Init(void);
boolean DrvMpu9250_IsReady(void);
void    DrvMpu9250_ReadSensors(void);
float32 DrvMpu9250_GetRoll(void);
float32 DrvMpu9250_GetPitch(void);
float32 DrvMpu9250_GetYaw(void);
void    DrvMpu9250_SendUart(void);

#endif /* DRVMPU9250_H */
