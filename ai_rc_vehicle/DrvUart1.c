/**********************************************************************************************************************
 * \file DrvUart1.c
 * \brief ASCLIN1 UART 드라이버 (HC-10 BLE 모듈)
 *
 * TX : P20.10 (X102 pin 38)  RX : P20.9 (X102 pin 37)
 * SW FIFO : TX 256 byte / RX 64 byte
 * ISR 우선순위 : TX=9, RX=10, ERR=11
 *********************************************************************************************************************/
#include "DrvUart1.h"
#include "Compilers.h"
#include "Asclin/Asc/IfxAsclin_Asc.h"
#include "_PinMap/IfxAsclin_PinMap.h"

/******************************************************************************/
/*                           Configuration                                    */
/******************************************************************************/
#define UART1_TX_BUFFER_SIZE     256u
#define UART1_RX_BUFFER_SIZE     64u

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static IfxAsclin_Asc s_asc1;

static uint8 s_txBuffer1[UART1_TX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8u];
static uint8 s_rxBuffer1[UART1_RX_BUFFER_SIZE + sizeof(Ifx_Fifo) + 8u];

/******************************************************************************/
/*                           ISR                                              */
/******************************************************************************/
IFX_INTERRUPT(DrvUart1_TxISR, 0, DRVUART1_TX_PRIORITY)
{
    IfxAsclin_Asc_isrTransmit(&s_asc1);
}

IFX_INTERRUPT(DrvUart1_RxISR, 0, DRVUART1_RX_PRIORITY)
{
    IfxAsclin_Asc_isrReceive(&s_asc1);
}

IFX_INTERRUPT(DrvUart1_ErrISR, 0, DRVUART1_ERR_PRIORITY)
{
    IfxAsclin_Asc_isrError(&s_asc1);
}

/******************************************************************************/
/*                           Functions                                        */
/******************************************************************************/
void DrvUart1_Init(void)
{
    static const IfxAsclin_Asc_Pins pins = {
        NULL,                            IfxPort_InputMode_pullUp,              /* CTS 미사용 */
        &IfxAsclin1_RXC_P20_9_IN,      IfxPort_InputMode_pullUp,              /* RX: P20.9 */
        NULL,                            IfxPort_OutputMode_pushPull,           /* RTS 미사용 */
        &IfxAsclin1_TX_P20_10_OUT,     IfxPort_OutputMode_pushPull,           /* TX: P20.10 */
        IfxPort_PadDriver_cmosAutomotiveSpeed1
    };

    IfxAsclin_Asc_Config cfg;
    IfxAsclin_Asc_initModuleConfig(&cfg, &MODULE_ASCLIN1);

    cfg.baudrate.baudrate    = 9600.0f;
    cfg.baudrate.prescaler   = 1u;

    cfg.frame.dataLength     = IfxAsclin_DataLength_8;
    cfg.frame.parityBit      = FALSE;
    cfg.frame.stopBit        = IfxAsclin_StopBit_1;

    cfg.interrupt.txPriority = DRVUART1_TX_PRIORITY;
    cfg.interrupt.rxPriority = DRVUART1_RX_PRIORITY;
    cfg.interrupt.erPriority = DRVUART1_ERR_PRIORITY;
    cfg.interrupt.typeOfService = IfxSrc_Tos_cpu0;

    cfg.pins         = &pins;
    cfg.txBuffer     = s_txBuffer1;
    cfg.txBufferSize = UART1_TX_BUFFER_SIZE;
    cfg.rxBuffer     = s_rxBuffer1;
    cfg.rxBufferSize = UART1_RX_BUFFER_SIZE;

    IfxAsclin_Asc_initModule(&s_asc1, &cfg);
}

void DrvUart1_SendByte(uint8 data)
{
    IfxAsclin_Asc_blockingWrite(&s_asc1, data);
}

void DrvUart1_SendString(const char *str)
{
    while (*str != '\0')
    {
        IfxAsclin_Asc_blockingWrite(&s_asc1, (uint8)*str);
        str++;
    }
}

boolean DrvUart1_ReceiveByte(uint8 *data)
{
    Ifx_SizeT count = 1;
    return IfxAsclin_Asc_read(&s_asc1, data, &count, TIME_NULL);
}
