"""
HC-12 AT Command Setup Tool
────────────────────────────
HC-12 모듈의 SET 핀을 GND에 연결한 상태에서 실행.
AT 모드는 항상 9600 baud로 통신.

Usage:
    python hc12_setup.py           # COM 포트 자동 검색
    python hc12_setup.py COM13     # 포트 지정
"""

import sys
import time

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("pyserial required: pip install pyserial")
    sys.exit(1)

AT_BAUD = 9600  # AT 모드는 항상 9600


def find_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if any(k in p.description for k in ("CP210", "Silicon", "CH340", "PL2303", "FTDI")):
            return p.device
    if ports:
        return ports[0].device
    return None


def send_at(ser, cmd, wait=1.0):
    ser.reset_input_buffer()
    ser.write((cmd + "\r\n").encode())
    time.sleep(wait)
    resp = ser.read(ser.in_waiting or 64).decode("ascii", errors="ignore").strip()
    return resp


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    if not port:
        print("COM port not found!")
        return

    print(f"Opening {port} @ {AT_BAUD} (AT mode)")
    ser = serial.Serial(port, AT_BAUD, timeout=1)
    time.sleep(0.5)

    print("\n=== HC-12 AT Command Tool ===")
    print("SET pin must be connected to GND!")
    print("Type AT commands, or 'q' to quit.\n")

    # Test connection
    resp = send_at(ser, "AT")
    print(f">> AT")
    print(f"<< {resp}\n")

    if "OK" not in resp:
        print("No response! Check:")
        print("  1. SET pin -> GND connected?")
        print("  2. Correct COM port?")
        print("  3. Power on?")
        ser.close()
        return

    # Show current settings
    print("--- Current Settings ---")
    for cmd in ["AT+RX", "AT+V"]:
        resp = send_at(ser, cmd)
        print(f">> {cmd}")
        print(f"<< {resp}")
    print()

    # Interactive mode
    print("--- Commands ---")
    print("  AT+B38400   : Set baud 38400")
    print("  AT+B9600    : Set baud 9600")
    print("  AT+B115200  : Set baud 115200")
    print("  AT+C001     : Set channel 001 (default)")
    print("  AT+P8       : Set power 8 (max)")
    print("  AT+RX       : Show all params")
    print("  AT+V        : Show version")
    print("  q           : Quit")
    print()

    while True:
        try:
            cmd = input(">> ").strip()
        except (EOFError, KeyboardInterrupt):
            break

        if not cmd:
            continue
        if cmd.lower() == 'q':
            break

        resp = send_at(ser, cmd)
        print(f"<< {resp}\n")

    ser.close()
    print("Done.")


if __name__ == "__main__":
    main()
