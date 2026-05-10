# TC23x (TC233/TC234/TC237) 데이터시트 요약

> 출처: TC233/TC234/TC237 Data Sheet V1.1, 2015-06 (Infineon Technologies AG)
> AURIX 32비트 싱글 칩 마이크로컨트롤러, A-Step

---

## 1. 디바이스 개요 및 제품 라인업

### 1.1 제품 패밀리 개요

TC23x 제품 패밀리는 **싱글 코어 TriCore CPU (TC1.6E)** 기반의 고성능 마이크로컨트롤러이다.

- **CPU**: TriCore TC1.6E (TC1.6P와 바이너리 코드 호환)
- **최대 동작 주파수**: 200 MHz (전체 온도 범위)
- **FPU (부동소수점 연산 유닛)**: 지원
- **Lockstep 섀도우 코어**: TC1.6E용 안전 체커 코어 탑재
- **안전 등급**: ASIL-D까지 지원

### 1.2 제품 변형 (Ordering Code)

| 변형 | 패키지 | 주요 차이점 |
|------|--------|------------|
| TC233x | PG-TQFP-100-23 | 100핀 TQFP 패키지 |
| TC234x (TC23x-ADAS) | PG-TQFP-144-27 | 144핀 TQFP 패키지 |
| TC237x | PG-LFBGA-292-6 | 292볼 LFBGA 패키지 (풀 기능) |

- **SAK 제품**: 동작 온도 -40 ~ +125 C (접합 온도 -40 ~ +150 C)
- **SAL 제품**: 동작 온도 -40 ~ +150 C (접합 온도 -40 ~ +165 C)
- **SAK-TC233LC-24F133F**: 최대 주파수 133 MHz 제한 변형 (저가형)

### 1.3 Feature Package 구분

데이터시트에서 Feature Package로 L, LC, LP, LA, LX를 구분하며, 전류 소비/열 특성 등이 상이하다.

---

## 2. 메모리 구성

| 메모리 유형 | 크기 | 비고 |
|-------------|------|------|
| Program Flash (PFLASH) | 최대 2 MByte | ECC (SECDED) 보호 |
| Data Flash (DFLASH) | 최대 128 KByte | EEPROM 에뮬레이션 가능, ECC 보호 |
| DSPR (Data Scratch-Pad RAM) | 최대 184 KByte | CPU 전용 데이터 SRAM, 제로 웨이트 스테이트 |
| PSPR (Instruction Scratch-Pad RAM) | 최대 8 KByte | CPU 전용 명령어 SRAM |
| Instruction Cache (ICACHE) | 8 KByte | - |
| Data Read Buffer (DRB) | 4 라인 | - |
| LMU (Local Memory Unit) | 32 KByte | 공유 메모리 |
| EMEM (Emulation Memory) | 512 KByte | 변형별 탑재 여부 상이 |
| BootROM (BROM) | 있음 | 부트 펌웨어 저장 |

> **주의**: 모든 내장 NVM 및 SRAM은 ECC 보호 적용됨.
> 명령어 메모리 상위 64바이트는 선행 페치(Speculative Fetch) 문제로 명령어 저장 미사용 권장.

---

## 3. 핀 배치 및 패키지 정보

### 3.1 패키지 종류

| 패키지 | 디바이스 | 핀/볼 수 | EPad 크기 (nominal) |
|--------|----------|---------|---------------------|
| PG-LFBGA-292-6 | TC237x | 292볼 (0.8mm 피치) | - |
| PG-TQFP-144-27 | TC234x (ADAS) | 144핀 | Ex/Ey: 5.7mm (L,LP), 6.7mm (LA,LX) |
| PG-TQFP-100-23 | TC233x | 100핀 | Ex/Ey: 5.7mm (L,LC,LP) |

### 3.2 열 특성 (Thermal Resistance)

| 디바이스 | 패키지 | R_JCT (K/W) | R_JCB (K/W) | R_JA (K/W) | 조건 |
|----------|--------|-------------|-------------|------------|------|
| TC233 | PG-TQFP-100-23 | 21.2 | 12.1 | 30.4 | 솔더 EPad, 내부 LDO |
| TC233 | PG-TQFP-100-23 | 11.9 | 2.9 | 20.4 | 솔더 EPad, DCDC EVR |
| TC234 | PG-TQFP-144-27 | 20.9 | 11.7 | 30.0 | 솔더 EPad, 내부 LDO |
| TC234 | PG-TQFP-144-27 | 11.7 | 2.8 | 19.8 | 솔더 EPad, DCDC EVR |
| TC237 | PG-LFBGA-292-6 | 13.7 | 21.0 | 33.1 | 내부 LDO |
| TC237 | PG-LFBGA-292-6 | 7.6 | 14.1 | 25.4 | DCDC EVR |

> 접합 온도 계산: TJ = TA + R_JA * PD

### 3.3 I/O 타입

- **3.3V CMOS** (ADC 핀에서 5V 입력 지원)
- **패드 클래스**: A1 (3.3V 일반), A1+ (3.3V 강화 드라이버), S (ADC 아날로그/디지털), I (입력 전용)

### 3.4 핀 멀티플렉싱 / Alternative 기능

각 GPIO 핀은 최대 8개의 출력 기능(O0~O7)과 다수의 입력 기능을 멀티플렉싱한다.

- **출력 선택**: IOCR 레지스터의 PCx 비트 필드로 ALT1~ALT7 선택
  - O0 (PCx=1X000B): GPIO 출력
  - O1 (ALT1, PCx=1X001B): 주로 GTM_TOUT
  - O2~O7: ASCLIN, CAN, CCU6, SENT 등 주변장치 출력
- **입력 연결**: 여러 주변장치 입력이 동시에 하나의 핀에 연결 가능 (주변장치 설정으로 선택)
- **GTM 서브 멀티플렉서**: GTM_TOUT 출력은 GTM 모듈 내부 서브 MUX로 TOM 채널 선택
- **리셋 시 상태**: 핀별로 PU(풀업), PD(풀다운), High-Z 상태 정의됨

#### 드라이버 모드 선택

| PDx 설정 | A1 패드 | A1+ 패드 |
|----------|---------|----------|
| PDx.0=0 | medium (A1m) | Strong slow edge (A1+sw) |
| PDx.0=1 | weak (A1w) | Strong soft edge (A1+sf) |
| PDx.1 변경 | - | medium (A1+m) |
| PDx.2 변경 | - | weak (A1+w) |

---

## 4. 전기적 특성

### 4.1 절대 최대 정격 (Absolute Maximum Ratings)

| 파라미터 | 심볼 | 최대값 | 단위 | 비고 |
|----------|------|--------|------|------|
| 보관 온도 | T_ST | -65 ~ +170 | C | 170 C에서 15시간 이내 |
| VDD 전원핀 전압 | VDD_SR | 1.9 | V | VSS 기준 |
| VDDP3 전원핀 전압 | VDDP3_SR | 4.43 | V | VSS 기준 |
| VDDM 전원핀 전압 | VDDM_SR | 7.0 | V | VSS 기준 |
| 아날로그/S 클래스 입력 전압 | VIN_SR | -0.5 ~ 7.0 | V | VSS 기준 |
| 기타 입력핀 전압 | VIN_SR | -0.5 ~ min(VDDP3+0.5, 4.43) | V | 작은 값 적용 |

### 4.2 동작 조건 (Operating Conditions)

#### 전원 전압

| 파라미터 | 심볼 | 최소 | 표준 | 최대 | 단위 | 비고 |
|----------|------|------|------|------|------|------|
| 코어 전원 (VDD) | VDD | 1.17 | 1.3 | 1.43 | V | 외부 공급 시만 필요 |
| GPIO 디지털 전원 (VDDP3) | VDDP3 | 2.97 | 3.3 | 3.63 | V | 모든 VDDP3 핀 PCB에서 연결 |
| ADC 아날로그 전원 (VDDM) | VDDM | 2.97 | 5.0 | 5.5 | V | - |
| 디지털 그라운드 (VSS) | VSS | 0 | - | - | V | - |
| 아날로그 그라운드 (VSSM) | VSSM | -0.1 | 0 | 0.1 | V | VDDM 기준 |
| 패드 상태 보장 전압 | VDDPPA | 0.72 | - | - | V | PORST 로우 유지 시 |

#### 온도 범위

| 파라미터 | 최소 | 최대 | 비고 |
|----------|------|------|------|
| 주변 온도 (SAK) | -40 C | +125 C | 모든 SAK 제품 |
| 주변 온도 (SAL) | -40 C | +150 C | 모든 SAL 제품 |
| 접합 온도 (SAK) | -40 C | +150 C | - |
| 접합 온도 (SAL) | -40 C | +165 C | - |

### 4.3 전원 공급 전류

| 파라미터 | 최대값 | 단위 | 조건 |
|----------|--------|------|------|
| IDD 합계 (1.3V 코어+주변) | 215 | mA | 패키지 L/LC/LP, 최대 전력 패턴 |
| IDD 합계 (1.3V) | 160 | mA | 패키지 L/LC/LP, 실제 전력 패턴 |
| IDD 합계 (1.3V) | 236 | mA | 패키지 LA/LX, 최대 전력 패턴 |
| IDD 합계 (1.3V) | 181 | mA | 패키지 LA/LX, 실제 전력 패턴 |
| PORST 중 IDD 코어 전류 | 95 | mA | LA/LX, TJ=165 C |
| CPU0 lockstep 추가 전류 | 20 | mA | - |
| HSM 추가 전류 | 34 | mA | - |
| 3.3V 공급 전류 (패드 비활성) | 34 | mA | 실제 전력 패턴 |
| IDDM 공급 전류 (5V) | 6 | mA | 실제 전력 패턴 |
| 전체 합산 전류 (DC-DC EVR) | 129 | mA | DC-DC EVR 활성 |
| 전체 합산 전류 | 200 | mA | L/LC/LP, 실제 전력, OSC/EVR/PFlash 포함 |
| 전체 합산 전류 | 221 | mA | LA/LX, 실제 전력 |
| STANDBY 전류 | 650 | uA | 대기 RAM 활성, TJ=25 C |
| SLEEP 전류 | 10 | mA | CPU idle, 주변장치 sleep, fSRI/SPB=1MHz, TJ=55 C |
| 최대 소비 전력 | 460 | mW | L/LC/LP, 최대 전력 패턴 |
| 최대 소비 전력 | 360 | mW | L/LC/LP, 실제 전력 패턴 |
| 최대 소비 전력 | 490 | mW | LA/LX, 최대 전력 패턴 |
| 최대 소비 전력 | 390 | mW | LA/LX, 실제 전력 패턴 |

**실제 전력 패턴(Real Power Pattern) 측정 조건**:
- TJ = 150 C, fSRI = fCPU0 = 200 MHz, fSPB = fSTM = fGTM = 40 MHz
- VDD = 1.326 V, VDDP3 = 3.366 V, VDDM = 5.1 V
- HSM, Ethernet, MTU 비활성

**최대 전력 패턴(Max Power Pattern) 측정 조건**:
- TJ = 150 C, fSRI = fCPU0 = 200 MHz, fSPB = fSTM = fGTM = 100 MHz
- VDD = 1.43 V, VDDP3 = 3.63 V, VDDM = 5.5 V
- 모든 주변장치 활성

### 4.4 3.3V 패드 특성

#### 공통 패드 파라미터

| 파라미터 | 값 | 단위 |
|----------|-----|------|
| 핀 용량 (디지털 I/O) | 6 (typ), 10 (max) | pF |
| PORST 스파이크 필터 차단 폭 | 80 (max) | ns |
| PORST 스파이크 필터 통과 폭 | 220 (min) | ns |
| PORST 패드 출력 전류 | 10.1 (min) | mA |

#### Class A1 패드

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 입력 주파수 최대 | 100 | MHz | - |
| 입력 히스테리시스 | 0.1 * VDDP3 | V | - |
| 입력 High 전압 (VIH) | 0.7 * VDDP3 | V | CMOS |
| 입력 Low 전압 (VIL) | 0.3 * VDDP3 | V | CMOS |
| 입력 누설 전류 | -400 ~ +400 (max) | nA | 0.1*VDDP3 < VIN < 0.9*VDDP3 |
| 풀다운 전류 | 15 ~ 120 | uA | - |
| 풀업 전류 | 15 ~ 120 | uA | - |
| 중간 드라이버 온저항 | 125 (typ), 200 (max) | Ohm | IOH=2mA, IOL=2mA |
| 약 드라이버 온저항 | 500 (typ), 800 (max) | Ohm | IOH=0.5mA, IOL=0.5mA |
| Rise/Fall 시간 (중간) | max 10 + 0.4*CL | ns | CL[pF], 10%~90% |
| Rise/Fall 시간 (약) | max 30 + 2.0*CL | ns | CL[pF], 10%~90% |

#### Class A1+ 패드

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 입력 주파수 최대 | 75 | MHz | - |
| 입력 히스테리시스 | 0.1 * VDDP3 | V | - |
| 입력 High 전압 (VIH) | 0.7 * VDDP3 | V | CMOS |
| 입력 Low 전압 (VIL) | 0.3 * VDDP3 | V | CMOS |
| 입력 누설 전류 | -1 ~ +1 (max) | uA | 0.1*VDDP3 < VIN < 0.9*VDDP3 |
| 강 드라이버 온저항 | 40 (typ), 65 (max) | Ohm | IOH=6mA, IOL=6mA |
| 중간 드라이버 온저항 | 125 (typ), 200 (max) | Ohm | IOH=2mA, IOL=2mA |
| 약 드라이버 온저항 | 500 (typ), 800 (max) | Ohm | IOH=0.5mA, IOL=0.5mA |
| Rise/Fall (강, slow edge) | max 8 + 0.14*CL | ns | - |
| Rise/Fall (강, soft edge) | max 1 + 0.14*CL | ns | - |
| Rise/Fall (중간) | max 10 + 0.4*CL | ns | - |
| Rise/Fall (약) | max 30 + 2.0*CL | ns | - |

#### Class S 패드 (ADC 디지털 입력)

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 입력 주파수 최대 | 75 | MHz | - |
| 입력 히스테리시스 | 0.3 (min) | V | - |
| 입력 High 전압 (VIH) | max 3.8 | V | - |
| 입력 Low 전압 (VIL) | min 1.39 | V | - |
| 입력 용량 | max 10 | pF | - |

#### Class I 패드 (입력 전용)

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 입력 주파수 최대 | 100 | MHz | - |
| 입력 High 전압 (VIH) | 0.7 * VDDP3 | V | CMOS |
| 입력 Low 전압 (VIL) | 0.3 * VDDP3 | V | CMOS |
| 입력 누설 전류 | -150 ~ +150 | nA | 0.1*VDDP3 < VIN < 0.9*VDDP3 |

---

## 5. 클럭 사양

### 5.1 시스템 클럭 주파수

| 클럭 도메인 | 최대 주파수 | 비고 |
|-------------|-------------|------|
| fSRI (SRI 버스) | 200 MHz | TC233LC-24F133F: 133 MHz |
| fCPU0 (CPU0) | 200 MHz | TC233LC-24F133F: 133 MHz |
| fMAX (최대 시스템) | 200 MHz | TC233LC-24F133F: 133 MHz |
| fPLL (PLL 출력) | 20 ~ 200 MHz | TC233LC-24F133F: 133 MHz |
| fPLL_ERAY (ERAY PLL) | 20 ~ 160 MHz | TC233LC-24F133F: 80 MHz |
| fSPB (시스템 주변 버스) | 100 MHz | - |
| fGTM (GTM 타이머) | 100 MHz | - |
| fSTM (시스템 타이머) | 100 MHz | - |
| fCAN (MultiCAN) | 100 MHz | - |
| fERAY (FlexRay) | 80 MHz | - |
| fBBB (BBB 버스) | 100 MHz | - |
| fASCLINF (ASCLIN 고속) | 200 MHz | TC233LC-24F133F: 133 MHz |
| fASCLINS (ASCLIN 저속) | 100 MHz | - |
| fFSI (Flash) | 100 MHz | - |
| fFSI2 (Flash2) | 200 MHz | TC233LC-24F133F: 133 MHz |
| fBAUD2 (Baud2) | 200 MHz | TC233LC-24F133F: 133 MHz |

### 5.2 오실레이터 (OSC_XTAL)

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 오실레이터 주파수 (크리스탈) | 8 ~ 40 | MHz | External Crystal Mode |
| 직접 입력 모드 | 4 ~ 40 | MHz | Direct Input Mode |
| 시작 시간 | max 5 | ms | VDDP3=3.13V 기준, 주파수에 따라 감소 |
| 입력 전류 (XTAL1) | -25 ~ +25 | uA | VIN > 0V |
| 입력 진폭 (>25MHz, 셰이퍼 미bypass) | min 0.3*VDDP3, max VDDP3+1.0 | V | - |
| 입력 진폭 (<=25MHz, 셰이퍼 미bypass) | min 0.4*VDDP3, max VDDP3+1.0 | V | - |
| Bypass 시 VIH | min 0.8 | V | - |
| Bypass 시 VIL | max 0.4 | V | - |

> 최종 타겟 시스템에서 반드시 오실레이션 허용량(음성 저항) 측정 권장.

### 5.3 백업 클럭

| 파라미터 | 최소 | 표준 | 최대 | 단위 |
|----------|------|------|------|------|
| 트리밍 전 | 75 | 100 | 125 | MHz |
| 트리밍 후 | 97.5 | 100 | 102.5 | MHz |

### 5.4 System PLL

| 파라미터 | 최소 | 표준 | 최대 | 단위 |
|----------|------|------|------|------|
| PLL Base 주파수 | 80 | 150 | 360 | MHz |
| VCO 주파수 범위 | 400 | - | 800 | MHz |
| VCO 입력 주파수 (fREF) | 8 | - | 24 | MHz |
| 변조 진폭 (MA) | 0 | - | 2 | % |
| 피크 주기 지터 | -200 | - | 200 | ps |
| PLL Lock-in 시간 | 11.5 | - | 200 | us |
| 변조 변동 주파수 | - | - | 5.4 | MHz |

> PLL 지터 사양은 핀당 용량 부하 CL = 20 pF, 최대 드라이버, soft edge 조건에서 유효.
> 전원 잡음 제한: 300 KHz 미만 Vpp < 100 mV, 300 KHz 초과 Vpp < 40 mV.

### 5.5 ERAY PLL

| 파라미터 | 최소 | 표준 | 최대 | 단위 |
|----------|------|------|------|------|
| PLL Base 주파수 | 50 | 200 | 320 | MHz |
| VCO 주파수 범위 | 400 | - | 480 | MHz |
| VCO 입력 주파수 (fREF) | 16 | - | 24 | MHz |
| 누적 지터 (DP) | -0.5 | - | 0.5 | ns |
| SYSCLK 핀 누적 지터 (DPP) | -0.8 | - | 0.8 | ns |
| PLL Lock-in 시간 | 5.6 | - | 200 | us |

---

## 6. 주변장치별 사양 요약

### 6.1 통신 인터페이스

| 주변장치 | 수량 | 주요 사양 |
|----------|------|-----------|
| ASCLIN (비동기/동기 직렬) | 2채널 | LIN V1.3/V2.0/V2.1/J2602 지원, 최대 50 MBaud |
| QSPI (큐 SPI) | 4채널 | 마스터/슬레이브, 최대 50 Mbit/s |
| MultiCAN+ | 2모듈 | 각 모듈 3 CAN 노드, 총 6 CAN 노드 |
| CAN 메시지 객체 | 128개 | 자유 할당 가능, FIFO/게이트웨이 지원 |
| CAN FD | 지원 | - |
| SENT | 4채널 | SAE J2716 센서 연결용 |
| FlexRay (E-Ray) | 1모듈, 2채널 | V2.1 지원 |
| Ethernet MAC | 1채널 | IEEE802.3, RMII/MII, TC237만 해당 (TC233/234: 0채널) |
| HSIC | 2채널 | - |

### 6.2 타이머

| 주변장치 | 수량 | 주요 사양 |
|----------|------|-----------|
| GTM (Generic Timer Module) | 1 | TIM: 12+12채널, TOM: 2, DTM: 2, CMU/ICM: 1, TBU: 1 |
| CCU6 (Capture/Compare 6) | 2커널 | CCU60, CCU61 |
| GPT12 (General Purpose Timer) | 1 | GPT120 |
| STM (System Timer) | 1 | - |

### 6.3 ADC (VADC)

| 파라미터 | 값 | 비고 |
|----------|-----|------|
| ADC 커널 수 | 4개 (독립 클러스터) | - |
| 아날로그 입력 범위 | VAGND ~ VAREF | VAREF = VDDM 기준 |
| ADC 공급 전압 (VDDM) | 2.97V ~ 5.5V | 5V 및 3.3V 모드 지원 |
| 해상도 | 12비트 / 10비트 / 8비트 | Fast Compare 모드 포함 |
| 컨버터 기준 클럭 (fADCI) | max 50 MHz (5V), max 35 MHz (3.3V) | - |
| 12비트 변환 시간 | (16+STC)*tADCI + 2*tVADC | STC: 샘플 시간 설정값 |
| 10비트 변환 시간 | (14+STC)*tADCI + 2*tVADC | - |
| 8비트 변환 시간 | (12+STC)*tADCI + 2*tVADC | - |
| Fast Compare 변환 시간 | (4+STC)*tADCI + 2*tVADC | - |

#### VADC 정밀도 (5V 모드, VDDM = 4.5~5.5V)

| 파라미터 | 최소 | 최대 | 단위 | 비고 |
|----------|------|------|------|------|
| TUE (Total Unadjusted Error) | -4 | +4 | LSB | 12비트 해상도 |
| INL | -3 | +3 | LSB | 12비트 해상도 |
| DNL | -3 | +3 | LSB | 12비트 해상도 |
| Gain Error | -3.5 | +3.5 | LSB | 12비트 해상도 |
| Offset Error | -4 | +4 | LSB | 12비트 해상도 |
| RMS Noise | typ 0.5, max 0.8 | LSB | 1-sigma 가우스 분포 |
| 입력 총 커패시턴스 (CAINT) | max 30 | pF | - |
| 스위칭 커패시턴스 (CAINS) | 2 ~ 7 | pF | - |
| 입력 경로 저항 (RAIN) | max 1.5 | kOhm | - |
| 기준 입력 스위칭 커패시턴스 | max 30 | pF | - |
| CSD 저항 (RCSD) | max 28 | kOhm | 샘플 시간 > 5*RCSD*CAINS 권장 |

#### VADC 정밀도 (3.3V 모드, VDDM = 2.97~4.5V)

| 파라미터 | 최소 | 최대 | 단위 | 비고 |
|----------|------|------|------|------|
| TUE | -6 / -12 | +6 / +12 | LSB | 12비트, TJ<=150 C / TJ>150 C |
| INL | -5 / -12 | +5 / +12 | LSB | 12비트 |
| Gain Error | -5.5 / -6 | +5.5 / +6 | LSB | - |
| Offset Error | -5 / -6 | +5 / +6 | LSB | - |
| DNL | -4 / -4 | +4 / +4 | LSB | - |
| RMS Noise | max 1.7 | LSB | - |
| 입력 경로 저항 (RAIN) | max 4.5 | kOhm | 3.3V 모드에서 증가 |
| 기준 입력 경로 저항 | max 3 | kOhm | - |

> 기준 전압이 k배(k<1) 축소되면 TUE/DNL/INL/Gain/Offset 오차가 1/k배 증가.
> VAREF는 반드시 외부 디커플링 캐패시터로 안정화 필요.

### 6.4 온도 센서 (DTS)

| 파라미터 | 값 | 단위 |
|----------|-----|------|
| 측정 시간 | max 100 | us |
| 캘리브레이션 정확도 | +/-1 | C |
| 비선형 정확도 | +/-2 | C |
| 측정 범위 | -40 ~ 170 | C |
| 리셋 후 시작 시간 | max 20 | us |

온도 계산 공식: `Tj = (DTSSTAT.RESULT - 607) / 2.13`

### 6.5 기타 주변장치

| 주변장치 | 설명 |
|----------|------|
| DMA | 16채널, 안전 데이터 전송 |
| SMU (Safety Management Unit) | 1개, 안전 모니터 알람 처리 |
| IOM (Hardware I/O Monitor) | 1개, 디지털 I/O 모니터링 |
| MTU (Memory Test Unit) | ECC, 메모리 초기화, MBIST |
| HSM (Hardware Security Module) | 일부 변형에 선택적 탑재 |
| EVR (Embedded Voltage Regulator) | DCDC(3.3V->1.3V) 및 LDO 지원 |
| 버스 구조 | 64비트 SRI 크로스바 + 32비트 SPB + SFI 브리지 |

---

## 7. 내장 전압 레귤레이터 (EVR)

### 7.1 LDO 모드

| 파라미터 | 최소 | 표준 | 최대 | 단위 | 비고 |
|----------|------|------|------|------|------|
| 입력 전압 (VDDP3) | 2.97 | - | 3.63 | V | - |
| 출력 전압 범위 (VDD) | 1.17 | 1.3 | 1.43 | V | - |
| 트리밍 후 정밀 정확도 | 1.275 | 1.3 | 1.325 | V | IDD max power pattern 부하 |
| 출력 버퍼 커패시턴스 | 1.4 | 2.2 | 3.0 | uF | IDD < 230mA 제한 |
| VDD 1차 저전압 리셋 임계값 | - | - | 1.17 | V | - |
| 시작 시간 | - | - | 1000 | us | - |
| 입력 공급 램프 | - | 1 | 50 | V/ms | - |
| 부하 스텝 응답 | - | - | 100 | mV | dI=-100mA, Tsettle=20us |

### 7.2 SMPS (스위칭 레귤레이터) 모드

| 파라미터 | 최소 | 표준 | 최대 | 단위 | 비고 |
|----------|------|------|------|------|------|
| 출력 전압 범위 | 1.17 | 1.3 | 1.43 | V | VDDP3 > 2.97V, IDDDC < 200mA |
| 트리밍 후 정밀 정확도 | 1.275 | 1.3 | 1.325 | V | fDCDC = 1MHz |
| 스위칭 주파수 | 0.4 | - | 2.0 | MHz | - |
| 최대 리플 (peak-to-peak) | - | - | 26 | mV | IDDDC < 230mA, fDCDC = 1MHz |
| 최대 출력 전류 | - | - | 250 | mA | VDDP3 > 2.97V, VDD = 1.17V |
| 부하 과도 응답 | - | - | 90 | mV | dI < 100mA, fDCDC=1MHz |
| 효율 | - | 72 | - | % | VIN=3.3V, IDDDC=200mA |

> fSRI 최대 주파수 동작 시 VDD 동작 범위 1.235V ~ 1.430V로 제한.
> DCDC 공칭 전압 1.33V +/- 7.5%로 설정 가능.

### 7.3 SMPS 외부 부품 요구사항

| 부품 | 최소 | 표준 | 최대 | 단위 | 비고 |
|------|------|------|------|------|------|
| 출력 커패시터 (COUTDC) | 6.5 | 10 | 13.5 | uF | IDDDC=230mA, +/-35% 공차 포함 |
| 출력 커패시터 ESR | - | - | 50 | mOhm | 0.5~10MHz |
| 입력 커패시터 (CIN) | 3.29 | 4.7 | 6.11 | uF | IDDDC=230mA |
| 입력 커패시터 ESR | - | - | 100 | mOhm | 0.5~10MHz |
| 플라잉 커패시터 (CFLY) | 0.7 | 1 | 1.3 | uF | 핀 가까이 배치, 비아 없이 |
| 플라잉 커패시터 ESR | - | - | 50 | mOhm | 0.5~10MHz |

> 커패시터 min-max 범위는 DC 바이어스 효과 포함 +/-35% 공차.
> 커패시터~공급/GND 레일 트레이스 저항 25 mOhm 이하 제한.

### 7.4 전원 모니터링

| 파라미터 | 최소 | 표준 | 최대 | 단위 | 비고 |
|----------|------|------|------|------|------|
| VDDP3 1차 저전압 임계값 | 2.86 | 2.92 | 2.97 | V | 개별 +/-1% 생산 테스트 |
| VDD 1차 저전압 임계값 | 1.13 | 1.15 | 1.17 | V | 개별 +/-1% 생산 테스트 |
| VDDP3 2차 모니터 정확도 | 3.23 | 3.30 | 3.37 | V | 임계값=3.3V=0x91h |
| VDD 2차 모니터 정확도 | 1.27 | 1.30 | 1.33 | V | 임계값=1.3V=0xE4h |
| 모니터 측정 지연 | - | - | 1.8 | us | 새 공급값 반영까지 |

---

## 8. 리셋 타이밍

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| Application Reset 부트 시간 | max 350 | us | 최대 주파수 동작 시 |
| System Reset 부트 시간 | max 1 | ms | - |
| Power-on Reset 부트 시간 | max 2.5 | ms | dV/dT=1V/ms, EVR 램프업+FW 포함 |
| 펌웨어 실행 시간 (외부 공급만) | max 1.1 | ms | EVR 미사용 시 |
| EVR 시작/램프업 시간 | max 1 | ms | dV/dT=1V/ms |
| PORST 최소 활성 유지 시간 | min 1 | ms | 전원 안정 후 |
| HWCFG 핀 hold 시간 | min 16/fSPB | ns | ESR0 상승 에지 기준 |
| HWCFG 핀 setup 시간 | min 0 | ns | ESR0 상승 에지 기준 |
| PORST 후 포트 비활성 시간 | max 150 | ns | 아날로그 스파이크 필터 지연 포함 |
| PORST rising edge hold 시간 | min 150 | ns | - |

**전원 시퀀스 (Single Supply, 3.3V)**:
1. T0: VDDP3 램프업 시작
2. T1: 기본 공급/클럭 인프라 가용, HWCFG[0,2] 핀으로 공급 모드 판별, EVR13 소프트 스타트 개시
3. T2: 모든 공급 전압이 1차 리셋 임계값 초과, PORST(output) 해제, HWCFG[3:5] 래치, 펌웨어 실행 개시
4. T3: 펌웨어 완료, 사용자 코드 시작 (기본 fCPU = 100 MHz)
5. T4: 램프다운 시 1차 저전압 리셋 임계값 하회 시 PORST 재발생

**전원 시퀀스 (External Supply, 3.3V + 1.3V)**:
- VDDP3와 VDD 독립 램프업/다운 가능
- 전류 증가율 제한: max 50 mA/100 us
- 잔류 전압 0~1V에서의 램프업에도 안정 동작

---

## 9. Flash 파라미터

> PFlash 프로그램/삭제는 TJ <= 150 C에서만 허용.

### 9.1 Program Flash (PFlash)

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 섹터 삭제 시간 | max 1 | s | cycle count < 1000 |
| 멀티 섹터 삭제 시간 | 0.207 + 0.003*S[KB]/fFSI[MHz] | s | 연속 논리 섹터, S = 총 크기 |
| 페이지 프로그램 시간 (3.3V) | max 81 + 3400/fFSI[MHz] | us | 32 Byte 페이지 |
| 버스트 프로그램 시간 (3.3V) | max 410 + 12000/fFSI[MHz] | us | 256 Byte |
| 1MB 버스트 프로그램 시간 | max 2.2 | s | 통신 오버헤드 제외 |
| Write Page Once 추가 시간 | max 15 + 500/fFSI[MHz] | us | - |
| Suspend-to-Read 지연 | max 12000/fFSI[MHz] | us | - |
| 액세스 지연 (tPF) | max 30 | ns | PMU_FCON.WSPFLASH 참조 |
| ECC 지연 (tPFECC) | max 10 | ns | PMU_FCON.WSECPF 참조 |
| 데이터 보존 시간 | 20 | years | max 1000 P/E 사이클 |
| PFlash P/E 온도 제한 | 150 | C | 접합 온도 기준 |

### 9.2 Data Flash (DFlash)

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 섹터 삭제 시간 (< 1000 cycles) | typ 0.12+0.08/fFSI | s | - |
| 섹터 삭제 시간 (< 125000 cycles) | typ 0.57+0.15/fFSI, max 0.928+0.15/fFSI | s | - |
| 멀티 섹터 삭제 시간 | 0.12+0.01*S[KB]/fFSI (< 1000 cycles) | s | - |
| 페이지 프로그램 시간 | max 50+2500/fFSI[MHz] | us | 8 Byte |
| 버스트 프로그램 시간 | max 96+4400/fFSI[MHz] | us | 32 Byte |
| Suspend-to-Read 지연 | max 12000/fFSI[MHz] | us | - |
| 액세스 지연 (tDF) | max 100 | ns | PMU_FCON.WSDFLASH 참조 |
| ECC 지연 (tDFECC) | max 20 | ns | PMU_FCON.WSECDF 참조 |
| EEPROM 내구성 (NE_EEP10) | 125,000 | cycles | 10년 데이터 보존 기준 |
| DF0 수명 내 삭제 횟수 (NERD0) | 750,000 | cycles | - |
| 삭제 교란 한계 (NDFD) | 50 | cycles | - |
| 마진 변경 후 대기 시간 | max 10 | us | - |

### 9.3 UCB (User Configuration Block)

| 파라미터 | 값 | 비고 |
|----------|-----|------|
| UCB 보존 시간 | 20년 | max 100회/UCB, 총 400회 P/E 사이클 |

---

## 10. 디버그 인터페이스

### 10.1 JTAG (IEEE 1149.1-2000)

| 파라미터 | 최소 | 최대 | 단위 |
|----------|------|------|------|
| TCK 클럭 주기 (t1) | 25 | - | ns |
| TCK High 시간 (t2) | 10 | - | ns |
| TCK Low 시간 (t3) | 10 | - | ns |
| TCK Rise/Fall 시간 | - | 4 | ns |
| TDI/TMS setup 시간 (t6) | 6 | - | ns |
| TDI/TMS hold 시간 | 16 | - | ns |

### 10.2 DAP (Device Access Port)

- JTAG 4/5선 또는 DAP 인터페이스 선택 가능
- OCDS Level 1 디버그 지원 (CPU, DMA, 온칩 버스)
- DAP0 = TCK, DAP1 = TMS

---

## 11. 통신 인터페이스 AC 특성

### 11.1 ASCLIN SPI 마스터 타이밍

- ASCLIN 모듈의 SPI 마스터 모드 타이밍은 데이터시트 Section 3.21에 상세 기재

### 11.2 QSPI 타이밍

- 마스터/슬레이브 모드 지원
- 최대 50 Mbit/s
- ECON 레지스터의 CPH, CPOL 설정으로 클럭 위상/극성 제어
- A1+ 강 드라이버 사용 시 최적 타이밍

### 11.3 Ethernet (ETH)

#### MDC/MDIO 관리 신호

| 파라미터 | 최소 | 최대 | 단위 | 비고 |
|----------|------|------|------|------|
| MDC 주기 | 400 | - | ns | CL=25pF |
| MDC High 시간 | 160 | - | ns | - |
| MDC Low 시간 | 160 | - | ns | - |
| MDIO setup (출력) | 10 | - | ns | - |
| MDIO hold (출력) | 10 | - | ns | - |
| MDIO data valid (입력) | 0 | 300 | ns | - |

#### MII (Media Independent Interface)

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 클럭 주기 | 40ns (100Mbps) / 400ns (10Mbps) | ns | CL=25pF |
| 클럭 High/Low 시간 | 14~26ns (100M) / 140~260ns (10M) | ns | 35%~65% 듀티 |
| 입력 setup 시간 | min 10 | ns | - |
| 입력 hold 시간 | min 10 | ns | - |
| 출력 valid 시간 | max 25 | ns | - |

#### RMII (Reduced MII)

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| REF_CLK 주기 | 20 | ns | 50MHz, +/-50ppm |
| REF_CLK High/Low | 7~13 | ns | 35%~65% 듀티 |
| Setup 시간 | min 4 | ns | - |
| Hold 시간 | min 2 | ns | - |

### 11.4 E-Ray (FlexRay)

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| TxEN Rise/Fall 시간 | max 9 | ns | CL=25pF |
| TxD Rise+Fall 합산 | max 9 | ns | 20%~80%, CL=25pF |
| Tx 지연 (TxEN/TxD 01/10) | max 25 | ns | - |
| 송신 비대칭 | +/-2.45 | ns | CL=25pF |
| 수신 비대칭 허용 | -30.5 ~ +43.0 | ns | CL=25pF |
| 논리 High 감지 임계값 | 35 ~ 70 | % | - |
| 논리 Low 감지 임계값 | 30 ~ 65 | % | - |
| Rx 지연 (RxD 01/10) | max 10 | ns | - |

---

## 12. 품질 파라미터

| 파라미터 | 값 | 단위 | 비고 |
|----------|-----|------|------|
| 동작 수명 | max 24,500 | 시간 | - |
| ESD (HBM) | 2,000 | V | JESD22-A114-B |
| ESD (CDM) 일반 핀 | 500 | V | JESD22-C101-C |
| ESD (CDM) 코너 핀 | 750 | V | JESD22-C101-C |
| MSL (내습 민감도) | Level 3 | - | 240 C, J-STD-020C |

---

## 13. 펌웨어 개발자를 위한 핵심 수치 요약

| 항목 | 값 |
|------|-----|
| CPU 코어 | TriCore TC1.6E x 1 (+ Lockstep 체커) |
| CPU 최대 클럭 | 200 MHz |
| SPB (주변장치 버스) 최대 클럭 | 100 MHz |
| 외부 크리스탈 범위 | 8 ~ 40 MHz |
| PFLASH 크기 | 최대 2 MB |
| DFLASH 크기 | 최대 128 KB |
| DSPR 크기 | 최대 184 KB |
| PSPR 크기 | 최대 8 KB |
| LMU SRAM | 32 KB |
| CAN 노드 수 | 6 (2모듈 x 3노드), 128 메시지 객체, CAN FD 지원 |
| ASCLIN 채널 수 | 2 (UART/LIN/SPI, 최대 50 MBaud) |
| QSPI 채널 수 | 4 (최대 50 Mbit/s) |
| SENT 채널 수 | 4 |
| FlexRay 채널 수 | 2 (1모듈, V2.1) |
| Ethernet | 1 (TC237만, MII/RMII, 10/100 Mbps) |
| ADC 커널 수 | 4 (독립) |
| ADC 해상도 | 12/10/8비트 |
| ADC 입력 전압 범위 | 0 ~ VAREF (VDDM 최대 5.5V) |
| ADC 5V 모드 TUE | +/-4 LSB (12비트) |
| ADC 3.3V 모드 TUE | +/-6 LSB (12비트, TJ<=150C) |
| DMA 채널 수 | 16 |
| GTM TIM 채널 | 12+12 |
| GTM TOM 모듈 | 2 |
| CCU6 커널 | 2 (CCU60, CCU61) |
| GPT12 | 1 (GPT120) |
| STM | 1 |
| VDD (코어) | 1.17 ~ 1.43V (typ 1.3V) |
| VDDP3 (I/O) | 2.97 ~ 3.63V (typ 3.3V) |
| VDDM (ADC) | 2.97 ~ 5.5V (typ 5.0V) |
| 동작 온도 (SAK) | -40 ~ +125 C |
| 동작 온도 (SAL) | -40 ~ +150 C |
| ASIL 등급 | 최대 ASIL-D |
| Standby 전류 | max 650 uA (TJ=25 C) |
| Sleep 전류 | max 10 mA (fSRI/SPB=1MHz) |
| 최대 소비 전력 (실제 패턴) | 360 mW (L/LC/LP), 390 mW (LA/LX) |
| PFlash 데이터 보존 | 20년 (1000 P/E 사이클) |
| DFlash EEPROM 내구성 | 125,000 사이클 (10년 보존) |
| POR 부트 시간 | max 2.5 ms |

---

*이 문서는 Infineon TC233/TC234/TC237 Data Sheet V1.1 (2015-06)을 기반으로 작성되었습니다.*
*정밀 설계 시 반드시 원본 데이터시트를 참조하십시오. 수치 값은 동작 조건(Ta, Vdd, TJ 등)에 따라 달라집니다.*
