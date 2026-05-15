/**********************************************************************************************************************
 * \file AppProtocol.c
 * \brief RC Vehicle UART 패킷 프로토콜 파서 구현
 *********************************************************************************************************************/
#include "AppProtocol.h"
#include "AppVehicle.h"
#include "DrvUart.h"
#include "DrvUart1.h"
#include "DrvGtmTimer.h"

/******************************************************************************/
/*                           Types                                            */
/******************************************************************************/
typedef enum
{
    PS_WAIT_STX = 0,
    PS_WAIT_LEN,
    PS_WAIT_CMD,
    PS_WAIT_PAYLOAD,
    PS_WAIT_CHK,
    PS_WAIT_ETX
} ParseState;

typedef struct
{
    ParseState state;
    uint8      len;
    uint8      cmd;
    uint8      payload[PROTO_MAX_PAYLOAD];
    uint8      payloadLen;
    uint8      payloadIdx;
    uint8      chkRecv;
} Parser;

/******************************************************************************/
/*                           Module Variables                                 */
/******************************************************************************/
static Parser s_parser[2];

volatile uint8 g_vehicleMode = VEHICLE_MODE_MANUAL;
uint32         g_lastMoveTime = 0u;

/******************************************************************************/
/*                           Static Functions                                 */
/******************************************************************************/
static void sendByte(uint8 data, uint8 ch)
{
    if (ch == 0u)
        DrvUart_SendByte(data);
    else
        DrvUart1_SendByte(data);
}

/* AA 02 80 <origCmd> <chk> 55 */
static void sendAck(uint8 origCmd, uint8 ch)
{
    uint8 chk = CMD_ACK ^ origCmd;
    sendByte(PROTO_STX,  ch);
    sendByte(0x02u,      ch);
    sendByte(CMD_ACK,    ch);
    sendByte(origCmd,    ch);
    sendByte(chk,        ch);
    sendByte(PROTO_ETX,  ch);
}

/* AA 02 E0 <errCode> <chk> 55 */
static void sendNack(uint8 errCode, uint8 ch)
{
    uint8 chk = CMD_NACK ^ errCode;
    sendByte(PROTO_STX,  ch);
    sendByte(0x02u,      ch);
    sendByte(CMD_NACK,   ch);
    sendByte(errCode,    ch);
    sendByte(chk,        ch);
    sendByte(PROTO_ETX,  ch);
}

static void dispatchPacket(const Parser *p, uint8 ch)
{
    uint8 chkCalc = p->cmd;
    uint8 i;

    for (i = 0u; i < p->payloadLen; i++)
        chkCalc ^= p->payload[i];

    if (chkCalc != p->chkRecv)
    {
        sendNack(NACK_ERR_CHK, ch);
        return;
    }

    switch (p->cmd)
    {
    case CMD_MOVE:
        if (p->payloadLen == 2u)
        {
            uint8 dir   = p->payload[0];
            uint8 speed = p->payload[1];
            if (dir   > DIR_RIGHT) dir   = DIR_STOP;
            if (speed > 100u)      speed = 100u;
            g_vehicleCmd   = dir;
            g_vehicleSpeed = (float32)speed;
            g_lastMoveTime = g_1ms_counter;
            sendAck(CMD_MOVE, ch);
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_MODE:
        if (p->payloadLen == 1u)
        {
            uint8 mode = p->payload[0];
            if (mode <= VEHICLE_MODE_AUTO)
                g_vehicleMode = mode;
            sendAck(CMD_MODE, ch);
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_PING:
        if (p->payloadLen == 0u)
        {
            sendAck(CMD_PING, ch);
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    default:
        sendNack(NACK_ERR_CMD, ch);
        break;
    }
}

/******************************************************************************/
/*                           Public API                                       */
/******************************************************************************/
void AppProtocol_Init(void)
{
    uint8 i;
    for (i = 0u; i < 2u; i++)
    {
        s_parser[i].state      = PS_WAIT_STX;
        s_parser[i].len        = 0u;
        s_parser[i].cmd        = 0u;
        s_parser[i].payloadLen = 0u;
        s_parser[i].payloadIdx = 0u;
        s_parser[i].chkRecv    = 0u;
    }
    g_vehicleMode  = VEHICLE_MODE_MANUAL;
    g_lastMoveTime = 0u;
}

void AppProtocol_Feed(uint8 byte, uint8 ch)
{
    Parser *p = &s_parser[ch];

    switch (p->state)
    {
    case PS_WAIT_STX:
        if (byte == PROTO_STX)
            p->state = PS_WAIT_LEN;
        break;

    case PS_WAIT_LEN:
        if (byte == 0u || byte > (PROTO_MAX_PAYLOAD + 1u))
        {
            p->state = PS_WAIT_STX;
            sendNack(NACK_ERR_LEN, ch);
        }
        else
        {
            p->len        = byte;
            p->payloadLen = byte - 1u;
            p->payloadIdx = 0u;
            p->state      = PS_WAIT_CMD;
        }
        break;

    case PS_WAIT_CMD:
        p->cmd   = byte;
        p->state = (p->payloadLen == 0u) ? PS_WAIT_CHK : PS_WAIT_PAYLOAD;
        break;

    case PS_WAIT_PAYLOAD:
        p->payload[p->payloadIdx++] = byte;
        if (p->payloadIdx >= p->payloadLen)
            p->state = PS_WAIT_CHK;
        break;

    case PS_WAIT_CHK:
        p->chkRecv = byte;
        p->state   = PS_WAIT_ETX;
        break;

    case PS_WAIT_ETX:
        if (byte == PROTO_ETX)
            dispatchPacket(p, ch);
        else
            sendNack(NACK_ERR_CHK, ch);
        p->state = PS_WAIT_STX;
        break;

    default:
        p->state = PS_WAIT_STX;
        break;
    }
}
