"""
RC Vehicle 무선 컨트롤러 (HC-12 Serial)

방향키로 조종, 떼면 자동 정지.
숫자 1~9로 속도 조절.

사용법:
    python rc_controller.py COM포트번호
    예: python rc_controller.py COM3

조작:
    ↑  전진    ↓  후진
    ←  좌스핀  →  우스핀
    1~9  속도 (10%~90%)
    ESC  종료
"""

import sys
import serial
import time

# Windows 전용: msvcrt로 키 입력 감지
import msvcrt

def get_key():
    """비차단 키 입력. 방향키는 2바이트(0xE0 + 코드)."""
    if not msvcrt.kbhit():
        return None

    ch = msvcrt.getch()

    # 방향키: 0xE0 또는 0x00 prefix
    if ch in (b'\xe0', b'\x00'):
        ch2 = msvcrt.getch()
        code = ch2[0]
        if code == 72: return 'UP'
        if code == 80: return 'DOWN'
        if code == 75: return 'LEFT'
        if code == 77: return 'RIGHT'
        return None

    val = ch[0]

    # ESC
    if val == 27:
        return 'ESC'

    # 숫자
    if 0x30 <= val <= 0x39:
        return chr(val)

    return None


def main():
    if len(sys.argv) < 2:
        print("사용법: python rc_controller.py COM포트")
        print("예: python rc_controller.py COM3")
        sys.exit(1)

    port = sys.argv[1]

    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
    except serial.SerialException as e:
        print(f"포트 열기 실패: {e}")
        sys.exit(1)

    print(f"=== RC Vehicle Controller ({port}) ===")
    print("  ↑↓←→ : 전진/후진/좌스핀/우스핀")
    print("  1~9  : 속도 (10%~90%)")
    print("  ESC  : 종료")
    print("=" * 40)

    # 방향키 → ESC 시퀀스 매핑 (터미널 표준)
    arrow_map = {
        'UP':    b'\x1b[A',
        'DOWN':  b'\x1b[B',
        'RIGHT': b'\x1b[C',
        'LEFT':  b'\x1b[D',
    }

    cmd_name = {
        'UP': '전진', 'DOWN': '후진',
        'LEFT': '좌스핀', 'RIGHT': '우스핀',
    }

    last_cmd = None
    last_send_time = 0
    REPEAT_INTERVAL = 0.05  # 50ms 간격 반복 전송

    try:
        while True:
            key = get_key()

            if key == 'ESC':
                print("\n종료합니다.")
                break

            now = time.time()

            if key in arrow_map:
                ser.write(arrow_map[key])
                last_cmd = key
                last_send_time = now
                print(f"\r  [{cmd_name[key]}]     ", end='', flush=True)

            elif key and key.isdigit():
                ser.write(key.encode())
                print(f"\r  [속도: {int(key)*10}%]   ", end='', flush=True)

            elif key is None and last_cmd is not None:
                # 키를 계속 누르고 있으면 반복 전송
                if msvcrt.kbhit():
                    continue
                # 키를 뗀 상태 → 반복 전송 중단
                if (now - last_send_time) > 0.15:
                    last_cmd = None
                    print(f"\r  [정지]        ", end='', flush=True)

            # 키 누른 상태면 반복 전송 (50ms 간격)
            if last_cmd and (now - last_send_time) >= REPEAT_INTERVAL:
                # 아직 키가 눌려있는지는 다음 루프에서 확인
                pass

            time.sleep(0.01)  # CPU 부하 방지

    except KeyboardInterrupt:
        print("\n종료합니다.")
    finally:
        ser.close()


if __name__ == '__main__':
    main()
