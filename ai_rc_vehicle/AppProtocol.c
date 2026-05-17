/**********************************************************************************************************************
 * \file AppProtocol.c
 * \brief RC Vehicle UART 패킷 프로토콜 파서 구현
 *********************************************************************************************************************/
#include "AppProtocol.h"
#include "AppVehicle.h"
#include "DrvUart.h"
#include "DrvUart1.h"
#include "DrvGtmTimer.h"
#include "DrvFlash.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"

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
            if (dir   > DIR_YAW_ZERO) dir   = DIR_STOP;
            if (speed > 100u)      speed = 100u;
            g_vehicleSpeed = (float32)speed;
            /* TEST/AUTO 모드에서는 방향을 태스크가 제어하므로 dir 무시 */
            if (g_vehicleMode == VEHICLE_MODE_MANUAL ||
                g_vehicleMode == VEHICLE_MODE_CALIB)
            {
                g_vehicleCmd = dir;
            }
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
            if (mode <= VEHICLE_MODE_TEST)
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

    case CMD_CAL_BAT:
        if (p->payloadLen == 2u)
        {
            /* payload[0]=hi, payload[1]=lo → uint16 multiplier (x1000) */
            uint16 mul = ((uint16)p->payload[0] << 8) | (uint16)p->payload[1];
            if ((mul >= 1000u) && (mul <= 4000u))
            {
                g_batMultiplier = mul;
                sendAck(CMD_CAL_BAT, ch);
            }
            else
            {
                sendNack(NACK_ERR_CMD, ch);
            }
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_CAL_SAVE:
        if (p->payloadLen == 0u)
        {
            if (DrvFlash_SaveCalibration())
                sendAck(CMD_CAL_SAVE, ch);
            else
                sendNack(NACK_ERR_CMD, ch);
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_CAL_LOAD:
        if (p->payloadLen == 0u)
        {
            if (DrvFlash_LoadCalibration())
                sendAck(CMD_CAL_LOAD, ch);
            else
                sendNack(NACK_ERR_CMD, ch);
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_CAL_DUTY:
        if (p->payloadLen == 4u)
        {
            /* payload: FL, FR, RL, RR (0~100%) */
            uint8 fl = p->payload[0];
            uint8 fr = p->payload[1];
            uint8 rl = p->payload[2];
            uint8 rr = p->payload[3];
            if ((fl <= 100u) && (fr <= 100u) && (rl <= 100u) && (rr <= 100u))
            {
                g_calDutyFL = fl;
                g_calDutyFR = fr;
                g_calDutyRL = rl;
                g_calDutyRR = rr;
                sendAck(CMD_CAL_DUTY, ch);
            }
            else
            {
                sendNack(NACK_ERR_CMD, ch);
            }
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_CAL_TURN:
        if (p->payloadLen == 2u)
        {
            /* payload: front, rear (0~100%) */
            uint8 front = p->payload[0];
            uint8 rear  = p->payload[1];
            if ((front <= 100u) && (rear <= 100u))
            {
                g_calTurnFront = front;
                g_calTurnRear  = rear;
                sendAck(CMD_CAL_TURN, ch);
            }
            else
            {
                sendNack(NACK_ERR_CMD, ch);
            }
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_CAL_QUERY:
        if (p->payloadLen == 0u)
        {
            /* 현재 캘리브레이션 값을 텍스트로 전송 */
            char buf[64];
            char *ptr = buf;
            /* "CAL:FL,FR,RL,RR,FT,RT,BATMUL\r\n" */
            static const char hex[] = "0123456789";
            #define PUT_U8(v) do { \
                uint8 _v = (v); \
                if (_v >= 100u) *ptr++ = hex[_v / 100u]; \
                if (_v >= 10u)  *ptr++ = hex[(_v / 10u) % 10u]; \
                *ptr++ = hex[_v % 10u]; \
            } while(0)
            #define PUT_U16(v) do { \
                uint16 _v = (v); \
                if (_v >= 10000u) *ptr++ = hex[_v / 10000u]; \
                if (_v >= 1000u)  *ptr++ = hex[(_v / 1000u) % 10u]; \
                if (_v >= 100u)   *ptr++ = hex[(_v / 100u) % 10u]; \
                if (_v >= 10u)    *ptr++ = hex[(_v / 10u) % 10u]; \
                *ptr++ = hex[_v % 10u]; \
            } while(0)

            *ptr++ = 'C'; *ptr++ = 'A'; *ptr++ = 'L'; *ptr++ = ':';
            PUT_U8(g_calDutyFL); *ptr++ = ',';
            PUT_U8(g_calDutyFR); *ptr++ = ',';
            PUT_U8(g_calDutyRL); *ptr++ = ',';
            PUT_U8(g_calDutyRR); *ptr++ = ',';
            PUT_U8(g_calTurnFront); *ptr++ = ',';
            PUT_U8(g_calTurnRear);  *ptr++ = ',';
            PUT_U16(g_batMultiplier);
            *ptr++ = '\r'; *ptr++ = '\n'; *ptr = '\0';

            if (ch == 0u)
                DrvUart_SendString(buf);
            else
                DrvUart1_SendString(buf);

            sendAck(CMD_CAL_QUERY, ch);

            #undef PUT_U8
            #undef PUT_U16
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_CAL_ERASE:
        if (p->payloadLen == 0u)
        {
            if (DrvFlash_EraseSector())
                sendAck(CMD_CAL_ERASE, ch);
            else
                sendNack(NACK_ERR_CMD, ch);
        }
        else
        {
            sendNack(NACK_ERR_LEN, ch);
        }
        break;

    case CMD_RESET:
        if (p->payloadLen == 0u)
        {
            sendAck(CMD_RESET, ch);
            /* ACK 전송 완료 대기 후 소프트웨어 리셋 */
            {
                volatile uint32 wait;
                for (wait = 0u; wait < 100000u; wait++) {}
            }
            /* Safety Endinit 해제 후 SW Reset 요청 */
            {
                uint16 pwd = IfxScuWdt_getSafetyWatchdogPassword();
                IfxScuWdt_clearSafetyEndinit(pwd);
                MODULE_SCU.SWRSTCON.B.SWRSTREQ = 1;
                while (1) {}
            }
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
