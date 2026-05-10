# MPU-9250 레지스터 맵 및 드라이버 개발 레퍼런스

## 1. 디바이스 개요

- **칩**: MPU-9250 (InvenSense), 9축 IMU (3축 자이로 + 3축 가속도계 + AK8963 3축 자력계)
- **패키지**: 3x3x1mm QFN 24핀
- **VDD**: 2.4V ~ 3.6V, **VDDIO**: 1.71V ~ VDD
- **WHO_AM_I**: 0x75 레지스터, 응답값 = **0x71**
- **자력계 (AK8963) Device ID**: 0x00 레지스터, 응답값 = **0x48**

## 2. 통신 인터페이스

### I2C
- 최대 400 kHz (Fast Mode)
- 슬레이브 주소: AD0=Low → **0x68**, AD0=High → **0x69**
- AK8963 (자력계) 직접 접근 시: **0x0C** (Bypass 모드)

### SPI
- 레지스터 R/W: 최대 **1 MHz**
- 센서/인터럽트 레지스터 읽기: 최대 **20 MHz**
- MSB first, CPOL=0/CPHA=0
- 첫 바이트: `[R/W(1) | RegAddr(7)]` (Read=1, Write=0)
- SPI 모드 진입: USER_CTRL(0x6A)의 I2C_IF_DIS(bit4) = 1

## 3. 핵심 레지스터 맵 (자이로/가속도계)

### 설정 레지스터

| 주소 | 이름 | R/W | 핵심 비트 | 설명 |
|------|------|-----|----------|------|
| 0x19 (25) | SMPLRT_DIV | R/W | [7:0] | 샘플레이트 분주기. Rate = 1kHz / (1 + DIV) |
| 0x1A (26) | CONFIG | R/W | [6] FIFO_MODE, [5:3] EXT_SYNC_SET, [2:0] DLPF_CFG | 자이로 DLPF 설정 |
| 0x1B (27) | GYRO_CONFIG | R/W | [4:3] GYRO_FS_SEL, [1:0] FCHOICE_B | 자이로 풀스케일 및 필터 |
| 0x1C (28) | ACCEL_CONFIG | R/W | [4:3] ACCEL_FS_SEL | 가속도계 풀스케일 |
| 0x1D (29) | ACCEL_CONFIG2 | R/W | [3] accel_fchoice_b, [2:0] A_DLPFCFG | 가속도계 DLPF 설정 |
| 0x1F (31) | WOM_THR | R/W | [7:0] | Wake-on-Motion 임계값 (LSB=4mg) |
| 0x23 (35) | FIFO_EN | R/W | [7:0] | FIFO 데이터 선택 |
| 0x37 (55) | INT_PIN_CFG | R/W | [7] ACTL, [5] LATCH_INT_EN, [1] BYPASS_EN | 인터럽트 핀 / 바이패스 설정 |
| 0x38 (56) | INT_ENABLE | R/W | [6] WOM_EN, [4] FIFO_OFLOW_EN, [0] RAW_RDY_EN | 인터럽트 활성화 |
| 0x6A (106) | USER_CTRL | R/W | [6] FIFO_EN, [5] I2C_MST_EN, [4] I2C_IF_DIS, [2] FIFO_RST, [0] SIG_COND_RST | 사용자 제어 |
| 0x6B (107) | PWR_MGMT_1 | R/W | [7] H_RESET, [6] SLEEP, [5] CYCLE, [2:0] CLKSEL | 전원관리1 |
| 0x6C (108) | PWR_MGMT_2 | R/W | [5:3] DIS_XA/YA/ZA, [2:0] DIS_XG/YG/ZG | 전원관리2 (축별 활성화) |

### 센서 데이터 레지스터 (Read-Only)

| 주소 | 이름 | 설명 |
|------|------|------|
| 0x3B-0x3C | ACCEL_XOUT_H/L | 가속도 X축 (16-bit, 2의 보수) |
| 0x3D-0x3E | ACCEL_YOUT_H/L | 가속도 Y축 |
| 0x3F-0x40 | ACCEL_ZOUT_H/L | 가속도 Z축 |
| 0x41-0x42 | TEMP_OUT_H/L | 온도 센서 |
| 0x43-0x44 | GYRO_XOUT_H/L | 자이로 X축 (16-bit, 2의 보수) |
| 0x45-0x46 | GYRO_YOUT_H/L | 자이로 Y축 |
| 0x47-0x48 | GYRO_ZOUT_H/L | 자이로 Z축 |
| 0x49-0x60 | EXT_SENS_DATA_00~23 | 외부 센서 (자력계 등) 데이터 |
| 0x3A (58) | INT_STATUS | [6] WOM_INT, [4] FIFO_OFLOW, [0] RAW_DATA_RDY |
| 0x75 (117) | WHO_AM_I | 디바이스 ID (0x71) |

### 인터럽트 상태 (0x3A)

| 비트 | 이름 | 설명 |
|------|------|------|
| [6] | WOM_INT | Wake-on-Motion 발생 |
| [4] | FIFO_OVERFLOW_INT | FIFO 오버플로 |
| [0] | RAW_DATA_RDY_INT | 센서 데이터 준비 완료 |

## 4. 자이로 풀스케일 설정 (GYRO_CONFIG 0x1B)

| GYRO_FS_SEL [4:3] | 범위 | 감도 (LSB/(°/s)) |
|:--:|:--:|:--:|
| 00 | ±250 °/s | 131 |
| 01 | ±500 °/s | 65.5 |
| 10 | ±1000 °/s | 32.8 |
| 11 | ±2000 °/s | 16.4 |

## 5. 가속도계 풀스케일 설정 (ACCEL_CONFIG 0x1C)

| ACCEL_FS_SEL [4:3] | 범위 | 감도 (LSB/g) |
|:--:|:--:|:--:|
| 00 | ±2g | 16384 |
| 01 | ±4g | 8192 |
| 10 | ±8g | 4096 |
| 11 | ±16g | 2048 |

## 6. DLPF 설정 (자이로, CONFIG 0x1A)

FCHOICE_B = 00 일 때 DLPF_CFG 유효:

| DLPF_CFG | 대역폭 (Hz) | 지연 (ms) | Fs (kHz) |
|:--:|:--:|:--:|:--:|
| 0 | 250 | 0.97 | 8 |
| 1 | 184 | 2.9 | 1 |
| 2 | 92 | 3.9 | 1 |
| 3 | 41 | 5.9 | 1 |
| 4 | 20 | 9.9 | 1 |
| 5 | 10 | 17.85 | 1 |
| 6 | 5 | 33.48 | 1 |

## 7. 가속도계 DLPF 설정 (ACCEL_CONFIG2 0x1D)

accel_fchoice_b = 0 일 때 A_DLPFCFG 유효:

| A_DLPFCFG | 대역폭 (Hz) | 지연 (ms) | Rate (kHz) |
|:--:|:--:|:--:|:--:|
| 0 | 460 | 1.94 | 1 |
| 1 | 184 | 5.80 | 1 |
| 2 | 92 | 7.80 | 1 |
| 3 | 41 | 11.80 | 1 |
| 4 | 20 | 19.80 | 1 |
| 5 | 10 | 35.70 | 1 |
| 6 | 5 | 66.96 | 1 |

## 8. 클럭 소스 선택 (PWR_MGMT_1 0x6B, CLKSEL[2:0])

| CLKSEL | 클럭 소스 |
|:--:|------|
| 0 | 내부 20MHz 오실레이터 |
| 1~5 | Auto-select (PLL ready → PLL, 아니면 내부) |
| 6 | 내부 20MHz 오실레이터 |
| 7 | 클럭 정지 |

**권장**: CLKSEL = 1 (자이로 PLL 자동 선택, ±1% 정밀도)

## 9. 온도 센서 변환

```
TEMP_degC = ((TEMP_OUT - RoomTemp_Offset) / Temp_Sensitivity) + 21°C
```

## 10. 자력계 (AK8963) 레지스터 맵

AK8963은 MPU-9250 내부에 I2C 슬레이브로 연결 (주소: 0x0C).

### 접근 방법
1. **I2C Bypass 모드**: INT_PIN_CFG(0x37)의 BYPASS_EN=1 → 호스트에서 직접 0x0C로 접근
2. **I2C Master 모드**: MPU-9250 내부 I2C Master로 자동 읽기 (SPI 사용 시 필수)

### 주요 레지스터

| 주소 | 이름 | R/W | 설명 |
|------|------|-----|------|
| 0x00 | WIA | R | 디바이스 ID = **0x48** |
| 0x02 | ST1 | R | [0] DRDY: 데이터 준비 |
| 0x03-0x08 | HXL~HZH | R | 자력 측정값 (X/Y/Z, 16bit Little Endian) |
| 0x09 | ST2 | R | [3] HOFL: 오버플로, [4] BIT: 출력 비트 미러. **반드시 읽어야 측정 완료** |
| 0x0A | CNTL1 | R/W | [4] BIT: 0=14bit/1=16bit, [3:0] MODE |
| 0x0B | CNTL2 | R/W | [0] SRST: 소프트 리셋 |
| 0x10-0x12 | ASAX/Y/Z | R | 감도 보정값 (Fuse ROM, Fuse access mode에서만 읽기) |

### 동작 모드 (CNTL1 MODE[3:0])

| 코드 | 모드 | 설명 |
|:--:|------|------|
| 0000 | Power-down | 기본값 |
| 0001 | Single measurement | 1회 측정 후 자동 Power-down |
| 0010 | Continuous mode 1 | 8Hz 연속 측정 |
| 0110 | Continuous mode 2 | 100Hz 연속 측정 |
| 1000 | Self-test | 자기장 발생 테스트 |
| 1111 | Fuse ROM access | ASA 보정값 읽기 모드 |

### 감도 보정 공식

```
Hadj = H × ((ASA - 128) × 0.5 / 128 + 1)
```
- H: 측정 raw 값
- ASA: ASAX/Y/Z 보정값 (Fuse ROM)
- Hadj: 보정된 측정값

### 자력 데이터 범위
- 14-bit 모드: 0.6 µT/LSB
- 16-bit 모드: 0.15 µT/LSB
- 풀스케일: ±4912 µT (±32760 LSB)
- **Little Endian** 순서 (Low byte 먼저)

## 11. I2C Master를 이용한 자력계 읽기 (SPI 모드에서 필수)

MPU-9250이 SPI로 호스트와 통신할 때, AK8963은 내부 I2C Master로만 접근 가능.

### 관련 레지스터

| 주소 | 이름 | 용도 |
|------|------|------|
| 0x24 (36) | I2C_MST_CTRL | I2C Master 클럭/설정 |
| 0x25 (37) | I2C_SLV0_ADDR | [7] R/W, [6:0] 슬레이브 주소 (0x0C) |
| 0x26 (38) | I2C_SLV0_REG | 읽기 시작 레지스터 주소 |
| 0x27 (39) | I2C_SLV0_CTRL | [7] EN, [3:0] 읽기 바이트 수 |
| 0x49-0x60 | EXT_SENS_DATA | 읽은 데이터 저장 위치 |
| 0x63 (99) | I2C_SLV0_DO | 쓰기 데이터 |

### I2C Master 클럭 설정 (I2C_MST_CLK[3:0])

| 값 | 속도 | 값 | 속도 |
|:--:|:--:|:--:|:--:|
| 0 | 348 kHz | 9 | 500 kHz |
| 5 | 286 kHz | 13 | 400 kHz |

## 12. 초기화 시퀀스 (SPI 모드)

1. VDD 인가 후 100ms 대기
2. PWR_MGMT_1(0x6B) = 0x80 → H_RESET (전체 리셋)
3. 100ms 대기
4. PWR_MGMT_1(0x6B) = 0x01 → CLKSEL=1 (auto PLL), SLEEP 해제
5. USER_CTRL(0x6A) = 0x10 → I2C_IF_DIS=1 (SPI 전용 모드)
6. GYRO_CONFIG(0x1B) 설정 → 풀스케일 선택
7. ACCEL_CONFIG(0x1C) 설정 → 풀스케일 선택
8. CONFIG(0x1A) 설정 → DLPF 대역폭
9. ACCEL_CONFIG2(0x1D) 설정 → 가속도계 DLPF
10. SMPLRT_DIV(0x19) 설정 → 샘플레이트
11. I2C Master 설정 → AK8963 접근을 위해
12. AK8963 초기화 (Fuse ROM 읽기 → 연속 측정 모드 설정)

## 13. 전원 모드

| 모드 | 자이로 | 가속도계 | 자력계 | 전류 |
|------|:--:|:--:|:--:|:--:|
| Sleep | Off | Off | Off | 8 µA |
| Accel Low Power | Off | Duty-cycled | Off | 8.4~19.8 µA |
| Accel Only | Off | On | Off | 450 µA |
| Gyro Only | On | Off | Off | 3.2 mA |
| 6축 (Accel+Gyro) | On | On | Off | 3.4 mA |
| 9축 전체 | On | On | On(8Hz) | 3.7 mA |

## 14. FIFO (512 바이트)

- FIFO_EN(0x23)으로 저장할 데이터 선택
- FIFO_COUNT(0x72-0x73): 현재 FIFO 바이트 수
- FIFO_R_W(0x74): FIFO 읽기/쓰기
- FIFO_MODE(CONFIG bit6): 1=가득 차면 추가 쓰기 거부, 0=오래된 데이터 덮어쓰기

## 15. GY-9250 모듈 핀 매핑 (SPI 모드)

| 모듈 핀 | MPU-9250 핀 | SPI 기능 | TC237 연결 |
|---------|------------|---------|-----------|
| VCC | VDD | 전원 | 3.3V |
| GND | GND | 그라운드 | GND |
| SCL | SCL/SCLK (23) | SCLK | QSPI CLK |
| SDA | SDA/SDI (24) | MOSI (SDI) | QSPI MOSI |
| EDA | AUX_DA (21) | - | 미연결 (외부 I2C) |
| ECL | AUX_CL (7) | - | 미연결 (외부 I2C) |
| AD0 | AD0/SDO (9) | MISO (SDO) | QSPI MISO |
| INT | INT (12) | 인터럽트 | GPIO (선택) |
| NCS | nCS (22) | Chip Select | QSPI CS (Active Low) |
| FSYNC | FSYNC (11) | 프레임 동기 | GND (미사용 시) |
