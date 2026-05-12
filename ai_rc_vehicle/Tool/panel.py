"""
RC Car Control Panel (Controller + Sensor Monitor)
───────────────────────────────────────────────────
HC-12 시리얼 하나로 조종(TX) + 센서 모니터(RX) 통합.

TX: 방향키 ESC 시퀀스 (0x1B 5B 41~44), 속도 '1'~'9'
RX: "L:xxxx,R:xxxx,U:xxx\r\n"

Usage:
    python panel.py
"""

import math
import time
import threading
from collections import deque

import tkinter as tk
from tkinter import ttk, messagebox

import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.animation as animation

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("pyserial required: pip install pyserial")
    import sys; sys.exit(1)

# ── Configuration ────────────────────────────────────────────────────────────
DEFAULT_BAUD = 38400
ADC_MAX = 4095
VAREF = 5.0
HISTORY_SIZE = 200
SEND_INTERVAL = 0.05  # 50ms key repeat

# ── Color Palette ────────────────────────────────────────────────────────────
BG_COLOR = "#1e1e2e"
FG_COLOR = "#cdd6f4"
ACCENT_L = "#89b4fa"
ACCENT_R = "#f38ba8"
ACCENT_U = "#a6e3a1"
PHOS = "#00ff88"
WARN_COLOR = "#f9e2af"
DANGER_COLOR = "#f38ba8"
CHART_BG = "#181825"
GRID_COLOR = "#313244"
TEXT_DIM = "#6c7086"
PANEL_BG = "#11111b"

# ── Arrow key map ────────────────────────────────────────────────────────────
ARROW_MAP = {
    'Up':    b'\x1b[A',
    'Down':  b'\x1b[B',
    'Right': b'\x1b[C',
    'Left':  b'\x1b[D',
}
CMD_NAME = {
    'Up': 'FWD  ↑', 'Down': 'REV  ↓',
    'Left': 'L-SPIN ←', 'Right': 'R-SPIN →',
}

USB_KW = ("PL2303", "Prolific", "CH340", "FTDI", "Silicon", "CP210", "USB Serial")


def adc_to_voltage(adc_val):
    return adc_val / ADC_MAX * VAREF


def voltage_to_distance_cm(voltage):
    if voltage < 0.3:
        return 80.0
    if voltage > 3.2:
        return 10.0
    try:
        dist = 29.988 * pow(voltage, -1.173)
    except (ValueError, ZeroDivisionError):
        return 80.0
    return max(10.0, min(80.0, dist))


class RcPanel:
    def __init__(self, root):
        self.root = root
        self.root.title("RC Car Control Panel")
        self.root.configure(bg=BG_COLOR)
        self.root.geometry("1280x900")
        self.root.minsize(1100, 800)

        # Serial
        self.ser = None
        self.connected = False

        # Controller state
        self.active_key = None
        self.speed = 5
        self.sending = False

        # Sensor state
        self.ir_left = 0
        self.ir_right = 0
        self.us_dist = 0
        self.rx_count = 0

        self.left_history = deque([0] * HISTORY_SIZE, maxlen=HISTORY_SIZE)
        self.right_history = deque([0] * HISTORY_SIZE, maxlen=HISTORY_SIZE)
        self.us_history = deque([0] * HISTORY_SIZE, maxlen=HISTORY_SIZE)

        self._build_ui()
        self._bind_keys()
        self._start_animation()

    # ════════════════════════════════════════════════════════════════════════
    # UI Build
    # ════════════════════════════════════════════════════════════════════════
    def _build_ui(self):
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("Dark.TFrame", background=BG_COLOR)
        style.configure("Dark.TLabel", background=BG_COLOR, foreground=FG_COLOR,
                        font=("Segoe UI", 10))
        style.configure("Panel.TFrame", background=PANEL_BG)

        self._build_topbar()

        # Main content: left (controller) | right (sensor)
        main = ttk.Frame(self.root, style="Dark.TFrame")
        main.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        self._build_controller_panel(main)
        self._build_sensor_panel(main)

    def _build_topbar(self):
        top = tk.Frame(self.root, bg="#11111b", height=50)
        top.pack(fill=tk.X)
        top.pack_propagate(False)

        tk.Label(top, text="RC Car Control Panel", bg="#11111b", fg=PHOS,
                 font=("Consolas", 14, "bold")).pack(side=tk.LEFT, padx=16, pady=10)

        # LED
        self._led_cv = tk.Canvas(top, width=14, height=14,
                                  bg="#11111b", highlightthickness=0)
        self._led_cv.pack(side=tk.LEFT, padx=(8, 3))
        self._led = self._led_cv.create_oval(2, 2, 12, 12, fill=TEXT_DIM, outline="")

        self._status_var = tk.StringVar(value="Disconnected")
        tk.Label(top, textvariable=self._status_var, bg="#11111b", fg=TEXT_DIM,
                 font=("Consolas", 10)).pack(side=tk.LEFT, padx=(2, 20))

        # Connect button
        self._btn_conn = tk.Button(top, text="Connect", width=10,
                                    bg="#162035", fg=FG_COLOR, relief=tk.FLAT,
                                    font=("Consolas", 10, "bold"), cursor="hand2",
                                    command=self._toggle_connect)
        self._btn_conn.pack(side=tk.RIGHT, padx=16)

        # Baud
        self._baud_combo = ttk.Combobox(top, width=7, state="readonly",
                                         font=("Consolas", 9))
        self._baud_combo["values"] = ["9600", "19200", "38400", "57600", "115200"]
        self._baud_combo.set("38400")
        self._baud_combo.pack(side=tk.RIGHT, padx=(5, 0))
        tk.Label(top, text="Baud:", bg="#11111b", fg=TEXT_DIM,
                 font=("Consolas", 10)).pack(side=tk.RIGHT)

        # Port combo
        self._combo = ttk.Combobox(top, width=14, state="readonly",
                                    font=("Consolas", 9))
        self._combo.pack(side=tk.RIGHT, padx=(5, 10))

        tk.Button(top, text="Scan", width=5, bg="#162035", fg=FG_COLOR,
                  relief=tk.FLAT, font=("Consolas", 9), cursor="hand2",
                  command=self._refresh_ports).pack(side=tk.RIGHT, padx=(5, 0))

        tk.Label(top, text="Port:", bg="#11111b", fg=TEXT_DIM,
                 font=("Consolas", 10)).pack(side=tk.RIGHT)

        # RX count
        self._rx_var = tk.StringVar(value="RX: 0")
        tk.Label(top, textvariable=self._rx_var, bg="#11111b", fg=WARN_COLOR,
                 font=("Consolas", 9)).pack(side=tk.RIGHT, padx=(0, 15))

        self._refresh_ports()

    # ── Controller Panel (Left) ──────────────────────────────────────────────
    def _build_controller_panel(self, parent):
        left = tk.Frame(parent, bg=PANEL_BG, width=320, relief=tk.FLAT,
                        highlightbackground=GRID_COLOR, highlightthickness=1)
        left.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 5), pady=5)
        left.pack_propagate(False)

        tk.Label(left, text="CONTROLLER", bg=PANEL_BG, fg=PHOS,
                 font=("Consolas", 12, "bold")).pack(pady=(15, 5))

        # Command display
        self._lbl_cmd = tk.Label(left, text="STOP", bg=PANEL_BG, fg=TEXT_DIM,
                                  font=("Consolas", 22, "bold"))
        self._lbl_cmd.pack(pady=(10, 15))

        # D-pad
        grid = tk.Frame(left, bg=PANEL_BG)
        grid.pack()

        btn_cfg = dict(width=6, height=2, relief=tk.FLAT,
                       font=("Consolas", 16, "bold"), cursor="hand2",
                       activebackground="#2a3850")

        tk.Label(grid, bg=PANEL_BG, width=6).grid(row=0, column=0)
        self._btn_up = tk.Button(grid, text="▲", bg="#162035", fg=FG_COLOR, **btn_cfg)
        self._btn_up.grid(row=0, column=1, padx=3, pady=3)
        tk.Label(grid, bg=PANEL_BG, width=6).grid(row=0, column=2)

        self._btn_left = tk.Button(grid, text="◄", bg="#162035", fg=FG_COLOR, **btn_cfg)
        self._btn_left.grid(row=1, column=0, padx=3, pady=3)
        self._btn_stop = tk.Button(grid, text="■", bg="#1a1020", fg=DANGER_COLOR, **btn_cfg)
        self._btn_stop.grid(row=1, column=1, padx=3, pady=3)
        self._btn_right = tk.Button(grid, text="►", bg="#162035", fg=FG_COLOR, **btn_cfg)
        self._btn_right.grid(row=1, column=2, padx=3, pady=3)

        tk.Label(grid, bg=PANEL_BG, width=6).grid(row=2, column=0)
        self._btn_down = tk.Button(grid, text="▼", bg="#162035", fg=FG_COLOR, **btn_cfg)
        self._btn_down.grid(row=2, column=1, padx=3, pady=3)
        tk.Label(grid, bg=PANEL_BG, width=6).grid(row=2, column=2)

        self._dpad_btns = {
            'Up': self._btn_up, 'Down': self._btn_down,
            'Left': self._btn_left, 'Right': self._btn_right,
        }

        # Speed
        tk.Label(left, text="SPEED", bg=PANEL_BG, fg=TEXT_DIM,
                 font=("Consolas", 10)).pack(pady=(25, 5))

        spd_frm = tk.Frame(left, bg=PANEL_BG)
        spd_frm.pack()

        self._speed_btns = {}
        for i in range(1, 10):
            active = (i == self.speed)
            btn = tk.Button(spd_frm, text=str(i), width=3, height=1,
                            bg=PHOS if active else "#162035",
                            fg=PANEL_BG if active else FG_COLOR,
                            relief=tk.FLAT, font=("Consolas", 10, "bold"),
                            cursor="hand2",
                            command=lambda n=i: self._set_speed(n))
            btn.pack(side=tk.LEFT, padx=1)
            self._speed_btns[i] = btn

        self._lbl_speed = tk.Label(left, text=f"{self.speed * 10}%",
                                    bg=PANEL_BG, fg=PHOS,
                                    font=("Consolas", 18, "bold"))
        self._lbl_speed.pack(pady=(8, 0))

        # Footer
        tk.Label(left, text="Arrow keys: drive\n1~9: speed\nESC: quit",
                 bg=PANEL_BG, fg=TEXT_DIM, font=("Consolas", 9),
                 justify=tk.CENTER).pack(side=tk.BOTTOM, pady=15)

    # ── Sensor Panel (Right) ─────────────────────────────────────────────────
    def _build_sensor_panel(self, parent):
        right = tk.Frame(parent, bg=BG_COLOR)
        right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(5, 0), pady=5)

        tk.Label(right, text="SENSOR MONITOR", bg=BG_COLOR, fg=ACCENT_U,
                 font=("Consolas", 12, "bold")).pack(pady=(5, 5))

        # Chart
        chart_frm = tk.Frame(right, bg=BG_COLOR)
        chart_frm.pack(fill=tk.BOTH, expand=True)

        self.fig = Figure(figsize=(7, 2.5), dpi=100, facecolor=CHART_BG)
        self.ax = self.fig.add_subplot(111)
        self._setup_chart()
        self.canvas = FigureCanvasTkAgg(self.fig, master=chart_frm)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        # Bottom: Left IR | Car | Right IR
        bottom = tk.Frame(right, bg=BG_COLOR)
        bottom.pack(fill=tk.BOTH, padx=5, pady=(5, 5))

        # Left IR panel
        lp = tk.Frame(bottom, bg=BG_COLOR, width=180)
        lp.pack(side=tk.LEFT, fill=tk.Y)
        lp.pack_propagate(False)

        tk.Label(lp, text="LEFT IR (AN1)", bg=BG_COLOR, fg=ACCENT_L,
                 font=("Segoe UI", 11, "bold")).pack(pady=(5, 0))
        self.lbl_left_adc = tk.Label(lp, text="0", bg=BG_COLOR, fg=FG_COLOR,
                                      font=("Consolas", 28, "bold"))
        self.lbl_left_adc.pack()
        self.lbl_left_volt = tk.Label(lp, text="0.000 V", bg=BG_COLOR, fg=FG_COLOR,
                                       font=("Consolas", 11))
        self.lbl_left_volt.pack()
        self.lbl_left_dist = tk.Label(lp, text="-- cm", bg=BG_COLOR, fg=ACCENT_U,
                                       font=("Consolas", 18, "bold"))
        self.lbl_left_dist.pack(pady=(3, 0))
        self.left_gauge = tk.Canvas(lp, height=14, bg=CHART_BG, highlightthickness=0)
        self.left_gauge.pack(fill=tk.X, padx=12, pady=(6, 0))

        # Car canvas (center)
        cp = tk.Frame(bottom, bg=BG_COLOR)
        cp.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)
        self.car_canvas = tk.Canvas(cp, bg=CHART_BG, highlightthickness=0, height=280)
        self.car_canvas.pack(fill=tk.BOTH, expand=True)

        # Right IR panel
        rp = tk.Frame(bottom, bg=BG_COLOR, width=180)
        rp.pack(side=tk.RIGHT, fill=tk.Y)
        rp.pack_propagate(False)

        tk.Label(rp, text="RIGHT IR (AN12)", bg=BG_COLOR, fg=ACCENT_R,
                 font=("Segoe UI", 11, "bold")).pack(pady=(5, 0))
        self.lbl_right_adc = tk.Label(rp, text="0", bg=BG_COLOR, fg=FG_COLOR,
                                       font=("Consolas", 28, "bold"))
        self.lbl_right_adc.pack()
        self.lbl_right_volt = tk.Label(rp, text="0.000 V", bg=BG_COLOR, fg=FG_COLOR,
                                        font=("Consolas", 11))
        self.lbl_right_volt.pack()
        self.lbl_right_dist = tk.Label(rp, text="-- cm", bg=BG_COLOR, fg=ACCENT_U,
                                        font=("Consolas", 18, "bold"))
        self.lbl_right_dist.pack(pady=(3, 0))
        self.right_gauge = tk.Canvas(rp, height=14, bg=CHART_BG, highlightthickness=0)
        self.right_gauge.pack(fill=tk.X, padx=12, pady=(6, 0))

    # ════════════════════════════════════════════════════════════════════════
    # Chart
    # ════════════════════════════════════════════════════════════════════════
    def _setup_chart(self):
        self.ax.set_facecolor(CHART_BG)
        self.ax.set_xlim(0, HISTORY_SIZE)
        self.ax.set_ylim(0, 200)
        self.ax.set_ylabel("Distance (cm)", color=FG_COLOR, fontsize=9)
        self.ax.set_xlabel("Samples", color=FG_COLOR, fontsize=9)
        self.ax.tick_params(colors=FG_COLOR, labelsize=8)
        self.ax.grid(True, color=GRID_COLOR, alpha=0.5, linestyle="--")
        for spine in self.ax.spines.values():
            spine.set_color(GRID_COLOR)

        self.ax.axhspan(0, 20, alpha=0.08, color=DANGER_COLOR)
        self.ax.axhline(y=20, color=DANGER_COLOR, alpha=0.3, linestyle="--", linewidth=1)

        self.line_left, = self.ax.plot([], [], color=ACCENT_L, linewidth=1.5,
                                        label="IR Left", alpha=0.9)
        self.line_right, = self.ax.plot([], [], color=ACCENT_R, linewidth=1.5,
                                         label="IR Right", alpha=0.9)
        self.line_us, = self.ax.plot([], [], color=ACCENT_U, linewidth=2.5,
                                      label="Ultrasonic", alpha=0.9)
        self.ax.legend(loc="upper right", facecolor=CHART_BG, edgecolor=GRID_COLOR,
                       labelcolor=FG_COLOR, fontsize=8)
        self.fig.tight_layout(pad=2)

    def _start_animation(self):
        self.ani = animation.FuncAnimation(self.fig, self._update_chart,
                                           interval=100, blit=False,
                                           cache_frame_data=False)

    def _update_chart(self, frame):
        left_v = adc_to_voltage(self.ir_left)
        right_v = adc_to_voltage(self.ir_right)
        left_dist = voltage_to_distance_cm(left_v)
        right_dist = voltage_to_distance_cm(right_v)
        us_dist = float(self.us_dist)

        self.left_history.append(left_dist)
        self.right_history.append(right_dist)
        self.us_history.append(us_dist)

        x = list(range(HISTORY_SIZE))
        self.line_left.set_data(x, list(self.left_history))
        self.line_right.set_data(x, list(self.right_history))
        self.line_us.set_data(x, list(self.us_history))

        # Update labels
        self.lbl_left_adc.configure(text=str(self.ir_left))
        self.lbl_left_volt.configure(text=f"{left_v:.3f} V")
        self.lbl_left_dist.configure(text=f"{left_dist:.1f} cm")
        self._draw_gauge(self.left_gauge, left_dist, 80, ACCENT_L)

        self.lbl_right_adc.configure(text=str(self.ir_right))
        self.lbl_right_volt.configure(text=f"{right_v:.3f} V")
        self.lbl_right_dist.configure(text=f"{right_dist:.1f} cm")
        self._draw_gauge(self.right_gauge, right_dist, 80, ACCENT_R)

        self._draw_car(left_dist, right_dist, us_dist)

        self._rx_var.set(f"RX: {self.rx_count}")

        self.canvas.draw_idle()
        return [self.line_left, self.line_right, self.line_us]

    def _draw_gauge(self, canvas, dist, max_dist, color):
        canvas.delete("all")
        w = canvas.winfo_width()
        h = canvas.winfo_height()
        if w <= 1:
            return
        ratio = min(dist / max_dist, 1.0)
        bar_w = int(w * ratio)
        bar_color = DANGER_COLOR if dist < 20 else (WARN_COLOR if dist < 40 else color)
        canvas.create_rectangle(0, 0, w, h, fill=GRID_COLOR, outline="")
        if bar_w > 0:
            canvas.create_rectangle(0, 0, bar_w, h, fill=bar_color, outline="")

    def _draw_car(self, left_dist, right_dist, us_dist):
        c = self.car_canvas
        c.delete("all")
        w = c.winfo_width()
        h = c.winfo_height()
        if w <= 1:
            return

        cx, cy = w // 2, h // 2 + 25
        car_w, car_h = 80, 120

        # Car body
        c.create_rectangle(cx - car_w // 2, cy - car_h // 2,
                           cx + car_w // 2, cy + car_h // 2,
                           fill="#45475a", outline="#585b70", width=2)
        c.create_text(cx, cy + 8, text="TC237", fill=FG_COLOR,
                      font=("Consolas", 11, "bold"))

        # Wheels
        for dy in [-40, 40]:
            c.create_rectangle(cx - car_w // 2 - 8, cy + dy - 12,
                               cx - car_w // 2, cy + dy + 12,
                               fill="#6c7086", outline="")
            c.create_rectangle(cx + car_w // 2, cy + dy - 12,
                               cx + car_w // 2 + 8, cy + dy + 12,
                               fill="#6c7086", outline="")

        # Ultrasonic cone
        us_len = max(20, min(90, us_dist * 0.8))
        us_color = DANGER_COLOR if us_dist < 20 else (WARN_COLOR if us_dist < 50 else ACCENT_U)
        front_y = cy - car_h // 2
        cone_w = 45

        c.create_polygon(cx - 18, front_y, cx + 18, front_y,
                         cx + cone_w, front_y - us_len,
                         cx - cone_w, front_y - us_len,
                         fill="", outline=us_color, width=2)
        for i in range(3):
            off = i * us_len // 4
            c.create_line(cx - 18 - off // 2, front_y - off,
                          cx + 18 + off // 2, front_y - off,
                          fill=us_color, width=1, dash=(4, 4))

        c.create_rectangle(cx - 20, front_y - 5, cx + 20, front_y + 3,
                           fill="#585b70", outline=us_color, width=1)
        c.create_text(cx, front_y - us_len - 15,
                      text=f"{us_dist:.0f} cm", fill=us_color,
                      font=("Consolas", 14, "bold"))
        c.create_text(cx, front_y - us_len - 32,
                      text="ULTRASONIC", fill=us_color, font=("Segoe UI", 8))

        # IR beams
        ir_angle = math.radians(45)
        spread = 13

        left_len = max(15, min(80, left_dist))
        lc = DANGER_COLOR if left_dist < 20 else (WARN_COLOR if left_dist < 40 else ACCENT_L)
        lx0, ly0 = cx - car_w // 2, front_y
        lx1 = lx0 - left_len * math.sin(ir_angle)
        ly1 = ly0 - left_len * math.cos(ir_angle)
        c.create_line(lx0, ly0, lx1 - spread * 0.3, ly1 - spread * 0.3,
                      fill=lc, width=2, arrow=tk.LAST)
        c.create_line(lx0, ly0, lx1 + spread * 0.5, ly1 - spread * 0.5,
                      fill=lc, width=2, arrow=tk.LAST)
        c.create_text(lx1 - 5, ly1 - 18, text=f"{left_dist:.0f}cm",
                      fill=lc, font=("Consolas", 11, "bold"))

        right_len = max(15, min(80, right_dist))
        rc = DANGER_COLOR if right_dist < 20 else (WARN_COLOR if right_dist < 40 else ACCENT_R)
        rx0, ry0 = cx + car_w // 2, front_y
        rx1 = rx0 + right_len * math.sin(ir_angle)
        ry1 = ry0 - right_len * math.cos(ir_angle)
        c.create_line(rx0, ry0, rx1 + spread * 0.3, ry1 - spread * 0.3,
                      fill=rc, width=2, arrow=tk.LAST)
        c.create_line(rx0, ry0, rx1 - spread * 0.5, ry1 - spread * 0.5,
                      fill=rc, width=2, arrow=tk.LAST)
        c.create_text(rx1 + 5, ry1 - 18, text=f"{right_dist:.0f}cm",
                      fill=rc, font=("Consolas", 11, "bold"))

        c.create_text(cx, cy - car_h // 2 - 8, text="FRONT",
                      fill="#6c7086", font=("Segoe UI", 8))

    # ════════════════════════════════════════════════════════════════════════
    # Key Bindings (Controller)
    # ════════════════════════════════════════════════════════════════════════
    def _bind_keys(self):
        self.root.bind('<KeyPress-Up>',    lambda e: self._key_press('Up'))
        self.root.bind('<KeyPress-Down>',  lambda e: self._key_press('Down'))
        self.root.bind('<KeyPress-Left>',  lambda e: self._key_press('Left'))
        self.root.bind('<KeyPress-Right>', lambda e: self._key_press('Right'))
        self.root.bind('<KeyRelease-Up>',    lambda e: self._key_release('Up'))
        self.root.bind('<KeyRelease-Down>',  lambda e: self._key_release('Down'))
        self.root.bind('<KeyRelease-Left>',  lambda e: self._key_release('Left'))
        self.root.bind('<KeyRelease-Right>', lambda e: self._key_release('Right'))
        for i in range(1, 10):
            self.root.bind(str(i), lambda e, n=i: self._set_speed(n))
        self.root.bind('<Escape>', lambda e: self.on_close())

    def _key_press(self, direction):
        if not self.connected:
            return
        if self.active_key == direction:
            return
        self.active_key = direction
        self._update_dpad(direction)
        self._lbl_cmd.configure(text=CMD_NAME.get(direction, "?"), fg=PHOS)
        if not self.sending:
            self.sending = True
            threading.Thread(target=self._send_loop, daemon=True).start()

    def _key_release(self, direction):
        if self.active_key == direction:
            self.active_key = None
            self.sending = False
            self._update_dpad(None)
            self._lbl_cmd.configure(text="STOP", fg=TEXT_DIM)

    def _send_loop(self):
        while self.sending and self.connected:
            key = self.active_key
            if key and key in ARROW_MAP:
                try:
                    self.ser.write(ARROW_MAP[key])
                except Exception:
                    break
            else:
                break
            time.sleep(SEND_INTERVAL)
        self.sending = False

    def _update_dpad(self, active_dir):
        for d, btn in self._dpad_btns.items():
            if d == active_dir:
                btn.configure(bg=PHOS, fg=PANEL_BG)
            else:
                btn.configure(bg="#162035", fg=FG_COLOR)

    def _set_speed(self, n):
        if n < 1: n = 1
        if n > 9: n = 9
        self.speed = n
        for i, btn in self._speed_btns.items():
            if i == n:
                btn.configure(bg=PHOS, fg=PANEL_BG)
            else:
                btn.configure(bg="#162035", fg=FG_COLOR)
        self._lbl_speed.configure(text=f"{n * 10}%")
        if self.connected:
            try:
                self.ser.write(str(n).encode())
            except Exception:
                pass

    # ════════════════════════════════════════════════════════════════════════
    # Serial
    # ════════════════════════════════════════════════════════════════════════
    def _refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        names = [p.device for p in ports]
        self._combo["values"] = names
        if names:
            for i, p in enumerate(ports):
                if any(k in p.description for k in USB_KW):
                    self._combo.current(i)
                    return
            self._combo.current(0)

    def _toggle_connect(self):
        if self.connected:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        port = self._combo.get().strip()
        if not port:
            messagebox.showwarning("Warning", "COM port to select.")
            return
        baud = int(self._baud_combo.get())
        try:
            self.ser = serial.Serial(port, baud, timeout=1)
        except Exception as e:
            messagebox.showerror("Error", f"Cannot open {port}\n{e}")
            return

        self.connected = True
        self.rx_count = 0
        self._btn_conn.configure(text="Disconnect", fg=DANGER_COLOR)
        self._status_var.set(f"{port} @ {baud}")
        self._led_cv.itemconfigure(self._led, fill=PHOS)
        self._combo.configure(state="disabled")
        self._baud_combo.configure(state="disabled")
        self.root.focus_set()

        threading.Thread(target=self._read_serial, daemon=True).start()

    def _disconnect(self):
        self.connected = False
        self.sending = False
        self.active_key = None
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None
        self._btn_conn.configure(text="Connect", fg=FG_COLOR)
        self._status_var.set("Disconnected")
        self._led_cv.itemconfigure(self._led, fill=TEXT_DIM)
        self._combo.configure(state="readonly")
        self._baud_combo.configure(state="readonly")
        self._update_dpad(None)
        self._lbl_cmd.configure(text="STOP", fg=TEXT_DIM)

    def _read_serial(self):
        buffer = ""
        while self.connected and self.ser and self.ser.is_open:
            try:
                raw = self.ser.read(self.ser.in_waiting or 1)
                if not raw:
                    continue
                buffer += raw.decode("ascii", errors="ignore")
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.strip()
                    if line.startswith("L:") and ",R:" in line and ",U:" in line:
                        try:
                            parts = line.split(",")
                            self.ir_left = int(parts[0][2:])
                            self.ir_right = int(parts[1][2:])
                            self.us_dist = int(parts[2][2:])
                            self.rx_count += 1
                        except (ValueError, IndexError):
                            pass
            except Exception:
                break

        self.connected = False
        try:
            self.root.after(0, lambda: self._status_var.set("Disconnected"))
            self.root.after(0, lambda: self._btn_conn.configure(text="Connect", fg=FG_COLOR))
            self.root.after(0, lambda: self._led_cv.itemconfigure(self._led, fill=TEXT_DIM))
        except tk.TclError:
            pass

    # ════════════════════════════════════════════════════════════════════════
    def on_close(self):
        self.connected = False
        self.sending = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = RcPanel(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
