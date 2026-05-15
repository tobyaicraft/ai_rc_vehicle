"""
RC Car Telemetry Dashboard
───────────────────────────
HC-12 wireless: control(TX) + sensor/IMU monitor(RX)

TX: Packet protocol  STX(0xAA) | LEN | CMD | PAYLOAD | CHK | ETX(0x55)
    CMD_MOVE (0x01): dir(1B) + speed(1B)
    CMD_MODE (0x02): mode(1B)
    CMD_PING (0x10): no payload

RX (binary): ACK (0x80) / NACK (0xE0) packets
RX (text):   "L:xxxx,R:xxxx,U:xxx\\r\\n"  (IR + Ultrasonic)
             "R:+xxx.x,P:+xxx.x,Y:+xxx.x\\r\\n"  (IMU attitude)

Usage:
    python panel.py
"""

import math
import time
import threading
from collections import deque

import tkinter as tk
from tkinter import ttk, messagebox

import numpy as np
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import matplotlib.animation as animation

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("pyserial required: pip install pyserial")
    import sys; sys.exit(1)

# ── Configuration ────────────────────────────────────────────────────────────
DEFAULT_BAUD   = 38400
ADC_MAX        = 4095
VAREF          = 5.0
HISTORY_SIZE   = 200
SEND_INTERVAL  = 0.05   # 50ms MOVE repeat interval

# ── Packet Protocol ──────────────────────────────────────────────────────────
PROTO_STX = 0xAA
PROTO_ETX = 0x55
CMD_MOVE  = 0x01
CMD_MODE  = 0x02
CMD_PING  = 0x10
CMD_ACK   = 0x80
CMD_NACK  = 0xE0

DIR_STOP    = 0
DIR_FORWARD = 1
DIR_REVERSE = 2
DIR_LEFT    = 3
DIR_RIGHT   = 4

PROTO_MAX_PKT_LEN = 9   # CMD(1) + PAYLOAD(8) max

def build_packet(cmd, payload=b''):
    """STX LEN CMD [PAYLOAD] CHK ETX"""
    length = 1 + len(payload)           # LEN = CMD + PAYLOAD bytes
    chk = cmd
    for b in payload:
        chk ^= b
    return bytes([PROTO_STX, length, cmd] + list(payload) + [chk, PROTO_ETX])

DIR_MAP = {
    'Up': DIR_FORWARD, 'Down': DIR_REVERSE,
    'Left': DIR_LEFT,  'Right': DIR_RIGHT,
}
CMD_NAME = {
    'Up': 'FORWARD', 'Down': 'REVERSE',
    'Left': 'LEFT',  'Right': 'RIGHT',
}
NACK_NAMES = {0x01: "BAD_CHK", 0x02: "BAD_LEN", 0x03: "UNK_CMD"}

USB_KW = ("PL2303", "Prolific", "CH340", "FTDI", "Silicon", "CP210", "USB Serial")

# ── Color Palette ────────────────────────────────────────────────────────────
BG        = "#0a0a12"
PANEL_BG  = "#0e0e1a"
CARD_BG   = "#141422"
FG        = "#e0e0f0"
FG_DIM    = "#5a5a7a"
ACCENT    = "#00ff88"
ACCENT2   = "#00ccff"
ACCENT_L  = "#5b9aff"
ACCENT_R  = "#ff5b8a"
ACCENT_U  = "#5bff9a"
ACCENT_Y  = "#ffcc00"
WARN      = "#f9e2af"
DANGER    = "#ff4466"
GRID_CLR  = "#1a1a2e"
CHART_BG  = "#0c0c18"


def adc_to_voltage(v): return v / ADC_MAX * VAREF
def voltage_to_distance_cm(v):
    if v < 0.3: return 80.0
    if v > 3.2: return 10.0
    try: d = 29.988 * pow(v, -1.173)
    except: return 80.0
    return max(10.0, min(80.0, d))


# ── 3D Car Model ─────────────────────────────────────────────────────────────
CAR_BODY = np.array([
    [-1.0, -1.8, -0.3], [ 1.0, -1.8, -0.3],
    [ 1.0,  1.8, -0.3], [-1.0,  1.8, -0.3],
    [-1.0, -1.8,  0.3], [ 1.0, -1.8,  0.3],
    [ 1.0,  1.8,  0.3], [-1.0,  1.8,  0.3],
])
CAR_FACES = [
    [0,1,2,3],[4,5,6,7],[0,1,5,4],[2,3,7,6],[0,3,7,4],[1,2,6,5]
]
CAR_COLORS = [
    (0.08,0.08,0.15,0.9), (0.12,0.12,0.25,0.9),
    (0.8,0.2,0.2,0.8), (0.15,0.15,0.4,0.8),
    (0.9,0.7,0.1,0.8), (0.1,0.8,0.3,0.8),
]

WHEEL_W, WHEEL_H, WHEEL_D = 0.25, 0.5, 0.2
def make_wheel(cx, cy, cz):
    return np.array([
        [cx-WHEEL_W, cy-WHEEL_H, cz-WHEEL_D],[cx+WHEEL_W, cy-WHEEL_H, cz-WHEEL_D],
        [cx+WHEEL_W, cy+WHEEL_H, cz-WHEEL_D],[cx-WHEEL_W, cy+WHEEL_H, cz-WHEEL_D],
        [cx-WHEEL_W, cy-WHEEL_H, cz+WHEEL_D],[cx+WHEEL_W, cy-WHEEL_H, cz+WHEEL_D],
        [cx+WHEEL_W, cy+WHEEL_H, cz+WHEEL_D],[cx-WHEEL_W, cy+WHEEL_H, cz+WHEEL_D],
    ])

WHEELS = [
    make_wheel(-1.3, -1.2, -0.3), make_wheel(1.3, -1.2, -0.3),
    make_wheel(-1.3,  1.2, -0.3), make_wheel(1.3,  1.2, -0.3),
]

def rotation_matrix(roll_deg, pitch_deg, yaw_deg):
    r, p, y = np.radians(roll_deg), np.radians(pitch_deg), np.radians(yaw_deg)
    Ry = np.array([[np.cos(r),0,np.sin(r)],[0,1,0],[-np.sin(r),0,np.cos(r)]])
    Rx = np.array([[1,0,0],[0,np.cos(p),-np.sin(p)],[0,np.sin(p),np.cos(p)]])
    Rz = np.array([[np.cos(y),-np.sin(y),0],[np.sin(y),np.cos(y),0],[0,0,1]])
    return Rz @ Ry @ Rx


class RcDashboard:
    def __init__(self, root):
        self.root = root
        self.root.title("RC Car Telemetry Dashboard")
        self.root.configure(bg=BG)
        self.root.geometry("1400x920")
        self.root.minsize(1200, 800)

        self.ser       = None
        self.connected = False
        self.active_key = None
        self.speed     = 5
        self.sending   = False

        self.ir_left  = 0; self.ir_right = 0; self.us_dist = 0
        self.roll = 0.0;   self.pitch = 0.0;  self.yaw = 0.0
        self.rx_count = 0

        self.left_hist = deque([0]*HISTORY_SIZE, maxlen=HISTORY_SIZE)
        self.right_hist = deque([0]*HISTORY_SIZE, maxlen=HISTORY_SIZE)
        self.us_hist   = deque([0]*HISTORY_SIZE, maxlen=HISTORY_SIZE)

        self._build_ui()
        self._bind_keys()
        self._start_animation()

    # ════════════════════════════════════════════════════════════════
    # UI
    # ════════════════════════════════════════════════════════════════
    def _build_ui(self):
        self._build_topbar()
        main = tk.Frame(self.root, bg=BG)
        main.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0,8))
        top_row = tk.Frame(main, bg=BG)
        top_row.pack(fill=tk.BOTH, expand=True)
        self._build_controller(top_row)
        self._build_3d_view(top_row)
        self._build_sensor_panel(top_row)
        self._build_chart(main)

    def _build_topbar(self):
        bar = tk.Frame(self.root, bg="#06060e", height=52)
        bar.pack(fill=tk.X)
        bar.pack_propagate(False)

        tk.Label(bar, text="RC CAR TELEMETRY", bg="#06060e", fg=ACCENT,
                 font=("Consolas", 16, "bold")).pack(side=tk.LEFT, padx=20)
        tk.Label(bar, text="DASHBOARD", bg="#06060e", fg=ACCENT2,
                 font=("Consolas", 16, "bold")).pack(side=tk.LEFT)

        self._btn_conn = tk.Button(bar, text="CONNECT", width=12,
                                    bg="#1a2040", fg=ACCENT, relief=tk.FLAT,
                                    font=("Consolas", 10, "bold"), cursor="hand2",
                                    activebackground="#2a3060",
                                    command=self._toggle_connect)
        self._btn_conn.pack(side=tk.RIGHT, padx=16)

        self._baud_combo = ttk.Combobox(bar, width=7, state="readonly", font=("Consolas", 9))
        self._baud_combo["values"] = ["9600","19200","38400","57600","115200"]
        self._baud_combo.set("38400")
        self._baud_combo.pack(side=tk.RIGHT, padx=4)

        self._combo = ttk.Combobox(bar, width=10, state="readonly", font=("Consolas", 9))
        self._combo.pack(side=tk.RIGHT, padx=4)

        tk.Button(bar, text="SCAN", width=5, bg="#1a2040", fg=FG_DIM, relief=tk.FLAT,
                  font=("Consolas", 9), cursor="hand2",
                  command=self._refresh_ports).pack(side=tk.RIGHT, padx=4)

        self._led_cv = tk.Canvas(bar, width=12, height=12, bg="#06060e", highlightthickness=0)
        self._led_cv.pack(side=tk.RIGHT, padx=(10,4))
        self._led = self._led_cv.create_oval(1,1,11,11, fill=FG_DIM, outline="")

        self._status_var = tk.StringVar(value="OFFLINE")
        tk.Label(bar, textvariable=self._status_var, bg="#06060e", fg=FG_DIM,
                 font=("Consolas", 9)).pack(side=tk.RIGHT, padx=4)

        self._rx_var = tk.StringVar(value="")
        tk.Label(bar, textvariable=self._rx_var, bg="#06060e", fg=ACCENT_Y,
                 font=("Consolas", 9)).pack(side=tk.RIGHT, padx=(0,10))

        self._refresh_ports()

    # ── Controller ───────────────────────────────────────────────
    def _build_controller(self, parent):
        f = tk.Frame(parent, bg=CARD_BG, width=280,
                     highlightbackground="#1e1e3a", highlightthickness=1)
        f.pack(side=tk.LEFT, fill=tk.Y, padx=(0,4), pady=4)
        f.pack_propagate(False)

        tk.Label(f, text="CONTROL", bg=CARD_BG, fg=ACCENT,
                 font=("Consolas", 11, "bold")).pack(pady=(12,4))

        self._lbl_cmd = tk.Label(f, text="STANDBY", bg=CARD_BG, fg=FG_DIM,
                                  font=("Consolas", 18, "bold"))
        self._lbl_cmd.pack(pady=(8,12))

        # D-pad
        grid = tk.Frame(f, bg=CARD_BG)
        grid.pack()
        btn_cfg = dict(width=5, height=2, relief=tk.FLAT,
                       font=("Consolas", 16, "bold"), cursor="hand2",
                       activebackground="#2a2a50")

        tk.Label(grid, bg=CARD_BG, width=5).grid(row=0, column=0)
        self._btn_up = tk.Button(grid, text="W", bg="#1a1a30", fg=FG, **btn_cfg)
        self._btn_up.grid(row=0, column=1, padx=2, pady=2)

        self._btn_left = tk.Button(grid, text="A", bg="#1a1a30", fg=FG, **btn_cfg)
        self._btn_left.grid(row=1, column=0, padx=2, pady=2)
        self._btn_stop = tk.Button(grid, text="■", bg="#1a0a10", fg=DANGER, **btn_cfg,
                                    command=self._send_stop)
        self._btn_stop.grid(row=1, column=1, padx=2, pady=2)
        self._btn_right = tk.Button(grid, text="D", bg="#1a1a30", fg=FG, **btn_cfg)
        self._btn_right.grid(row=1, column=2, padx=2, pady=2)

        self._btn_down = tk.Button(grid, text="S", bg="#1a1a30", fg=FG, **btn_cfg)
        self._btn_down.grid(row=2, column=1, padx=2, pady=2)

        self._dpad_btns = {
            'Up': self._btn_up, 'Down': self._btn_down,
            'Left': self._btn_left, 'Right': self._btn_right,
        }

        # Mouse press/release on D-pad buttons
        for d, btn in self._dpad_btns.items():
            btn.bind('<ButtonPress-1>',   lambda e, k=d: self._key_press(k))
            btn.bind('<ButtonRelease-1>', lambda e, k=d: self._key_release(k))

        # Speed
        tk.Label(f, text="THROTTLE", bg=CARD_BG, fg=FG_DIM,
                 font=("Consolas", 9)).pack(pady=(20,4))

        self._lbl_speed = tk.Label(f, text=f"{self.speed*10}%", bg=CARD_BG, fg=ACCENT,
                                    font=("Consolas", 28, "bold"))
        self._lbl_speed.pack()

        spd_frm = tk.Frame(f, bg=CARD_BG)
        spd_frm.pack(pady=6)
        self._speed_btns = {}
        for i in range(1, 10):
            active = (i == self.speed)
            btn = tk.Button(spd_frm, text=str(i), width=2,
                            bg=ACCENT if active else "#1a1a30",
                            fg=BG if active else FG_DIM,
                            relief=tk.FLAT, font=("Consolas", 9, "bold"),
                            cursor="hand2", command=lambda n=i: self._set_speed(n))
            btn.pack(side=tk.LEFT, padx=1)
            self._speed_btns[i] = btn

        self._speed_bar = tk.Canvas(f, height=8, bg="#0a0a14", highlightthickness=0)
        self._speed_bar.pack(fill=tk.X, padx=30, pady=(4,0))
        self._draw_speed_bar()

        # ── Protocol Controls ────────────────────────────────────
        tk.Frame(f, bg="#1e1e3a", height=1).pack(fill=tk.X, padx=12, pady=(14,8))

        # Mode selector
        mode_row = tk.Frame(f, bg=CARD_BG)
        mode_row.pack(pady=(0,6))
        tk.Label(mode_row, text="MODE", bg=CARD_BG, fg=FG_DIM,
                 font=("Consolas", 9)).pack(side=tk.LEFT, padx=(0,6))
        self._mode_combo = ttk.Combobox(mode_row, width=13, state="readonly",
                                         font=("Consolas", 9))
        self._mode_combo["values"] = ["MANUAL", "CALIBRATION", "AUTO"]
        self._mode_combo.current(0)
        self._mode_combo.pack(side=tk.LEFT)
        self._mode_combo.bind("<<ComboboxSelected>>", self._on_mode_change)

        # PING button
        self._btn_ping = tk.Button(f, text="PING", width=10,
                                    bg="#1a2040", fg=ACCENT2, relief=tk.FLAT,
                                    font=("Consolas", 9, "bold"), cursor="hand2",
                                    command=self._send_ping)
        self._btn_ping.pack(pady=(4,4))

        # ACK / NACK status
        tk.Label(f, text="LAST RESPONSE", bg=CARD_BG, fg=FG_DIM,
                 font=("Consolas", 8)).pack()
        self._lbl_ack = tk.Label(f, text="---", bg=CARD_BG, fg=FG_DIM,
                                   font=("Consolas", 12, "bold"))
        self._lbl_ack.pack(pady=(2,0))

        tk.Label(f, text="WASD / 1~9 / Space=Stop", bg=CARD_BG, fg=FG_DIM,
                 font=("Consolas", 8)).pack(side=tk.BOTTOM, pady=10)

    # ── 3D Attitude View ─────────────────────────────────────────
    def _build_3d_view(self, parent):
        f = tk.Frame(parent, bg=CARD_BG,
                     highlightbackground="#1e1e3a", highlightthickness=1)
        f.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=4, pady=4)

        tk.Label(f, text="3D ATTITUDE", bg=CARD_BG, fg=ACCENT2,
                 font=("Consolas", 11, "bold")).pack(pady=(8,0))

        att_frm = tk.Frame(f, bg=CARD_BG)
        att_frm.pack(pady=(2,0))

        for label, color, attr in [("ROLL","#5b9aff","_lbl_roll"),
                                    ("PITCH","#ff9a5b","_lbl_pitch"),
                                    ("YAW","#ffcc00","_lbl_yaw")]:
            cell = tk.Frame(att_frm, bg=CARD_BG)
            cell.pack(side=tk.LEFT, padx=15)
            tk.Label(cell, text=label, bg=CARD_BG, fg=FG_DIM,
                     font=("Consolas", 8)).pack()
            lbl = tk.Label(cell, text="+000.0", bg=CARD_BG, fg=color,
                           font=("Consolas", 16, "bold"))
            lbl.pack()
            setattr(self, attr, lbl)

        self.fig3d = Figure(figsize=(5,4), dpi=100, facecolor='#0a0a14')
        self.ax3d  = self.fig3d.add_subplot(111, projection='3d')
        self.ax3d.set_facecolor('#0a0a14')
        self.canvas3d = FigureCanvasTkAgg(self.fig3d, master=f)
        self.canvas3d.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=6, pady=(2,6))

    # ── Sensor Panel ─────────────────────────────────────────────
    def _build_sensor_panel(self, parent):
        f = tk.Frame(parent, bg=CARD_BG, width=280,
                     highlightbackground="#1e1e3a", highlightthickness=1)
        f.pack(side=tk.RIGHT, fill=tk.Y, padx=(4,0), pady=4)
        f.pack_propagate(False)

        tk.Label(f, text="PROXIMITY", bg=CARD_BG, fg=ACCENT_U,
                 font=("Consolas", 11, "bold")).pack(pady=(12,8))

        us_card = tk.Frame(f, bg="#0a0a18", highlightbackground="#1a3a2a",
                           highlightthickness=1)
        us_card.pack(fill=tk.X, padx=12, pady=(0,8))
        tk.Label(us_card, text="ULTRASONIC  FRONT", bg="#0a0a18", fg=ACCENT_U,
                 font=("Consolas", 9)).pack(pady=(6,0))
        self._lbl_us = tk.Label(us_card, text="0 cm", bg="#0a0a18", fg=ACCENT_U,
                                 font=("Consolas", 32, "bold"))
        self._lbl_us.pack(pady=(0,2))
        self._us_bar = tk.Canvas(us_card, height=10, bg="#060612", highlightthickness=0)
        self._us_bar.pack(fill=tk.X, padx=12, pady=(0,8))

        self.car_top = tk.Canvas(f, bg="#0a0a14", highlightthickness=0, height=200)
        self.car_top.pack(fill=tk.X, padx=12, pady=4)

        ir_frm = tk.Frame(f, bg=CARD_BG)
        ir_frm.pack(fill=tk.X, padx=12, pady=(8,0))

        left_card = tk.Frame(ir_frm, bg="#0a0a18", highlightbackground="#1a2a3a",
                             highlightthickness=1)
        left_card.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0,3))
        tk.Label(left_card, text="IR LEFT", bg="#0a0a18", fg=ACCENT_L,
                 font=("Consolas", 8)).pack(pady=(4,0))
        self._lbl_ir_l = tk.Label(left_card, text="0", bg="#0a0a18", fg=ACCENT_L,
                                   font=("Consolas", 18, "bold"))
        self._lbl_ir_l.pack()
        self._lbl_ir_l_cm = tk.Label(left_card, text="-- cm", bg="#0a0a18", fg=FG_DIM,
                                      font=("Consolas", 10))
        self._lbl_ir_l_cm.pack(pady=(0,4))

        right_card = tk.Frame(ir_frm, bg="#0a0a18", highlightbackground="#3a1a2a",
                              highlightthickness=1)
        right_card.pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(3,0))
        tk.Label(right_card, text="IR RIGHT", bg="#0a0a18", fg=ACCENT_R,
                 font=("Consolas", 8)).pack(pady=(4,0))
        self._lbl_ir_r = tk.Label(right_card, text="0", bg="#0a0a18", fg=ACCENT_R,
                                   font=("Consolas", 18, "bold"))
        self._lbl_ir_r.pack()
        self._lbl_ir_r_cm = tk.Label(right_card, text="-- cm", bg="#0a0a18", fg=FG_DIM,
                                      font=("Consolas", 10))
        self._lbl_ir_r_cm.pack(pady=(0,4))

    # ── Chart ────────────────────────────────────────────────────
    def _build_chart(self, parent):
        f = tk.Frame(parent, bg=CARD_BG, height=180,
                     highlightbackground="#1e1e3a", highlightthickness=1)
        f.pack(fill=tk.X, pady=(4,0))
        f.pack_propagate(False)
        self.fig_chart = Figure(figsize=(10,1.6), dpi=100, facecolor=CHART_BG)
        self.ax_chart  = self.fig_chart.add_subplot(111)
        self._setup_chart()
        self.canvas_chart = FigureCanvasTkAgg(self.fig_chart, master=f)
        self.canvas_chart.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

    def _setup_chart(self):
        ax = self.ax_chart
        ax.set_facecolor(CHART_BG)
        ax.set_xlim(0, HISTORY_SIZE)
        ax.set_ylim(0, 150)
        ax.set_ylabel("cm", color=FG_DIM, fontsize=8)
        ax.tick_params(colors=FG_DIM, labelsize=7)
        ax.grid(True, color=GRID_CLR, alpha=0.5, linestyle="--")
        for s in ax.spines.values(): s.set_color(GRID_CLR)
        ax.axhspan(0, 20, alpha=0.06, color=DANGER)
        ax.axhline(y=20, color=DANGER, alpha=0.25, linestyle="--", linewidth=0.8)
        self.ln_l, = ax.plot([], [], color=ACCENT_L, lw=1.2, label="IR L", alpha=0.85)
        self.ln_r, = ax.plot([], [], color=ACCENT_R, lw=1.2, label="IR R", alpha=0.85)
        self.ln_u, = ax.plot([], [], color=ACCENT_U, lw=2.0, label="US",   alpha=0.9)
        ax.legend(loc="upper right", facecolor=CHART_BG, edgecolor=GRID_CLR,
                  labelcolor=FG_DIM, fontsize=7)
        self.fig_chart.tight_layout(pad=1.5)

    # ════════════════════════════════════════════════════════════════
    # Animation
    # ════════════════════════════════════════════════════════════════
    def _start_animation(self):
        self.ani = animation.FuncAnimation(self.fig_chart, self._update_all,
                                           interval=100, blit=False,
                                           cache_frame_data=False)

    def _update_all(self, frame):
        lv = adc_to_voltage(self.ir_left)
        rv = adc_to_voltage(self.ir_right)
        ld = voltage_to_distance_cm(lv)
        rd = voltage_to_distance_cm(rv)
        ud = float(self.us_dist)

        self.left_hist.append(ld)
        self.right_hist.append(rd)
        self.us_hist.append(ud)

        x = list(range(HISTORY_SIZE))
        self.ln_l.set_data(x, list(self.left_hist))
        self.ln_r.set_data(x, list(self.right_hist))
        self.ln_u.set_data(x, list(self.us_hist))

        self._lbl_ir_l.configure(text=str(self.ir_left))
        self._lbl_ir_l_cm.configure(text=f"{ld:.0f} cm")
        self._lbl_ir_r.configure(text=str(self.ir_right))
        self._lbl_ir_r_cm.configure(text=f"{rd:.0f} cm")

        us_color = DANGER if ud < 20 else (WARN if ud < 50 else ACCENT_U)
        self._lbl_us.configure(text=f"{ud:.0f} cm", fg=us_color)
        self._draw_bar(self._us_bar, ud, 200, ACCENT_U)

        self._lbl_roll.configure(text=f"{self.roll:+07.1f}")
        self._lbl_pitch.configure(text=f"{self.pitch:+07.1f}")
        self._lbl_yaw.configure(text=f"{self.yaw:+07.1f}")

        self._draw_3d()
        self._draw_car_top(ld, rd, ud)
        self._rx_var.set(f"RX:{self.rx_count}")

        self.canvas_chart.draw_idle()
        return [self.ln_l, self.ln_r, self.ln_u]

    def _draw_bar(self, canvas, val, max_val, color):
        canvas.delete("all")
        w = canvas.winfo_width()
        h = canvas.winfo_height()
        if w <= 1: return
        ratio = min(val / max_val, 1.0)
        bw = int(w * ratio)
        bc = DANGER if val < 20 else (WARN if val < 50 else color)
        canvas.create_rectangle(0,0,w,h, fill="#0a0a14", outline="")
        if bw > 0:
            canvas.create_rectangle(0,0,bw,h, fill=bc, outline="")

    def _draw_speed_bar(self):
        c = self._speed_bar
        c.delete("all")
        w = c.winfo_width()
        h = c.winfo_height()
        if w <= 1: return
        ratio = self.speed / 9.0
        bw = int(w * ratio)
        c.create_rectangle(0,0,w,h, fill="#0a0a14", outline="")
        if bw > 0:
            c.create_rectangle(0,0,bw,h, fill=ACCENT, outline="")

    def _draw_3d(self):
        ax = self.ax3d
        ax.cla()
        ax.set_facecolor('#0a0a14')
        R = rotation_matrix(self.roll, self.pitch, self.yaw)

        rotated = (R @ CAR_BODY.T).T
        faces = [[rotated[v] for v in f] for f in CAR_FACES]
        poly = Poly3DCollection(faces, facecolors=CAR_COLORS,
                                edgecolors="#ffffff40", linewidths=0.8)
        ax.add_collection3d(poly)

        for wh in WHEELS:
            rw = (R @ wh.T).T
            wf = [[rw[v] for v in f] for f in CAR_FACES]
            wp = Poly3DCollection(wf, facecolors=[(0.2,0.2,0.2,0.9)]*6,
                                  edgecolors="#333333", linewidths=0.5)
            ax.add_collection3d(wp)

        nose = R @ np.array([0, 2.8, 0])
        ax.quiver(0,0,0, nose[0],nose[1],nose[2], color=ACCENT,
                  arrow_length_ratio=0.15, linewidth=2.5)

        al = 3.0
        ax.quiver(0,0,0, al,0,0, color="#ff444444", arrow_length_ratio=0.08, linewidth=0.8)
        ax.quiver(0,0,0, 0,al,0, color="#44ff4444", arrow_length_ratio=0.08, linewidth=0.8)
        ax.quiver(0,0,0, 0,0,al, color="#4444ff44", arrow_length_ratio=0.08, linewidth=0.8)

        grid_pts = np.linspace(-3, 3, 7)
        for g in grid_pts:
            ax.plot([g,g],[-3,3],[-3], color=GRID_CLR, linewidth=0.3, alpha=0.3)
            ax.plot([-3,3],[g,g],[-3], color=GRID_CLR, linewidth=0.3, alpha=0.3)

        lim = 3.5
        ax.set_xlim([-lim,lim]); ax.set_ylim([-lim,lim]); ax.set_zlim([-lim,lim])
        ax.set_box_aspect([1,1,1])
        ax.set_axis_off()
        self.canvas3d.draw_idle()

    def _draw_car_top(self, ld, rd, ud):
        c = self.car_top
        c.delete("all")
        w = c.winfo_width()
        h = c.winfo_height()
        if w <= 1: return

        cx, cy = w//2, h//2 + 15
        cw, ch = 50, 75

        c.create_rectangle(cx-cw//2, cy-ch//2, cx+cw//2, cy+ch//2,
                           fill="#1a1a30", outline="#2a2a50", width=1)
        c.create_text(cx, cy+5, text="TC237", fill=FG_DIM, font=("Consolas",8))

        for dy in [-28, 28]:
            c.create_rectangle(cx-cw//2-5, cy+dy-8, cx-cw//2, cy+dy+8,
                               fill="#333355", outline="")
            c.create_rectangle(cx+cw//2, cy+dy-8, cx+cw//2+5, cy+dy+8,
                               fill="#333355", outline="")

        front_y = cy - ch//2
        us_len = max(12, min(60, ud * 0.5))
        uc = DANGER if ud < 20 else (WARN if ud < 50 else ACCENT_U)
        c.create_polygon(cx-12, front_y, cx+12, front_y,
                         cx+30, front_y-us_len, cx-30, front_y-us_len,
                         fill="", outline=uc, width=1)
        c.create_text(cx, front_y-us_len-10, text=f"{ud:.0f}cm",
                      fill=uc, font=("Consolas",10,"bold"))

        angle = math.radians(45)
        sp = 10

        ll = max(10, min(55, ld * 0.7))
        lc = DANGER if ld < 20 else (WARN if ld < 40 else ACCENT_L)
        lx0, ly0 = cx-cw//2, front_y
        lx1 = lx0 - ll*math.sin(angle)
        ly1 = ly0 - ll*math.cos(angle)
        c.create_line(lx0,ly0, lx1-sp*0.3,ly1-sp*0.3, fill=lc, width=1, arrow=tk.LAST)
        c.create_line(lx0,ly0, lx1+sp*0.4,ly1-sp*0.4, fill=lc, width=1, arrow=tk.LAST)
        c.create_text(lx1-3, ly1-12, text=f"{ld:.0f}", fill=lc, font=("Consolas",9,"bold"))

        rl = max(10, min(55, rd * 0.7))
        rc = DANGER if rd < 20 else (WARN if rd < 40 else ACCENT_R)
        rx0, ry0 = cx+cw//2, front_y
        rx1 = rx0 + rl*math.sin(angle)
        ry1 = ry0 - rl*math.cos(angle)
        c.create_line(rx0,ry0, rx1+sp*0.3,ry1-sp*0.3, fill=rc, width=1, arrow=tk.LAST)
        c.create_line(rx0,ry0, rx1-sp*0.4,ry1-sp*0.4, fill=rc, width=1, arrow=tk.LAST)
        c.create_text(rx1+3, ry1-12, text=f"{rd:.0f}", fill=rc, font=("Consolas",9,"bold"))

    # ════════════════════════════════════════════════════════════════
    # Keys
    # ════════════════════════════════════════════════════════════════
    def _bind_keys(self):
        wasd = [('w','Up'),('W','Up'),('s','Down'),('S','Down'),
                ('a','Left'),('A','Left'),('d','Right'),('D','Right')]
        for k, direction in wasd:
            self.root.bind(f'<KeyPress-{k}>',   lambda e, d=direction: self._key_press(d))
            self.root.bind(f'<KeyRelease-{k}>', lambda e, d=direction: self._key_release(d))
        for i in range(1, 10):
            self.root.bind(str(i), lambda e, n=i: self._set_speed(n))
        self.root.bind('<Escape>', lambda e: self.on_close())
        self.root.bind('<space>',  lambda e: self._send_stop())

    def _key_press(self, d):
        if not self.connected or self.active_key == d:
            return
        self.active_key = d
        self._update_dpad(d)
        self._lbl_cmd.configure(text=CMD_NAME.get(d, "?"), fg=ACCENT)
        if not self.sending:
            self.sending = True
            threading.Thread(target=self._send_loop, daemon=True).start()

    def _key_release(self, d):
        if self.active_key == d:
            self.active_key = None
            self.sending = False
            self._update_dpad(None)
            self._lbl_cmd.configure(text="STANDBY", fg=FG_DIM)
            self._write_packet(build_packet(CMD_MOVE, bytes([DIR_STOP, 0])))

    def _send_loop(self):
        """Repeatedly send MOVE packet at SEND_INTERVAL while key is held."""
        while self.sending and self.connected:
            k = self.active_key
            if k and k in DIR_MAP:
                pkt = build_packet(CMD_MOVE, bytes([DIR_MAP[k], self.speed * 10]))
                if not self._write_packet(pkt):
                    break
            else:
                break
            time.sleep(SEND_INTERVAL)
        self.sending = False

    def _send_stop(self):
        self.active_key = None
        self.sending = False
        self._update_dpad(None)
        self._lbl_cmd.configure(text="STANDBY", fg=FG_DIM)
        self._write_packet(build_packet(CMD_MOVE, bytes([DIR_STOP, 0])))

    def _send_ping(self):
        self._write_packet(build_packet(CMD_PING))

    def _on_mode_change(self, _event=None):
        mode_idx = self._mode_combo.current()   # 0=MANUAL, 1=CALIB, 2=AUTO
        self._write_packet(build_packet(CMD_MODE, bytes([mode_idx])))

    def _write_packet(self, pkt):
        """Send packet; returns False on error."""
        if not self.connected or not self.ser:
            return False
        try:
            self.ser.write(pkt)
            return True
        except Exception:
            return False

    def _update_dpad(self, active):
        for d, btn in self._dpad_btns.items():
            if d == active:
                btn.configure(bg=ACCENT, fg=BG)
            else:
                btn.configure(bg="#1a1a30", fg=FG)

    def _set_speed(self, n):
        n = max(1, min(9, n))
        self.speed = n
        for i, btn in self._speed_btns.items():
            btn.configure(bg=ACCENT if i==n else "#1a1a30",
                          fg=BG if i==n else FG_DIM)
        self._lbl_speed.configure(text=f"{n*10}%")
        self._draw_speed_bar()
        # If currently moving, immediately update speed
        if self.connected and self.active_key and self.active_key in DIR_MAP:
            self._write_packet(build_packet(CMD_MOVE, bytes([DIR_MAP[self.active_key], n * 10])))

    # ════════════════════════════════════════════════════════════════
    # Serial
    # ════════════════════════════════════════════════════════════════
    def _refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        names = [p.device for p in ports]
        self._combo["values"] = names
        if names:
            for i, p in enumerate(ports):
                if any(k in p.description for k in USB_KW):
                    self._combo.current(i); return
            self._combo.current(0)

    def _toggle_connect(self):
        if self.connected: self._disconnect()
        else: self._connect()

    def _connect(self):
        port = self._combo.get().strip()
        if not port:
            messagebox.showwarning("Warning", "Select COM port"); return
        baud = int(self._baud_combo.get())
        try:
            self.ser = serial.Serial(port, baud, timeout=1)
        except Exception as e:
            messagebox.showerror("Error", f"Cannot open {port}\n{e}"); return

        self.connected = True
        self.rx_count  = 0
        self._btn_conn.configure(text="DISCONNECT", fg=DANGER)
        self._status_var.set(f"{port}@{baud}")
        self._led_cv.itemconfigure(self._led, fill=ACCENT)
        self._combo.configure(state="disabled")
        self._baud_combo.configure(state="disabled")
        self.root.focus_set()
        threading.Thread(target=self._read_serial, daemon=True).start()

    def _disconnect(self):
        self.connected = False
        self.sending   = False
        self.active_key = None
        if self.ser:
            try: self.ser.close()
            except: pass
            self.ser = None
        self._btn_conn.configure(text="CONNECT", fg=ACCENT)
        self._status_var.set("OFFLINE")
        self._led_cv.itemconfigure(self._led, fill=FG_DIM)
        self._combo.configure(state="readonly")
        self._baud_combo.configure(state="readonly")
        self._update_dpad(None)
        self._lbl_cmd.configure(text="STANDBY", fg=FG_DIM)

    # ── RX: mixed binary packet + text line parser ────────────────
    def _read_serial(self):
        rx_buf = bytearray()
        while self.connected and self.ser and self.ser.is_open:
            try:
                raw = self.ser.read(self.ser.in_waiting or 1)
                if not raw:
                    continue
                rx_buf.extend(raw)

                while True:
                    if not rx_buf:
                        break

                    stx_pos = rx_buf.find(b'\xaa')
                    nl_pos  = rx_buf.find(b'\n')

                    # Nothing actionable yet
                    if stx_pos == -1 and nl_pos == -1:
                        if len(rx_buf) > 256:
                            rx_buf.clear()
                        break

                    # Binary packet arrives before (or instead of) text line
                    if stx_pos != -1 and (nl_pos == -1 or stx_pos < nl_pos):
                        # Discard junk before STX
                        if stx_pos > 0:
                            rx_buf = rx_buf[stx_pos:]
                            continue
                        # Need at least STX + LEN
                        if len(rx_buf) < 2:
                            break
                        pkt_len = rx_buf[1]
                        # Sanity-check LEN before allocating
                        if pkt_len == 0 or pkt_len > PROTO_MAX_PKT_LEN:
                            rx_buf = rx_buf[1:]   # skip bad STX
                            continue
                        total = 1 + 1 + pkt_len + 1 + 1  # STX LEN [CMD+PLD] CHK ETX
                        if len(rx_buf) < total:
                            break   # wait for more bytes
                        pkt = bytes(rx_buf[:total])
                        rx_buf = rx_buf[total:]
                        if pkt[-1] == PROTO_ETX:
                            self._handle_binary_packet(pkt)

                    # Text line
                    elif nl_pos != -1:
                        line = rx_buf[:nl_pos].decode('ascii', errors='ignore').strip()
                        rx_buf = rx_buf[nl_pos + 1:]
                        if line:
                            self._handle_text_line(line)
                    else:
                        break

            except Exception:
                break

        self.connected = False
        try:
            self.root.after(0, lambda: self._status_var.set("OFFLINE"))
            self.root.after(0, lambda: self._btn_conn.configure(text="CONNECT", fg=ACCENT))
            self.root.after(0, lambda: self._led_cv.itemconfigure(self._led, fill=FG_DIM))
        except tk.TclError:
            pass

    def _handle_binary_packet(self, pkt):
        """pkt: bytes  →  AA LEN CMD [PAYLOAD] CHK ETX"""
        if len(pkt) < 5:
            return
        cmd = pkt[2]
        if cmd == CMD_ACK:
            orig = pkt[3] if len(pkt) > 5 else 0
            label = f"ACK  CMD={orig:02X}"
            color = ACCENT
        elif cmd == CMD_NACK:
            err   = pkt[3] if len(pkt) > 5 else 0
            label = f"NACK {NACK_NAMES.get(err, f'E{err:02X}')}"
            color = DANGER
        else:
            return
        self.rx_count += 1
        # Schedule UI update on main thread
        self.root.after(0, lambda l=label, c=color:
                        self._lbl_ack.configure(text=l, fg=c))

    def _handle_text_line(self, line):
        """Parse sensor or IMU text lines."""
        # Sensor: "L:xxxx,R:xxxx,U:xxx"
        if line.startswith("L:") and ",R:" in line and ",U:" in line:
            try:
                parts = line.split(",")
                self.ir_left  = int(parts[0][2:])
                self.ir_right = int(parts[1][2:])
                self.us_dist  = int(parts[2][2:])
                self.rx_count += 1
            except Exception:
                pass
        # IMU: "R:+xxx.x,P:+xxx.x,Y:+xxx.x"
        elif line.startswith("R:") and ",P:" in line and ",Y:" in line:
            try:
                parts = line.split(",")
                self.roll  = float(parts[0][2:])
                self.pitch = float(parts[1][2:])
                self.yaw   = float(parts[2][2:])
                self.rx_count += 1
            except Exception:
                pass

    def on_close(self):
        self.connected = False
        self.sending   = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.root.destroy()


def main():
    root = tk.Tk()
    app  = RcDashboard(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()

if __name__ == "__main__":
    main()
