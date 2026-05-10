/**********************************************************************************************************************
 * \file DrvUart.h
 * \brief ASCLIN0 UART 드라이버 (HC-12 무선 모듈 연결)
 *
 * TX : P15.2 (IfxAsclin0_TX_P15_2_OUT)  → X102 pin 31
 * RX : P15.3 (IfxAsclin0_RXB_P15_3_IN)  → X102 pin 32
 * BaudRate : 9600, 8-N-1 (HC-12 기본값)
 *********************************************************************************************************************/
#ifndef DRVUART_H
#define DRVUART_H

#include "Ifx_Types.h"

#define DRVUART_TX_PRIORITY     3
#define DRVUART_RX_PRIORITY     4
#define DRVUART_ERR_PRIORITY    2

void    DrvUart_Init(void);
void    DrvUart_SendByte(uint8 data);
void    DrvUart_SendString(const char *str);
boolean DrvUart_ReceiveByte(uint8 *data);

#endif /* DRVUART_H */
