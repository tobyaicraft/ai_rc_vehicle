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
 *   0x20 CAL_BAT : multiplier_hi(1B) + multiplier_lo(1B) → 배터리 보정 계수 설정 (RAM)
 *   0x21 CAL_SAVE: (없음)              → RAM → DFLASH 저장
 *   0x22 CAL_LOAD: (없음)              → DFLASH → RAM 로드
 *   0x23 CAL_DUTY: FL(1B) + FR(1B) + RL(1B) + RR(1B) → 모터 Duty 설정 (RAM)
 *   0x24 CAL_TURN: front(1B) + rear(1B) → 회전 팩터 설정 (RAM)
 *   0x25 CAL_QUERY: (없음) → 현재 캘리브레이션 값 텍스트로 응답
 *   0x25 CAL_TEST: motor_id(1B) + duty(1B) → 개별 모터 테스트 구동
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
#define CMD_CAL_BAT  0x20u
#define CMD_CAL_SAVE 0x21u
#define CMD_CAL_LOAD 0x22u
#define CMD_CAL_DUTY 0x23u
#define CMD_CAL_TURN  0x24u
#define CMD_CAL_QUERY 0x25u
#define CMD_CAL_ERASE 0x26u
#define CMD_RESET   0x30u
#define CMD_ACK     0x80u
#define CMD_NACK    0xE0u

/* MOVE - direction (payload[0]) — matches VehicleCommand enum values */
#define DIR_STOP      0u
#define DIR_FORWARD   1u
#define DIR_REVERSE   2u
#define DIR_LEFT      3u
#define DIR_RIGHT     4u
#define DIR_TURN90_L  5u
#define DIR_TURN90_R  6u
#define DIR_YAW_ZERO  7u

/* MODE - mode (payload[0]) */
#define VEHICLE_MODE_MANUAL  0u
#define VEHICLE_MODE_CALIB   1u
#define VEHICLE_MODE_AUTO    2u
#define VEHICLE_MODE_TEST    3u

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
