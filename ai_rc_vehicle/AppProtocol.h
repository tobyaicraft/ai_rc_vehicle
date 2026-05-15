/**********************************************************************************************************************
 * \file AppProtocol.h
 * \brief RC Vehicle UART 패킷 프로토콜 파서
 *
 * 패킷 구조:
 *   STX(0xAA) | LEN(1B) | CMD(1B) | PAYLOAD(N B) | CHK(1B) | ETX(0x55)
 *   LEN = CMD(1) + PAYLOAD(N) 바이트 수
 *   CHK = CMD XOR PAYLOAD[0] XOR ... XOR PAYLOAD[N-1]
 *
 * 명령:
 *   0x01 MOVE  : dir(1B) + speed(1B)   → 방향·속도 제어
 *   0x02 MODE  : mode(1B)              → 동작 모드 전환
 *   0x10 PING  : (없음)                → 연결 확인
 *   0x80 ACK   : origCmd(1B)           → 수신 확인 응답 (송신 전용)
 *   0xE0 NACK  : errCode(1B)           → 수신 실패 응답 (송신 전용)
 *********************************************************************************************************************/
#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include "Ifx_Types.h"

/* Packet framing */
#define PROTO_STX           0xAAu
#define PROTO_ETX           0x55u
#define PROTO_MAX_PAYLOAD   8u

/* Command codes */
#define CMD_MOVE    0x01u
#define CMD_MODE    0x02u
#define CMD_PING    0x10u
#define CMD_ACK     0x80u
#define CMD_NACK    0xE0u

/* MOVE - direction (payload[0]) — matches VehicleCommand enum values */
#define DIR_STOP    0u
#define DIR_FORWARD 1u
#define DIR_REVERSE 2u
#define DIR_LEFT    3u
#define DIR_RIGHT   4u

/* MODE - mode (payload[0]) */
#define VEHICLE_MODE_MANUAL  0u
#define VEHICLE_MODE_CALIB   1u
#define VEHICLE_MODE_AUTO    2u

/* NACK error codes */
#define NACK_ERR_CHK    0x01u
#define NACK_ERR_LEN    0x02u
#define NACK_ERR_CMD    0x03u

/* Last MOVE command timestamp (ms) — used by AppTask for timeout */
extern uint32 g_lastMoveTime;

/* Current vehicle mode */
extern volatile uint8 g_vehicleMode;

void AppProtocol_Init(void);
void AppProtocol_Feed(uint8 byte, uint8 ch);   /* ch: 0=HC-12, 1=HC-10 BLE */

#endif /* APP_PROTOCOL_H */
