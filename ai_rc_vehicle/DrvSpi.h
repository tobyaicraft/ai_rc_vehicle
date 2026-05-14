/**********************************************************************************************************************
 * \file DrvSpi.h
 * \brief QSPI3 SPI Master driver (MPU-9250)
 *
 * Pins:
 *   SCLK : P22.3 (X102 pin 30)
 *   MOSI : P22.0 (X102 pin 27)
 *   MISO : P22.1 (X102 pin 28)
 *   CS   : P22.2 (X102 pin 29, SLSO12)
 *
 * ISR priorities: TX=6, RX=7, ERR=8
 *********************************************************************************************************************/
#ifndef DRVSPI_H
#define DRVSPI_H

#include "Ifx_Types.h"

/* ISR priorities */
#define DRVSPI_TX_PRIORITY   6
#define DRVSPI_RX_PRIORITY   7
#define DRVSPI_ERR_PRIORITY  8

void  DrvSpi_Init(void);
uint8 DrvSpi_ReadReg(uint8 regAddr);
void  DrvSpi_WriteReg(uint8 regAddr, uint8 data);
void  DrvSpi_ReadBurst(uint8 regAddr, uint8 *buf, uint16 len);

#endif /* DRVSPI_H */
