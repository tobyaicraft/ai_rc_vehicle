"""
MPU-9250 IMU 3D Viewer (HC-12 wireless)
TC237 UART(HC-12 38400bps)로 수신한 Roll/Pitch/Yaw를 실시간 3D 큐브로 시각화

수신 포맷: "R:+012.3,P:-005.7,Y:+045.2\r\n"

Usage:
    python imu_3d_viewer.py
    python imu_3d_viewer.py COM13

필요 패키지:
    pip install pyserial matplotlib numpy
"""

import sys
import serial
import serial.tools.list_ports
import threading
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import matplotlib.animation as animation

# ─────────────────────────────────────────────
# Configuration
# ─────────────────────────────────────────────
BAUDRATE = 38400

# ─────────────────────────────────────────────
# Global attitude data
# ─────────────────────────────────────────────
roll = 0.0
pitch = 0.0
yaw = 0.0
lock = threading.Lock()
connected = False


def parse_line(line: str):
    """Parse UART line: 'R:+012.3,P:-005.7,Y:+045.2'"""
    global roll, pitch, yaw
    try:
        parts = line.strip().split(",")
        if len(parts) != 3:
            return
        r = float(parts[0].split(":")[1])
        p = float(parts[1].split(":")[1])
        y = float(parts[2].split(":")[1])
        with lock:
            roll, pitch, yaw = r, p, y
    except (ValueError, IndexError):
        pass


def serial_thread(port: str):
    """Serial receive thread"""
    global connected
    try:
        ser = serial.Serial(port, BAUDRATE, timeout=1)
        connected = True
        print(f"[Serial] Connected to {port} @ {BAUDRATE}")
        while True:
            line = ser.readline().decode("ascii", errors="ignore").strip()
            if line:
                print(f"[RX] {line}")
            if line.startswith("R:"):
                parse_line(line)
    except serial.SerialException as e:
        print(f"[Serial] Error: {e}")
        connected = False


# ─────────────────────────────────────────────
# Rotation matrix
# ─────────────────────────────────────────────
def rotation_matrix(roll_deg, pitch_deg, yaw_deg):
    r = np.radians(roll_deg)
    p = np.radians(pitch_deg)
    y = np.radians(yaw_deg)

    Ry = np.array([
        [ np.cos(r), 0, np.sin(r)],
        [ 0,         1, 0],
        [-np.sin(r), 0, np.cos(r)],
    ])
    Rx = np.array([
        [1,  0,          0],
        [0,  np.cos(p), -np.sin(p)],
        [0,  np.sin(p),  np.cos(p)],
    ])
    Rz = np.array([
        [np.cos(y), -np.sin(y), 0],
        [np.sin(y),  np.cos(y), 0],
        [0,          0,         1],
    ])

    return Rz @ Ry @ Rx


# ─────────────────────────────────────────────
# 3D cube (board shape)
# ─────────────────────────────────────────────
CUBE_VERTICES = np.array([
    [-1.2, -2.5, -0.15],
    [ 1.2, -2.5, -0.15],
    [ 1.2,  2.5, -0.15],
    [-1.2,  2.5, -0.15],
    [-1.2, -2.5,  0.15],
    [ 1.2, -2.5,  0.15],
    [ 1.2,  2.5,  0.15],
    [-1.2,  2.5,  0.15],
])

CUBE_FACES = [
    [0, 1, 2, 3], [4, 5, 6, 7],
    [0, 1, 5, 4], [2, 3, 7, 6],
    [0, 3, 7, 4], [1, 2, 6, 5],
]

FACE_COLORS = [
    (0.6, 0.2, 0.8, 0.7), (0.8, 0.4, 0.0, 0.7),
    (0.8, 0.2, 0.2, 0.7), (0.2, 0.2, 0.8, 0.7),
    (0.8, 0.8, 0.2, 0.7), (0.2, 0.8, 0.2, 0.7),
]


def select_port():
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No COM ports found.")
        sys.exit(1)

    print("\n=== Available COM Ports ===")
    for i, p in enumerate(ports):
        print(f"  [{i + 1}] {p.device:8s} - {p.description}")
    print()

    if len(ports) == 1:
        print(f"-> Auto-select: {ports[0].device}")
        return ports[0].device

    while True:
        try:
            sel = input(f"Select port (1~{len(ports)}): ").strip()
            idx = int(sel) - 1
            if 0 <= idx < len(ports):
                return ports[idx].device
        except (ValueError, KeyboardInterrupt):
            pass
        print("Invalid input.")


def main():
    if len(sys.argv) >= 2:
        port = sys.argv[1]
    else:
        port = select_port()

    t = threading.Thread(target=serial_thread, args=(port,), daemon=True)
    t.start()

    fig = plt.figure(figsize=(10, 7))
    fig.canvas.manager.set_window_title("MPU-9250 IMU 3D Viewer (RC Car)")
    ax = fig.add_subplot(111, projection="3d")

    def update(frame):
        ax.cla()

        with lock:
            r, p, y = roll, pitch, yaw

        R = rotation_matrix(r, p, y)

        rotated = (R @ CUBE_VERTICES.T).T

        faces = [[rotated[v] for v in face] for face in CUBE_FACES]
        poly = Poly3DCollection(faces, facecolors=FACE_COLORS,
                                edgecolors="white", linewidths=1.5)
        ax.add_collection3d(poly)

        # World axes
        axis_len = 3.5
        ax.quiver(0, 0, 0, axis_len, 0, 0, color="r",
                  arrow_length_ratio=0.1, linewidth=2)
        ax.quiver(0, 0, 0, 0, axis_len, 0, color="g",
                  arrow_length_ratio=0.1, linewidth=2)
        ax.quiver(0, 0, 0, 0, 0, axis_len, color="b",
                  arrow_length_ratio=0.1, linewidth=2)
        ax.text(axis_len + 0.3, 0, 0, "X", color="r", fontsize=12, fontweight="bold")
        ax.text(0, axis_len + 0.3, 0, "Y", color="g", fontsize=12, fontweight="bold")
        ax.text(0, 0, axis_len + 0.3, "Z", color="b", fontsize=12, fontweight="bold")

        # Nose arrow (+Y forward)
        nose = R @ np.array([0, 3.0, 0])
        ax.quiver(0, 0, 0, nose[0], nose[1], nose[2], color="green",
                  arrow_length_ratio=0.15, linewidth=3)

        lim = 3.5
        ax.set_xlim([-lim, lim])
        ax.set_ylim([-lim, lim])
        ax.set_zlim([-lim, lim])
        ax.set_xlabel("X")
        ax.set_ylabel("Y")
        ax.set_zlabel("Z")
        ax.set_box_aspect([1, 1, 1])

        status = "Connected" if connected else "Waiting..."
        color = "green" if connected else "red"
        ax.set_title(
            f"Roll: {r:+7.1f}   Pitch: {p:+7.1f}   Yaw: {y:+7.1f}\n"
            f"[{port} @ {BAUDRATE} - {status}]",
            fontsize=13,
            color=color if not connected else "black",
            fontfamily="monospace",
        )

    ani = animation.FuncAnimation(fig, update, interval=50, cache_frame_data=False)
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
