# TC21x/TC22x/TC23x User Manual 요약 (Quick Reference)

> **출처**: AURIX TC21x/TC22x/TC23x Family User's Manual V1.1, 2014-12 (Infineon Technologies)
> **대상**: TC237 펌웨어 개발자를 위한 핵심 모듈 레퍼런스

---

## 목차

1. [아키텍처 개요 / 메모리 맵](#1-아키텍처-개요--메모리-맵)
2. [SCU (System Control Unit) — 클럭, PLL, 리셋](#2-scu-system-control-unit)
3. [인터럽트 시스템 (IR/SRC)](#3-인터럽트-시스템-irsrc)
4. [Port/GPIO 모듈](#4-portgpio-모듈)
5. [VADC (Versatile Analog-to-Digital Converter)](#5-vadc)
6. [MultiCAN+ 모듈](#6-multican-모듈)
7. [ASCLIN (UART/LIN/SPI)](#7-asclin-uartlinspi)
8. [GTM (Generic Timer Module)](#8-gtm-generic-timer-module)
9. [STM (System Timer Module)](#9-stm-system-timer-module)
10. [DMA](#10-dma)

---

## 1. 아키텍처 개요 / 메모리 맵

### 1.1 디바이스 개요

TC21x/TC22x/TC23x는 TriCore 아키텍처 기반 32비트 마이크로컨트롤러로, RISC + DSP + 온칩 메모리/주변장치를 단일 다이에 통합한다.

| 특성 | TC21x | TC22x | TC23x | TC23x ADAS |
|------|-------|-------|-------|------------|
| CPU 코어 | TC1.6E | TC1.6E | TC1.6E | TC1.6E |
| 코어/체커코어 | 1/1 | 1/1 | 1/1 | 1/1 |
| 최대 주파수 | 133 MHz | 133 MHz | 200 MHz | 200 MHz |
| PFlash | 512 KB | 1 MB | 2 MB | 2 MB |
| DFlash | 64 KB | 96 KB | 128 KB | 128 KB |
| DSPR/PSPR | 48/8 KB | 88/8 KB | 184/8 KB | 184/8 KB |
| LMU SRAM | 0 KB | 0 KB | 0 KB | 32 KB |
| EMEM | 0 KB | 0 KB | 0 KB | 512 KB |
| DMA 채널 | 16 | 16 | 16 | 16 |
| ADC 입력 | 24 | 24 | 24 | 24 |
| ADC 컨버터 | 2 | 2 | 2 | 4 |
| PCache | 8 KB | 8 KB | 8 KB | 8 KB |
| FPU | Yes | Yes | Yes | Yes |

### 1.2 버스 시스템

- **SRI (Shared Resource Interconnect)**: 고속 크로스바 인터커넥트 (CPU, DMA, Flash, SRAM 연결)
- **SPB (System Peripheral Bus)**: 주변장치 연결 버스 (FPI 프로토콜)
- **SFI Bridge**: SRI와 SPB 간 양방향 브릿지 (주소 변환 없음, 투명)

### 1.3 메모리 맵 (Segment 기반)

| 세그먼트 | 주소 범위 | 내용 | CPU 캐시 속성 |
|----------|-----------|------|---------------|
| 0-6 | 0000_0000 - 6FFF_FFFF | Reserved | - |
| 7 | 7000_0000 - 7FFF_FFFF | CPU DSPR, PSPR, PCache | non-cached |
| 8 | 8000_0000 - 8FFF_FFFF | PFlash (cached), BROM | cached |
| 9 | 9000_0000 - 9FFF_FFFF | LMU SRAM, EMEM (cached) | cached |
| 10 | A000_0000 - AFFF_FFFF | PFlash, DFlash, BROM (non-cached) | non-cached |
| 11 | B000_0000 - BFFF_FFFF | LMU SRAM, EMEM (non-cached) | non-cached |
| 12-14 | C000_0000 - EFFF_FFFF | Reserved | - |
| 15 (F) | F000_0000 - FFFF_FFFF | SFR, CSFR (모든 레지스터) | non-cached |

### 1.4 주요 메모리 주소 (TC23x)

| 리소스 | 시작 주소 | 크기 |
|--------|-----------|------|
| CPU0 DSPR | 7000_0000 | 184 KB |
| CPU0 PSPR | 7010_0000 | 8 KB |
| CPU0 PCache | 7010_2000 | 8 KB |
| PFlash (cached) | 8000_0000 | 2 MB |
| BROM (cached) | 8FFF_8000 | 32 KB |
| PFlash (non-cached) | A000_0000 | 2 MB |
| DFlash (DF0) | AF00_0000 | 128 KB + 16 KB |
| BROM (non-cached) | AFFF_8000 | 32 KB |

### 1.5 Segment F 구조 (SFR 영역)

Segment F의 첫 16KB (F000_0000 ~ F000_3FFF) 내 모듈은 절대 주소 모드로 접근 가능:

| 모듈 | 비고 |
|------|------|
| STMx, Cerberus, ASCLINx | 첫 16KB 내 |
| QSPIx, CCU6x, GPT12x, SENT | 첫 16KB 내 |
| DMA, MultiCAN, ERAY, SAR-ADC | 상대 주소 모드 범위 |
| SCU, SMU, IR, Ports, GTM | 상위 영역 |

---

## 2. SCU (System Control Unit)

### 2.1 기능 개요

System Control Cluster는 다음 모듈로 구성:

| 모듈 | 기능 |
|------|------|
| **CCU** (Clock Control Unit) | 클럭 소스, PLL, 클럭 분배, 개별 클럭 설정 |
| **RCU** (Reset Control Unit) | 리셋 타입, 리셋 소스, 리셋 생성 |
| **PMC** (Power Management Controller) | 전원 공급, 전력 관리 모드 |
| **SCU** | ERU, WDT, Emergency Stop, NMI, OVC 레지스터 |

### 2.2 클럭 시스템 (CCU)

클럭 트리: **Clock Source -> Clock Speed Upscaling (PLL) -> Clock Distribution -> Individual Clock Configuration**

#### 2.2.1 클럭 소스

| 소스 | 설명 |
|------|------|
| **OSC** (외부 크리스탈/세라믹 레조네이터) | Pierce 오실레이터, XTAL1/XTAL2 핀 |
| **외부 입력 클럭** | XTAL1에 직접 클럭 입력, XTAL2 미연결 |
| **백업 클럭** | 내부 클럭, 정밀도 낮지만 항상 사용 가능 |

#### 2.2.2 OSCCON 레지스��� (오실레이터 제어) -- 오프셋 010H

| 비트필드 | 비트 | 타입 | 설명 |
|----------|------|------|------|
| PLLLV | 1 | rh | OSC 주파수 PLL 유효 (Low), 1=사용가능 |
| OSCRES | 2 | w | OSC WDT 리셋 (1=클리어 후 재시작) |
| GAINSEL | [4:3] | rw | 게인 선택: 00=4-8MHz, 01=4-16MHz, 10=4-20MHz, 11=4-25/40MHz |
| MODE | [6:5] | rw | 00=Crystal+ExtClk, 01=OSC비활성, 10=ExtClk+PowerSaving, 11=OSC비활성+PS |
| SHBY | 7 | rw | Shaper 바이패스 |
| PLLHV | 8 | rh | OSC 주파수 PLL 유효 (High), 1=사용가능 |
| OSCVAL | [20:16] | rw | fOSCREF = fOSC/(OSCVAL+1), fOSCREF 약 2.5MHz 목표 |
| APREN | 23 | rw | 진폭 조절 활성화 |

### 2.3 PLL (Phase-Locked Loop)

#### PLL 공식

| 모드 | 출력 주파수 공식 |
|------|-----------------|
| **Normal Mode** | fPLL = (N / (P x K2)) x fOSC, fPLL2 = (N / (P x K3)) x fOSC |
| **Prescaler Mode** | fPLL = fOSC / K1 |
| **Freerunning Mode** | fPLL = fPLLBASE / K2 (시스템 리셋 후 기본 모드) |

#### PLL 분주기

| 분주기 | 비트폭 | 범위 |
|--------|--------|------|
| P (입력 분주) | 4-bit | PDIV+1 |
| N (피드백 곱셈) | 7-bit | NDIV+1 |
| K1 (출력 분주) | 7-bit | K1DIV+1 |
| K2 (출력 분주) | 7-bit | K2DIV+1 |
| K3 (출력 분주) | 7-bit | K3DIV+1 |

#### PLLSTAT 레��스터 (오프셋 014H) -- 주요 상태 비트

| 비트필드 | 비트 | 설명 |
|----------|------|------|
| VCOBYST | 0 | 0=Freerunning/Normal, 1=Prescaler Mode |
| VCOLOCK | 2 | VCO Lock 상태 (1=locked) |
| FINDIS | 3 | fOSC disconnect 상태 (1=disconnected=Freerunning) |
| K1RDY | 4 | K1 분주기 준비 |
| K2RDY | 5 | K2 분주기 준비 |
| MODRUN | 7 | 주파수 변조 활성 |

#### PLLCON0 주요 제어 비트

| 비트필드 | 기능 |
|----------|------|
| VCOBYP | 1=Prescaler Mode 요청, 0=Normal/Freerunning |
| SETFINDIS | 1=Freerunning Mode 요청 (fOSC disconnect) |
| CLRFINDIS | 1=Normal Mode 요청 (fOSC connect) |
| RESLD | 1=VCO Lock 검출 리셋 |
| OSCDISCDIS | 1=Loss-of-Lock 시 OSC 분리 비활성 |
| PLLPWD | PLL Power Down |
| VCOPWD | VCO Power Down |
| MODEN | 주파수 변조 활성화 |

#### Normal Mode 진입 조건 (모두 충족 필요)

1. PLLSTAT.FINDIS = 0
2. PLLSTAT.VCOBYST = 0
3. PLLSTAT.VCOLOCK = 1
4. OSCCON.PLLLV = 1
5. OSCCON.PLLHV = 1

#### Normal Mode 진입 절차 (권장 순서)

1. Prescaler Mode 구성 및 진입
2. SMU의 VCO Loss-of-Lock 알람 비활성화
3. P, N, K2/K3 분주기 설정 (fVCO가 허용 범위 내)
4. PLLSTAT.VCOLOCK = 1 확인
5. PLLCON0.VCOBYP = 0으로 Normal Mode 전환
6. PLLSTAT.VCOBYST = 0 확인
7. SMU 알람 재활성화
8. K2 분주기 조정으로 목표 주파수 도달 (K2 변경 간 fPLL 6 사이클 대기)

### 2.4 클럭 분배 (CCUCON 레지스터)

| 레지스터 | 주요 비트필드 |
|----------|--------------|
| CCUCON0 | CLKSEL (클럭 소스 선택: 00=백업, 01=PLL), 각 모듈 분주비 |
| CCUCON1 | INSEL (PLL 입력 소스: 00=백업, 01=OSC) |

### 2.5 리셋 시스템 (RCU)

#### 리셋 타입

| 리셋 타입 | 범위 | 트리거 |
|-----------|------|--------|
| Power-On Reset | 전체 디바이스 | EVR, PORST 핀 |
| System Reset | 디지털 전체 | SW, WDT, SMU, ESRx |
| Application Reset | 응용부 | SW, STM, ESRx |
| Module Reset | 개별 모듈 | KRST 레지스터 |
| Debug Reset | 디버그 관련 | JTAG |

### 2.6 Watchdog Timer (WDT) 및 ENDINIT

- CPU별 WDT + Safety WDT
- ENDINIT 보호: 안전 관련 레지스터 보호 메커니즘
- Safety ENDINIT: 더 높은 수준의 보호 (SCU 관련 레지스터)
- 타이머 오버플로우 또는 잘못된 접근 시 리셋/인터럽트 발생

ENDINIT 해제 절차:
1. WDT_CON0에 올바른 패스워드 + ENDINIT=0 기록
2. 보호된 레지스터 수정
3. WDT_CON0에 올바른 패스워드 + ENDINIT=1 기록 (타이머 리로드)

> **중요**: ENDINIT 해제 후 일정 시간 내에 다시 설정하지 않으면 WDT 리셋 발생.

---

## 3. 인터럽트 시스템 (IR/SRC)

### 3.1 기능 개요

- 최대 512/1024개 서비스 요청 지원
- ICU(Interrupt Control Unit)당 최대 255개 우선순위 레벨
- CPU 및 DMA 모듈에 대한 전용 ICU
- 3~4 클럭 사이클의 저지연 중재
- 각 서비스 요청에 전용 SRN(Service Request Node) 할당
- HW 자동 클리어 (서비스 제공자 acknowledge 시)
- General Purpose Service Request (GPSR) 4개/CPU -- SW 인터럽트 용도
- Service Request Broadcast 메커니즘

### 3.2 SRC 레지스터 (Service Request Control Register)

모든 SRC 레지스터는 동일한 포맷. 각 서비스 요청 노드(SRN)마다 1개.

| 비트필드 | 비트 | 타입 | 설명 |
|----------|------|------|------|
| **SRPN** | [7:0] | rw | 서비스 요청 우선순위 (00H=최저/DMA ch0, FFH=최고). CPU는 01H부터 유효 |
| **SRE** | 10 | rw | 서비스 요청 활성화 (1=활성) |
| **TOS** | 11 | rw | 서비스 대상: 0=CPU0, 1=DMA |
| **ECC** | [20:16] | rwh | ECC (자동 계산, 에러 검출용) |
| **SRR** | 24 | rh | 서비스 요청 플래그 (1=대기 중) |
| **CLRR** | 25 | w | SRR 클리어 (1 기록 시) |
| **SETR** | 26 | w | SRR 설정 (SW 인터럽트 트리거) |
| **IOV** | 27 | rh | 인터럽트 트리거 오버플로우 |
| **IOVCLR** | 28 | w | IOV 클리어 |
| **SWS** | 29 | rh | SW Sticky Bit (SETR로 설정됨) |
| **SWSCLR** | 30 | w | SWS 클리어 |

### 3.3 SRN 재구성 절차

1. SRN 비활성화: SRC.SRE = 0
2. 비활성화 확인: SRC.SRE 읽어서 0 확인
3. LWSR 레지스터 확인 (이전 인터럽트 완료 확인)
4. SRC.TOS 및/또는 SRC.SRPN 변경
5. SRN 재활성화: SRC.SRE = 1

### 3.4 ICU 레지스터

| 레지스터 | 설명 |
|----------|------|
| LWSR (Latest Winning SR) | 최신 중재 승리 서비스 요청 정보 |
| LASR (Last Acknowledged SR) | 마지막 확인된 서비스 요청 정보 |
| ECR (Error Capture) | ECC 에러 캡처 |

### 3.5 SRC 접근 보호

- SRC[31:16]: ACCEN00으로 보호 (TOS, SRPN, SRE 설정)
- SRC[15:0]: ACCEN10으로 보호 (SW 인터럽트, Sticky, Overflow 제어)
- Master TAG ID 기반 접근 제어

### 3.6 외부 인터럽트 (ERU)

SCU의 ERU(External Request Unit)를 통해 설정:
- 4개 입력 선택기 (ERS)
- 4개 이벤트 트리거 로직 (ETL): rising/falling edge, high/low level
- 연결 매트릭스로 유연한 라우팅
- 4개 출력 게이팅 유닛 (OGU)

---

## 4. Port/GPIO 모듈

### 4.1 기능 개요

- 디지털 GPIO 포트 라인으로 온칩 주변장치 연결
- 각 핀: 입력/출력 개별 설정 가능
- 출력: push-pull 또는 open-drain
- 입력: pull-up, pull-down, 또는 무풀 선택 가능
- ALT1~ALT7 대체 출력 기능 (주변���치 연결)
- Emergency Stop 기능 (비상 시 tri-state 전환)
- 읽기: Pn_IN으로 항상 핀 레벨 읽기 가능 (출력 모드에서도)

### 4.2 포트 목록 (TC23x)

| 포트 | 베이스 주소 | 핀 수 | 비고 |
|------|------------|--------|------|
| P00 | F003_A000 | 13 | pins[12:0] |
| P02 | F003_A200 | 9 | pins[8:0] |
| P10 | F003_B000 | 5 | pins[6:5],[3:1] |
| P11 | F003_B100 | 8 | pins[3:2],6,[12:8] |
| P13 | F003_B300 | 4 | pins[3:0] |
| P14 | F003_B400 | 9 | pins[8:0] |
| P15 | F003_B500 | 9 | pins[8:0] |
| P20 | F003_C000 | 12 | pins[14:6],[3:2],0 |
| P21 | F003_C100 | 6 | pins[7:2] |
| P22 | F003_C200 | 5 | pins[4:0] |
| P23 | F003_C300 | 1 | pin1 |
| P33 | F003_D300 | 13 | pins[12:0] |
| P34 | F003_D400 | 4 | pins[3:0] |
| P40 | F003_E000 | 12 | pins[11:0] |
| P41 | F003_E100 | 12 | pins[11:0] |

### 4.3 주요 레지스터

| 레지스터 | 오프셋 | 기능 |
|----------|--------|------|
| **Pn_OUT** | 0000H | 출력 데이터 레지스터 |
| **Pn_OMR** | 0004H | 출력 수정 레지스터 (Set/Clear/Toggle 한번에) |
| **Pn_ID** | 0008H | 모듈 식별 |
| **Pn_IOCR0~12** | 0010H~001CH | 입력/출력 제어 (방향, pull, ALT 기능) |
| **Pn_IN** | 0024H | 입력 데이터 (읽기 전용) |
| **Pn_PDR0/1** | 0040H/0044H | 패드 드라이버 모드 (속도, 강도) |
| **Pn_ESR** | 0050H | Emergency Stop 레지스터 |
| **Pn_PDISC** | 0060H | 핀 기능 결정 제어 |
| **Pn_PCSR** | 0064H | 핀 컨트롤러 선택 |

### 4.4 IOCR (Input/Output Control) 비트필드

각 핀당 8비트 (PCx 필드). 비트[7:3]이 모드를 결정:

| PCx 값 | 모드 |
|---------|------|
| 0XXX0B | 입력 (no pull) |
| 0XX01B | 입력 (pull-down) |
| 0XX10B | 입력 (pull-up) |
| 1X000B | GPIO 출력 (push-pull) |
| 1X001B | ALT1 출력 (push-pull) |
| 1X010B | ALT2 출력 |
| 1X011B | ALT3 출력 |
| 1X100B | ALT4 출력 |
| 1X101B | ALT5 출력 |
| 1X110B | ALT6 출력 |
| 1X111B | ALT7 출력 |

> **참고**: 'X' 위치의 비트 = open-drain 선택 (1=open-drain, 0=push-pull)

### 4.5 OMR (Output Modification Register)

16비트 Set 필드 + 16비트 Clear 필드로 원자적 출력 제어:
- PSx (bit 0~15): 1 기록 시 해당 핀 Set
- PCLx (bit 16~31): 1 기록 시 해당 핀 Clear
- PSx=1 & PCLx=1: Toggle 동작

---

## 5. VADC

### 5.1 기능 개요

SAR(Successive Approximation Register) 방식 ADC 클러스터:
- 공칭 아날로그 공급 전압 5.0V, 3.3V 동작 지원 (성능 저하)
- 입력 전압 범위: 0V ~ 아날로그 공급 전압
- TC23x: 2개 독립 컨버터, 최대 12+2 아날로그 입력 (ADAS: 4개 컨버터)
- 변환 시간: 1us 미만 (결과 폭/샘플 시간 의존)
- 결과 폭: 8/10/12 비트 선택 가능
- 기본 모듈 클럭 fADC = fSPB
- 표준(VAREF) 및 대체(CH0) 기준전압 채널별 선택 가능

### 5.2 변환 요청 소스 및 모드

| 요청 소스 | 변환 모드 | 설명 |
|-----------|-----------|------|
| **Queue Source (RS0/RS3)** | Channel Sequence | 8개까지 임의 채널 순서 지정, 단일/반복 |
| **Scan Source (RS1/RS2)** | Auto Scan | 선형 순서로 모든 채널 스캔, 단일/반복 |
| **Background Source** | Background Scan | 그룹 소스에 미할당된 채널, 저우선순위 |

### 5.3 요청 소스 중재 (Arbitration)

- **Cancel-Inject-Restart**: 현재 변환 취소 후 고우선순위 변환 실행, 이후 재시작
- **Wait-for-Start**: 현재 변환 완료 후 고우선순위 변환 실행
- **Wait-for-Read**: 결과 레지스터 미읽음 시 변환 지연

### 5.4 입력 클래스 및 변환 제어

- 4개 입��� 클래스 (2개 그룹 전용 + 2개 글로벌)
- 각 채널별 입력 클래스 개별 할당
- 샘플 시간, 결과 해상도 등 설정 가능

### 5.5 결과 처리

- 16개 그룹별 결과 레지스터 + 1개 글로벌 결과 레지스터
- Wait-for-Read: 결과 덮어쓰기 방지
- 데이터 축소: 최대 4개 변환 결과 자동 누적
- FIR/IIR 필터 활성화 가능
- 결과 레지스터 연결하여 FIFO 구조 구성 가능

### 5.6 서비스 요청 생성

| 이벤트 | 설명 |
|--------|------|
| Source Event | 변환 ���퀀스 완료 시 |
| Channel Event | 특정 채널 변환 완료 시 (한계 검사와 결합 가능) |
| Result Event | 새 결과 데이터 가용 시 |

### 5.7 안전 기능

| 기능 | 설명 |
|------|------|
| BWD (Broken Wire Detection) | 입력 미연결 감지 (프리차지 레벨) |
| PDD (Pull Down Diagnostics) | 강한 풀다운으로 센서 연결 확인 |
| MD (Multiplexer Diagnostics) | 약한 풀업/풀다운으로 MUX 동작 확인 |
| CD (Converter Diagnostics) | 대체 신호로 컨버터 동작 확인 |

### 5.8 주요 레지스터

| 레지스터 | 오프셋 | 설명 |
|----------|--------|------|
| CLC | 0000H | 클럭 제어 (DISR, DISS, EDIS) |
| ID | 0008H | 모듈 식별 (00C5H = SARADC) |
| GLOBCFG | - | 글로벌 설정 |
| GxARBCFG | - | 그룹 중재 설정 |
| GxARBPR | - | 그룹 중재 우선순위 |
| GxCHCTRy | - | 채널 제어 (RESREG, ICLSEL 등) |
| GxRESy | - | 결과 레지스터 (RESULT, VF=Valid Flag) |
| GxQCTRL0 | - | Queue 소스 제어 |
| GxASCTRL | - | Auto Scan 제어 |
| GxASSEL | - | Scan 채널 선택 마스크 |

---

## 6. MultiCAN+ 모듈

### 6.1 기능 개요

- CAN 2.0A/2.0B 및 CAN FD 지원
- TC23x: 2개 CAN 노드, TC22x: 1개 CAN 노드
- 메시지 오브젝트 기반 통신
- Standard (11-bit) 및 Extended (29-bit) 식별자
- CAN FD: 최대 64바이트 데이터, 듀얼 비트레이트

### 6.2 CAN 프레임 타입

| 프레임 타입 | 설명 |
|------------|------|
| Classical CAN Base | 11-bit ID, 일정 비트레이트, 0-8바이트 |
| Classical CAN Extended | 29-bit ID, 일정 비트레이트, 0-8바이트 |
| CAN FD Base | 11-bit ID, 듀얼 비트레이트, 0-64바이트 |
| CAN FD Extended | 29-bit ID, 듀얼 비트레이트, 0-64바이트 |

### 6.3 비트 타이밍

| 세그먼트 | 설명 |
|----------|------|
| SYNC_SEG | 동기화 (항상 1 tQ) |
| PROP_SEG | 전파 지연 보상 |
| PHASE_SEG1 | 위상 에러 보상 (샘플 포인트 = PHASE_SEG1 끝) |
| PHASE_SEG2 | 후속 비트 레벨 계산 (>= 2 tQ) |

- 총 tQ: 8~25 범위
- tQ = (BRP+1) / fCAN
- Re-synchronization Jump Width: 1~4 tQ

### 6.4 모듈 구조

| 구성요소 | 설명 |
|----------|------|
| CAN Node | 비트 타이밍, 비트스트림 처리, 에러 핸들링, 프레임 카운터 |
| Message Object | 송수신 데이터 저장, 필터링, FIFO/게이트웨이 지원 |
| List Controller | 메시지 오브젝트의 노드 할당 관리 (PANCTR 레지스터) |

### 6.5 메시지 오브젝트 기능

| 기능 | 설명 |
|------|------|
| Standard Message Object | 기본 송수신 |
| Single Data Transfer | 단일 데이터 전송 |
| Receive/Transmit FIFO | FIFO 구조 (Base + Slave 오브젝트) |
| Gateway Mode | 수신 메시지 자동 전달 (노드 간) |
| CAN FD 64-byte | 확장 데이터 길이 지원 |

### 6.6 주요 레지스터

| 레지스터 그룹 | 설명 |
|--------------|------|
| **CAN_CLC** | 클럭 제어 |
| **CAN_MCR** | 모듈 제어 |
| **CAN_PANCTR** | Panel 제어 (List 명령) |
| **CAN_NxCR** | 노드 x 제어 (INIT, CCE, TRIE 등) |
| **CAN_NxSR** | 노드 x 상태 (LEC, TXOK, RXOK, BOFF, EWRN) |
| **CAN_NxBTR** | 노드 x 비트 타이밍 (BRP, SJW, TSEG1, TSEG2) |
| **CAN_NxECNT** | 노드 x 에러 카운터 (TEC, REC) |
| **CAN_NxIPR** | 노드 x 인터���트 포인터 |
| **CAN_MOFCRn** | 메시지 오브젝트 기능 제어 (MMC, DLC, RXIE, TXIE) |
| **CAN_MOARn** | 메시지 오브젝트 중재 (ID, IDE, PRI) |
| **CAN_MOAMRn** | 메시지 오브젝트 수용 마스크 |
| **CAN_MODATALn/Hn** | 메시지 데이터 Low/High |
| **CAN_MOCTRn** | 메시지 오브젝트 제어 (write-only: SETDIR, SETTXEN 등) |
| **CAN_MOSTATn** | 메시지 오브젝트 상태 (TXRQ, NEWDAT, MSGVAL 등) |

---

## 7. ASCLIN (UART/LIN/SPI)

### 7.1 기능 개요

| 모드 | 주요 특성 |
|------|-----------|
| **Standard ASC** | 전이중 비동기, 7/8/9(~16)비트 데이터, 패리티, 1/2 스톱비트, 최대 fA/16 (6.25 MBaud@100MHz) |
| **High Speed ASC** | 오버샘플��� 4~16배, 최대 fA/4 (25 MBaud@100MHz) |
| **LIN** | LIN 1.3/2.0/2.1, J2602, 마스터/슬레이브, 자동 보드레이트 감지 |
| **SPI** | 마스터 전용, 4-wire/3-wire, 최대 16비트 폭, 전이중/반이중 |

### 7.2 공통 특성

- 16바이트 TxFIFO / 16바이트 RxFIFO
- FIFO 패킹/언패킹 (8/16/32비트 버스 접근)
- 프로그래밍 가능 오버샘플링 (4~16배/비트)
- 프로그래밍 가능 샘플링 포인트
- 디지털 글리치 필터 + 미디언 필터
- 내부 루프백 모드
- CPU 또는 DMA 트리거 가능 인터럽트

### 7.3 외부 신호

| 신호 | 방향 | 용도 |
|------|------|------|
| ARX | 입력 | 수신 데이터 (SPI: MRST) |
| ATX | 출력 | 송신 데이터 (SPI: MTSR) |
| ASCLKO | 출력 | 시리얼 클럭 (SPI) |
| ASLSO | 출력 | 슬레이브 선택 (SPI) |
| ARTS | 출력 | RTS 핸드셰이크 |
| ACTS | 입력 | CTS 핸드셰이크 |

### 7.4 보드레이트 생성

- 클럭 소스 선택: fASCLINS, fASCLINF, fERAY, fOSC0
- fA (모듈 클럭) -> Fractional Divider -> fOVS (오버샘플링 클럭) -> fSHIFT (시프트 클럭)
- Baud Rate = fOVS / (오버샘플링 비율)
- 최소 보드레이트: fA / 268,435,456 (약 0.37 Baud@100MHz)

### 7.5 LIN 주요 기능

| 기능 | 설명 |
|------|------|
| Break 감지/주입 | LIN 프레임 시작 |
| Sync 필드 생성 | 자동 동기화 |
| Auto Baud Detection | Sync 필드 측정 기반 |
| Collision Detection | LIN 2.1 필수 |
| Header/Response Timeout | LIN Watchdog |
| Wake-Up | 슬립/웨이크업 지원 |

### 7.6 주요 레지스터

| 레지스터 | 설명 |
|----------|------|
| CLC | 클럭 제어 |
| IOCR | 입출력 제어 (ALTI=대체 입력 선택) |
| FRAMECON | 프레임 구성 (MODE, DATLEN, STOP, PEN) |
| DATCON | 데이터 구성 (DATLEN) |
| BRG | 보드레이트 생성 (NUMERATOR, DENOMINATOR) |
| BRD | 보드레이트 검출 (LIN 자동 보드레이트) |
| BITCON | 비트 타이��� (OVERSAMPLING, SAMPLEPOINT) |
| TXFIFOCON | TX FIFO 제어 (INW, 인터럽트 레벨) |
| RXFIFOCON | RX FIFO 제어 (OUTW, 인터럽트 레벨) |
| TXDATA | 송신 데이터 |
| RXDATA | 수신 데이터 |
| FLAGS | 상태 플래그 (TH, TR, RH, RR, FE, PE, OE) |
| FLAGSSET/FLAGSCLEAR | 플래그 설정/클리어 |
| LINCON | LIN 제어 |
| CSR | 클럭 소스 선택 |

---

## 8. GTM (Generic Timer Module)

### 8.1 기능 개요

범용 타이머 모듈로, 구성 가능한 서브모듈 조합으로 다양한 타이머 기능 제공.

| 서브모듈 | 기능 |
|----------|------|
| **CMU** (Clock Management Unit) | 최대 13개 클럭 생성 + 3개 외부 클럭 |
| **TBU** (Time Base Unit) | 3개 독립 공통 시간 기준 |
| **TIM** (Timer Input Module) | 입력 신호 필터링/캡처/측정 (8채널) |
| **TOM** (Timer Output Module) | PWM 출력 생성 (16채널/인스턴스) |
| **DTM** (Dead Time Module) | 데드 타임 삽입 |
| **ICM** (Interrupt Concentrator) | 인터럽트 집중/그룹화 |

### 8.2 TC23x GTM 구성

- TIM0: 8채널
- TOM0, TOM1: 각 16채널
- DTM1, DTM5: 각 4채널
- CMU: 8개 구성 가능 클럭(CLK0~7) + 5개 고정 클럭(FXCLK0~4) + 3개 외부 클럭(ECLK0~2)
- TBU: 3채널 (TBU_TS0, TBU_TS1, TBU_TS2)

### 8.3 CMU (Clock Management Unit)

| 클럭 그룹 | 용도 |
|-----------|------|
| CMU_CLK[0~7] | 구성 가능 클럭 -- TIM 등 서브모듈용 |
| CMU_FXCLK[0~4] | 고정 분주 클럭 -- TOM PWM 생성용 |
| CMU_ECLK[0~2] | 외부 클럭 출력 |

- 글로벌 분주기: CMU_GCLK_NUM / CMU_GCLK_DEN으로 SYS_CLK 분주
- 각 CMU_CLK[x]: CMU_CLK_x_CTRL로 개별 분주 설정
- CMU_CLK_EN: 개별 클럭 활성화/비활성화

### 8.4 TBU (Time Base Unit)

- 3개 채널: TBU_CH0, TBU_CH1, TBU_CH2
- 각 채널: CMU 클럭 중 하나로 구동
- TBU_CHEN: 채널 활성화 제어
- TBU_TS0: TIM0에서 확장 27비트 폭으로 연결

### 8.5 TIM (Timer Input Module) -- 8채널

#### 기능
- 입력 신호 필터링 (3가지 필터 모드)
- 에지 감지 (rising/falling)
- PWM 측정 (주기, 듀티)
- 타임스탬프 캡처 (TBU 기반)
- ���임아웃 감지 (TDU)
- 24비트 내부 동작 레지스터

#### 필터 모드

| 모드 | 설명 |
|------|------|
| Immediate Edge Propagation | 에지 즉시 전달, 수용 시간 동안 글리치 무시 |
| Individual De-glitch (Up/Down) | ���/다운 카운터로 디글리치 |
| Individual De-glitch (Hold) | 홀드 카운터로 디글리치 |

- 필터 카운터 클럭: CMU_CLK0, CMU_CLK1, CMU_CLK6, CMU_CLK7 선택
- 디글리치 시간: T = (FLT_xE + 1) x T_FLT_CLK
- FLT_RE/FLT_FE: rising/falling 에지별 독립 설정 (24비트)

#### 주요 TIM 레지스터

| 레지스터 | 설명 |
|----------|------|
| TIM0_CHx_CTRL | 채널 제어 (모드, 필터 클럭, 에지 선택 등) |
| TIM0_CHx_IN_SRC | 입력 소스 선택 (CICTRL, MODE, VAL) |
| TIM0_CHx_FLT_RE | Rising 에지 필터 파라미터 |
| TIM0_CHx_FLT_FE | Falling 에지 필터 파라미터 |
| TIM0_CHx_GPR0/1 | 범용 레지스터 (캡처 값) |
| TIM0_CHx_CNT | 카운터 |
| TIM0_CHx_IRQ_NOTIFY | 인터럽트 알림 |
| TIM0_CHx_IRQ_EN | 인터럽트 활성화 |

### 8.6 TOM (Timer Output Module) -- 16채���/인스턴스

#### 기능
- 각 채널: 독립 PWM 출력 생성
- CCU0: 주기 제어 (CN0 카운터, CM0 비교)
- CCU1: 듀티 사이클 제어 (CM1 비교)
- 쉐도우 레지스터 (SR0/SR1): 글리치 없는 업데이트
- TGC0/TGC1: 글로벌 채널 제어 (각 8채널)
- CMU_FXCLK 5개 클럭 선택 가능

#### TOM 채널 구조

| 구성요소 | 설명 |
|----------|------|
| CN0 (16-bit 카운터) | CMU_FXCLK로 구동, CM0 비교 시 리셋 |
| CM0 (Compare 0) | 주기 설정 (SR0에서 업데이트) |
| CM1 (Compare 1) | 듀티 설정 (SR1에서 업데이트) |
| SOU (Signal Output) | SOUR 플립플롭으로 출력 생성 |
| SL (Signal Level) | 출력 비활성 시 기본 레벨 |

#### TGC (Global Channel Control) -- 3가지 트리거 소스

| 트리거 | 설명 |
|--------|------|
| HOST_TRIG | CPU 직접 트리거 (레지스터 기록) |
| TBU 타임스탬프 | TBU_TS0/1/2 비교 매치 (ACT_TB) |
| TRIG (내부) | 채널 트리거 또는 TIM_EXT_CAPTURE |

#### TGC 제어 메커니즘

| 레지스터 | 기능 |
|----------|------|
| TOMi_TGCy_ENDIS_CTRL/STAT | 채널 활성화/비활성화 |
| TOMi_TGCy_OUTEN_CTRL/STAT | 출력 활성화/비활성화 |
| TOMi_TGCy_FUPD_CTRL | 강제 업데이트 (Shadow -> Active) |
| TOMi_TGCy_GLB_CTRL | UPEN_CTRL, HOST_TRIG, TBU_SEL 등 |
| TOMi_TGCy_ACT_TB | TBU 비교 값 |

#### 주요 TOM 채널 레지스터

| 레지스터 | 설명 |
|----------|------|
| TOMi_CHx_CTRL | 채널 제어 (SL, CLK_SRC, OSM 등) |
| TOMi_CHx_SR0 | 쉐도우 레지스터 0 (주기) |
| TOMi_CHx_SR1 | 쉐도우 레지스터 1 (듀티) |
| TOMi_CHx_CM0 | Compare 0 (동작 중 주기) |
| TOMi_CHx_CM1 | Compare 1 (동작 중 듀티) |
| TOMi_CHx_CN0 | 카운터 |
| TOMi_CHx_IRQ_NOTIFY | 인터럽트 알림 (CCU0TC, CCU1TC) |
| TOMi_CHx_IRQ_EN | 인터럽트 활성화 |

### 8.7 GTM 인터럽트

4가지 인터럽트 모드 (IRQ_MODE 레지스터):

| 모드 | 설명 |
|------|------|
| Level | 기본 모드, 이벤트 발생 시 레벨 유지 |
| Pulse | 이벤트 시 펄스 발생 |
| **Pulse-Notify** | **권장 모드**, 펄스 + 알림 |
| Single-Pulse | 단일 펄스 |

인터럽트 제어 레지스터 세트:
- IRQ_NOTIFY: 이벤트 수집 (1 기록으로 클리어)
- IRQ_EN: 개별 인터럽트 활성화
- IRQ_FORCINT: SW 트리거 (GTM_CTRL.RF_PROT = 0 필요)
- IRQ_MODE: 인터럽트 모드 설정 (기록 시 IRQ_NOTIFY 전부 클리어)

---

## 9. STM (System Timer Module)

### 9.1 기능 개요

- 64비트 free-running 업카운터
- fSTM 클럭으로 구동
- Application Reset 후 자동 카운트 시작 (ARSTDIS 비트로 리셋 동작 제어 가능)
- 타이머 내용 변경 불가 (읽기 전용)
- 디버그 시 서스펜드 가능 (클럭 정지, 레지스터 읽기 가능)

### 9.2 베이스 주소

| 모듈 | 베이스 주소 | 끝 주소 |
|------|------------|---------|
| STM0 | F000_0000 | F000_00FF |

### 9.3 타이머 읽기

64비트 타이머를 32비트 단위로 읽기:
- TIM0~TIM6: 점점 높은 차수의 32비트 부분 선택
- **CAP** (Capture): TIM0~TIM5 읽기 시 상위 부분 자동 래치

#### 일관된 64비트 읽기 절차
1. TIMx 읽기 (하위 부분) -> CAP 자동 래치
2. CAP 읽기 (상위 부분) -> 동일 시점의 상위 값 획득

### 9.4 비교 레지스터

2개 비교 레지스터 (CMP0, CMP1):
- **MSIZEx**: 비교 폭 (0~31, 비교할 비트 수)
- **MSTARTx**: 시작 위치 (0~31, 64비트 STM 내 비교 시작 비트)
- 비교 매치 시 서비스 요청 생성 가능
- 단일 비트 전이 감지 가능 (MSIZE=0, MSTART=n)

### 9.5 인터���트 제어

| 레지스터 | 설명 |
|----------|------|
| ICR | CMPxIR (비교 매치 플래그), CMPxEN (활성화), CMPxOS (출력 선택 STMIR0/1) |
| ISCR | CMPxIRS (SW 설정), CMPxIRR (SW 클리어) |

> **주의**: 리셋 후 CMPxIR이 즉시 설정됨 (리셋 값과 비교 매치). 인터럽�� 활성화 전 CMPxIRR로 클리어 필수.

### 9.6 STM 레지스터 요약

| 레지스터 | 오프셋 | 설명 |
|----------|--------|------|
| CLC | 00H | 클럭 제어 |
| ID | 08H | 식별 |
| TIM0 | 10H | 타이머 [31:0] |
| TIM1 | 14H | 타이머 [35:4] |
| TIM2 | 18H | 타이머 [39:8] |
| TIM3 | 1CH | 타이머 [43:12] |
| TIM4 | 20H | 타이머 [47:16] |
| TIM5 | 24H | 타이머 [51:20] |
| TIM6 | 28H | 타이머 [63:32] |
| CAP | 2CH | 캡처 레지스터 |
| CMP0 | 30H | 비교 레지스터 0 |
| CMP1 | 34H | 비교 레지스터 1 |
| CMCON | 38H | 비교 매치 제어 (MSIZE0/1, MSTART0/1) |
| ICR | 3CH | 인터럽트 제어 |
| ISCR | 40H | 인터럽트 설정/클리어 |
| TIM0SV | 50H | 타이머 0 두 번째 뷰 |
| CAPSV | 54H | 캡처 두 번째 뷰 |
| OCS | E8H | OCDS 제어/상태 |

### 9.7 STM을 리셋 트리거로 사용

CMP0 비교 매치 시 시스템 리셋 트리거 가능 (SCU_RSTCON에서 STM별 활성화).

### 9.8 주기적 인터럽트 구현

STM은 카운터를 리셋할 수 없으므로 주기적 인터럽트는 **비교값을 매번 증가**시키는 방식:
```
ISR 내부:
1. 플래그 클리어
2. CMP = CMP + ticks (다음 매치 시점 설정)
```

---

## 10. DMA

### 10.1 기능 개요

- 16개 DMA 채널 (ch015 = 최고 우선순위, ch000 = 최저)
- 2개 Move Engine (병렬 실행)
- ���랜잭션 제어 셋: DMARAM에 저장 (8 x 32비트 워드/채널)
- 하드웨어 요청: Interrupt Router ICU 경유 (어떤 주변장치도 DMA 트리거 ���능)
- 소프트웨어 요청 지원
- SRI 클럭 주파수로 커널 동작 (최대 처리량)

### 10.2 DMA 용어

| 용어 | 정의 |
|------|------|
| **DMA Move** | Read Move(소스->DMA) + Write Move(DMA->목적지) |
| **DMA Transfer** | 1/2/3/4/5/8/9/16개 Move로 구성 |
| **DMA Transaction** | 여러 Transfer로 구성 (Transfer Count 설정) |
| **Linked List** | 동일 채널에서 연속 Transaction 실행 |

### 10.3 데이터 폭

| 버스 | 지원 폭 |
|------|---------|
| SPB 마스터 | 8/16/32비트 |
| SRI 마스터 | 8/16/32/64/128/256비트 |

### 10.4 처리량

- SRI-to-SRI: >8 MB/transaction
- FPI 소스 또는 FPI 목적지: >1 MB/transaction

### 10.5 주요 기능

| 기능 | 설명 |
|------|------|
| **Shadow Address** | 동작 중 다음 Transaction 주소 미리 설정 (소스 또는 목적지 중 하나) |
| **Circular Buffer** | 유연한 크기의 순환 버퍼 주소 지정 |
| **Double Buffering** | 2개 버퍼 간 자동 전환 |
| **Linked List** | DMARAM에 다음 TCS 자동 로드 + 자동 시작 |
| **Safe Linked List** | CRC 체크섬 검증 |
| **Conditional Linked List** | 조건부 실행 |
| **Pattern Detection** | 8/16/32비트 데이터 패턴 비교 |
| **Flow Control** | 채널 간 데이지 체인 (완료 시 다음 채널 자동 시작) |
| **Access Protection** | 4개 HW 리소스 파티션, Master TAG ID 기반 접근 제어 |

### 10.6 채널 동작 모드

| 모드 | 설명 |
|------|------|
| Single Mode | 1 Transaction 후 정지 |
| Continuous Mode | Transaction 완료 후 자동 재시작 (TCOUNT 리로드) |

### 10.7 Shadow Address 메커니즘

ADICRz.SHCT 필드로 제어:

| SHCT 값 | 모드 | 설명 |
|---------|------|------|
| 0001B | Read Only, Source Shadow | 소스 주소 -> MExSHADR (자동), 직접 쓰기 불가 |
| 0010B | Read Only, Dest Shadow | 목적지 주소 -> MExSHADR (자동), 직접 쓰기 불가 |
| 0101B | Direct Write, Source Shadow | MExSHADR 직접 기록 가능 |
| 0110B | Direct Write, Dest Shadow | MExSHADR 직접 기록 가능 |

Transaction 완료 시 SHADR -> 실제 주소로 자동 전송.

### 10.8 주요 레지스터

#### DMA 채널 트랜잭션 제어 셋 (DMARAM, 8 워드/채널)

| 레지스터 | 설명 |
|----------|------|
| **MExCHSR** | 채널 상태 (TCOUNT, ECH, FROZEN 등) |
| **MExCHCR** | 채널 제어 (BLKM, RROAT, CHMODE, CHDW, TREL 등) |
| **MExADICR** | 주소/인터럽트 제어 (SHCT, SCBE, DCBE, WRPSE/DE 등) |
| **MExSHADR** | 쉐도우 주소 |
| **MExSADR** | 소스 주소 |
| **MExDADR** | 목적지 주소 |
| **MExSDCRC** | 소스/목적지 주소 CRC |
| **MExRDCRC** | 읽기 데이터 CRC |

#### MExCHCR 주요 필드

| 필드 | 설명 |
|------|------|
| TREL | Transfer Reload (Transfer 수) |
| BLKM | Block Mode (Move 수: 1/2/4/8/16 등) |
| RROAT | Request Only After Transaction (HW 요청 모드) |
| CHMODE | Channel Mode (0=Single, 1=Continuous) |
| CHDW | Channel Data Width (8/16/32/64/128/256비트) |

#### DMA 인터럽트

| 인터럽트 | 설명 |
|----------|------|
| Channel Transfer | Transfer 완료 시 (각 채널 고유 벡터) |
| Pattern Detection | 패턴 매치 시 |
| Wrap Buffer | 순환 버퍼 래핑 발생 시 |
| Transaction Lost | 미처리 요청 손실 시 |
| Source/Dest Error | 주소 에러 |
| Linked List Error | 링크드 리스트 에러 |

### 10.9 DMA 트리거 (SRC에서 TOS=DMA 설정)

SRC 레지스터에서 TOS=1 설정 시 해당 인터럽트가 CPU 대신 DMA 채널을 트리거. SRPN 필드가 DMA 채널 번호로 사용됨 (SRPN <= 최고 DMA 채널 번호).

---

## 부록: 주요 모듈 베이스 주소 요약

| 모듈 | 베이스 주소 |
|------|------------|
| STM0 | F000_0000 |
| ASCLIN0 | F000_0600 |
| ASCLIN1 | F000_0700 |
| DMA | F001_0000 |
| VADC | F002_0000 |
| MultiCAN | F020_0000 |
| SCU | F003_6000 |
| IR (Interrupt Router) | F003_7000 |
| Port 00 | F003_A000 |
| Port 02 | F003_A200 |
| Port 10 | F003_B000 |
| Port 11 | F003_B100 |
| Port 14 | F003_B400 |
| Port 15 | F003_B500 |
| Port 20 | F003_C000 |
| Port 33 | F003_D300 |
| Port 40 | F003_E000 |
| Port 41 | F003_E100 |
| GTM | F010_0000 |

> **참고**: 정확한 주소는 User's Manual Table 4-3 (Segment 15) 참조.
