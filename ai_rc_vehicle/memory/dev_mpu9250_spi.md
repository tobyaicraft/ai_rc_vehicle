# MPU-9250 SPI 통신 개발 이력

## 개발 완료 항목

### 1. SPI 드라이버 (DrvSpi.c/h)
- **QSPI3 Master** 모드, 1 MHz, SPI Mode 0 (CPOL=0, CPHA=0)
- 핀: SCLK=P22.3, MOSI=P22.0, MISO=P22.1, CS=P22.2 (GPIO 수동)
- ISR 우선순위: TX=6, RX=7, ERR=8
- `ReadReg`, `WriteReg`, `ReadBurst` 함수 구현
- **QSPI3 FIFO 한계**: 14바이트 버스트 불가 → 2바이트씩 6회 분할 읽기로 해결

### 2. MPU-9250 드라이버 (DrvMpu9250.c/h)
- **writeRegVerify()**: 레지스터 쓰기 후 읽기 검증 (최대 5회 재시도, 5ms 간격)
  - 리셋 직후 SPI 쓰기가 무시되는 문제 해결
- 초기화 시퀀스: 리셋(200ms) → Wake(50ms) → SPI모드(10ms) → 축활성화 → 센서설정
- 센서 설정: 자이로 ±500°/s, 가속도 ±2g (AC 레지스터 쓰기 실패로 기본값 사용), DLPF 41Hz, 100Hz 샘플레이트
- WHO_AM_I: 0x70 (MPU-6500 호환 칩)

### 3. 축 매핑 (실측 기반)
센서 보드 기준: **Y축 = 비행기 앞(코, VCC쪽)**, **X축 = 날개**, **Z축 = 위**

| 물리 동작 | 가속도 | 자이로 | 부호 |
|-----------|--------|--------|------|
| Roll (날개 기울임) | AX, AZ | GY (raw[1]) | 반전 (-) |
| Pitch (기수 상하) | AY, AZ | GX (raw[0]) | 반전 (-) |
| Yaw (좌우 회전) | - | GZ (raw[2]) | 정방향 |

### 4. Complementary Filter
- α=0.96, DT=0.01s (100Hz)
- Roll/Pitch: 자이로 96% + 가속도 4% 가중
- Yaw: 자이로 적분 only (가속도 보정 불가)

### 5. 캘리브레이션
- 시작 후 1초간(100회) 정지 상태 평균 수집
- 가속도 Roll/Pitch 오프셋 보정 (초기 자세 = 0°)
- 자이로 GX/GY/GZ 바이어스 보정 (정지 시 0°/s)

### 6. Yaw 드리프트 억제
- 데드존: ±1.5°/s 이하 무시 (노이즈 제거)
- 감쇠: 정지 시 Yaw × 0.998 (서서히 0 복귀)

### 7. UART 출력
- 포맷: `R:+012.3,P:-005.7,Y:+045.2\r\n` (100Hz)
- AppTask_10ms에서 ReadSensors() + SendUart() 호출

### 8. 3D 뷰어 (Tool/imu_3d_viewer.py)
- matplotlib 실시간 3D 큐브 시각화
- 회전 행렬: Rz(Yaw) @ Ry(Roll) @ Rx(Pitch)
- 초록 화살표 = 전진 방향 (+Y)
- 큐브 형태: X=날개(짧게), Y=앞뒤(길게), Z=얇게

### 9. 설계 문서
- `docs/SPI_MPU9250_Design.html` — SPI 설계서 (워드 복붙용)
- Logic Analyzer 캡처 삽입 위치 표시됨

## 트러블슈팅 이력

| # | 현상 | 원인 | 해결 |
|---|------|------|------|
| 1 | AX만 정상, 나머지 5축=0 | QSPI3 RX FIFO 오버플로 | 2바이트씩 6회 분할 읽기 |
| 2 | 자이로 전축 0, AZ=0 | PWR_MGMT_1/2 쓰기 실패 (GYRO_STANDBY, DISABLE_ZA) | writeRegVerify() 도입 + 딜레이 증가 |
| 3 | Roll/Pitch 3D 방향 반대 | 센서축과 물리 장착 방향 불일치 | 자이로/가속도 부호 반전 (실측 매핑) |
| 4 | Yaw 누적 드리프트 | 자이로 적분 only | 데드존 ±1.5°/s + 감쇠 ×0.998 |

## 미완료 / 향후 개발

### RC 자동차 적용
- [ ] Yaw 기반 90° 정확한 회전 제어
- [ ] 180° U턴
- [ ] 직진 보정 (Yaw 드리프트 감지 → 조향 보정)
- [ ] 전복 감지 (Roll > 45° → 모터 정지)
- [ ] 경사 감지 (Pitch 기반 속도 조절)
- [ ] 드리프트 제어 (자이로 + 조향 피드백)
- [ ] 자율 주행 패턴 (사각형, 원형 경로)

### 센서 개선
- [ ] AK8963 자력계 활성화 (Yaw 드리프트 보정)
- [ ] ACCEL_CONFIG ±4g 쓰기 실패 원인 조사 (현재 ±2g 기본값 사용 중)
- [ ] 버스트 읽기 최적화 (QSPI0/1/2 사용 시 14바이트 한번에 가능할 수 있음)

### Logic Analyzer 캡처 완료된 항목
- [x] AX 버스트 읽기 (0xBB → 0x36, 0x68)
- [x] AX+AY 연속 분할 읽기 패턴
- [ ] WHO_AM_I 읽기 (MOSI=0xF5 → MISO=0x70)
- [ ] 레지스터 쓰기 (WriteReg 파형)

## 파일 목록
- `DrvSpi.c/h` — QSPI3 SPI Master 드라이버
- `DrvMpu9250.c/h` — MPU-9250 센서 드라이버 + 자세 계산
- `AppTask.c` — 10ms 주기 센서 읽기/UART 전송, 100ms WHO_AM_I 캡처용 (임시)
- `Tool/imu_3d_viewer.py` — 3D 시각화 Python 툴
- `docs/SPI_MPU9250_Design.html` — SPI 설계 문서
