/**********************************************************************************************************************
 * \file DrvUart1.h
 * \brief ASCLIN1 UART 드라이버 (HC-10 BLE 모듈 연결)
 *
 * TX : P20.10 (IfxAsclin1_TX_P20_10_OUT)  -> X102 pin 38
 * RX : P20.9  (IfxAsclin1_RXC_P20_9_IN)   -> X102 pin 37
 * BaudRate : 9600, 8-N-1 (HC-10 기본값)
 *********************************************************************************************************************/
#ifndef DRVUART1_H
#define DRVUART1_H

#include "Ifx_Types.h"

#define DRVUART1_TX_PRIORITY     9
#define DRVUART1_RX_PRIORITY     10
#define DRVUART1_ERR_PRIORITY    11

void    DrvUart1_Init(void);
void    DrvUart1_SendByte(uint8 data);
void    DrvUart1_SendString(const char *str);
boolean DrvUart1_ReceiveByte(uint8 *data);

#endif /* DRVUART1_H */
