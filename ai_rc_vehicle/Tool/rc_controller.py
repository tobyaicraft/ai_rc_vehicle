"""
RC Vehicle 무선 컨트롤러 (HC-12 Serial)
──────────────────────────────────────────────────────────────────────────────
방향키로 4WD RC Vehicle 조종, 떼면 자동 정지.
Auto-detect USB-TTL adapters (PL2303 / CH340 / FTDI / CP210x)

조작:
    ↑  전진    ↓  후진
    ←  좌스핀  →  우스핀
    1~9  속도 (10%~90%)
"""

import time
import threading

import serial
import serial.tools.list_ports

import tkinter as tk
from tkinter import ttk, messagebox

# ── Constants ─────────────────────────────────────────────────────────────────
BAUD_RATE  = 115200
SEND_INTERVAL = 0.05   # 50ms 반복 전송 간격

# ── Dark theme ────────────────────────────────────────────────────────────────
BG         = "#0a0e1a"
PANEL_BG   = "#0c1120"
GRID_MAJ   = "#16202e"
PHOS       = "#00ff88"
TEXT_HI    = "#cce8ff"
TEXT_DIM   = "#3a5070"
WARN       = "#ff4422"
YELLOW     = "#ffcc00"
CYAN       = "#00ccff"

USB_KW = ("PL2303", "Prolific", "CH340", "FTDI", "Silicon", "CP210", "USB Serial")

# ── Arrow key → ESC sequence ──────────────────────────────────────────────────
ARROW_MAP = {
    'Up':    b'\x1b[A',
    'Down':  b'\x1b[B',
    'Right': b'\x1b[C',
    'Left':  b'\x1b[D',
}

CMD_NAME = {
    'Up':    '전진  ↑',
    'Down':  '후진  ↓',
    'Left':  '좌스핀 ←',
    'Right': '우스핀 →',
}


class RcControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("RC Vehicle Controller")
        self.root.configure(bg=BG)
        self.root.geometry("520x620")
        self.root.resizable(False, False)

        self.serial_port = None
        self.connected   = False
        self.active_key  = None       # 현재 눌린 방향키
        self.speed       = 5          # 기본 속도 단계 (1~9)
        self.send_thread = None
        self.sending     = False

        self._build_ui()
        self._bind_keys()
        self._refresh_ports()

    # ─────────────────────────────────────────────────────────────────────────
    # UI
    # ─────────────────────────────────────────────────────────────────────────
    def _build_ui(self):
        s = ttk.Style()
        s.theme_use("clam")
        s.configure("TCombobox",
                    fieldbackground="#0d1525", background="#0d1525",
                    foreground=TEXT_HI, selectbackground="#1a2840",
                    selectforeground=PHOS)

        self._build_topbar()
        self._build_status_panel()
        self._build_dpad()
        self._build_speed_panel()
        self._build_footer()

    def _build_topbar(self):
        bar = tk.Frame(self.root, bg="#060810", height=48)
        bar.pack(fill=tk.X)
        bar.pack_propagate(False)

        tk.Label(bar, text="RC Vehicle Controller",
                 bg="#060810", fg=PHOS,
                 font=("Consolas", 14, "bold")).pack(side=tk.LEFT, padx=16, pady=10)

        # Status LED
        self._led_cv = tk.Canvas(bar, width=14, height=14,
                                  bg="#060810", highlightthickness=0)
        self._led_cv.pack(side=tk.LEFT, padx=(8, 3))
        self._led = self._led_cv.create_oval(2, 2, 12, 12, fill=TEXT_DIM, outline="")

        self._status_var = tk.StringVar(value="Disconnected")
        tk.Label(bar, textvariable=self._status_var,
                 bg="#060810", fg=TEXT_DIM,
                 font=("Consolas", 10)).pack(side=tk.LEFT, padx=(2, 12))

    def _build_status_panel(self):
        row1 = tk.Frame(self.root, bg=BG)
        row1.pack(fill=tk.X, padx=16, pady=(10, 4))

        tk.Label(row1, text="Port:", bg=BG, fg=TEXT_DIM,
                 font=("Consolas", 10)).pack(side=tk.LEFT, padx=(0, 6))

        self._combo = ttk.Combobox(row1, width=30, state="readonly",
                                    font=("Consolas", 9))
        self._combo.pack(side=tk.LEFT, padx=4)

        btn_style = dict(relief=tk.FLAT, cursor="hand2",
                         font=("Consolas", 10, "bold"),
                         activebackground="#1e3050", activeforeground=PHOS)

        tk.Button(row1, text="⟳", width=3,
                  bg="#162035", fg=TEXT_HI, **btn_style,
                  command=self._refresh_ports).pack(side=tk.LEFT, padx=3)

        row2 = tk.Frame(self.root, bg=BG)
        row2.pack(fill=tk.X, padx=16, pady=(4, 4))

        tk.Label(row2, text="Baud:", bg=BG, fg=TEXT_DIM,
                 font=("Consolas", 10)).pack(side=tk.LEFT, padx=(0, 6))

        self._baud_combo = ttk.Combobox(row2, width=10, state="readonly",
                                         font=("Consolas", 9))
        self._baud_combo["values"] = ["9600", "19200", "38400", "57600", "115200"]
        self._baud_combo.set("115200")
        self._baud_combo.pack(side=tk.LEFT, padx=4)

        self._btn_conn = tk.Button(
            row2, text="Connect", width=11,
            bg="#162035", fg=TEXT_HI, **btn_style,
            command=self._toggle_connect)
        self._btn_conn.pack(side=tk.LEFT, padx=(12, 3))

    def _build_dpad(self):
        """방향키 D-pad 시각화"""
        frm = tk.Frame(self.root, bg=BG)
        frm.pack(pady=(20, 10))

        # 현재 명령 표시
        self._lbl_cmd = tk.Label(frm, text="STOP", bg=BG, fg=TEXT_DIM,
                                  font=("Consolas", 20, "bold"))
        self._lbl_cmd.pack(pady=(0, 14))

        # 3x3 그리드
        grid = tk.Frame(frm, bg=BG)
        grid.pack()

        btn_size = dict(width=6, height=2, relief=tk.FLAT,
                        font=("Consolas", 14, "bold"), cursor="hand2",
                        activebackground="#2a3850")

        # Row 0: 위
        tk.Label(grid, bg=BG, width=6).grid(row=0, column=0)
        self._btn_up = tk.Button(grid, text="▲", bg="#162035", fg=TEXT_HI,
                                  **btn_size)
        self._btn_up.grid(row=0, column=1, padx=3, pady=3)
        tk.Label(grid, bg=BG, width=6).grid(row=0, column=2)

        # Row 1: 좌, 정지, 우
        self._btn_left = tk.Button(grid, text="◄", bg="#162035", fg=TEXT_HI,
                                    **btn_size)
        self._btn_left.grid(row=1, column=0, padx=3, pady=3)

        self._btn_stop = tk.Button(grid, text="■", bg="#1a1020", fg=WARN,
                                    **btn_size)
        self._btn_stop.grid(row=1, column=1, padx=3, pady=3)

        self._btn_right = tk.Button(grid, text="►", bg="#162035", fg=TEXT_HI,
                                     **btn_size)
        self._btn_right.grid(row=1, column=2, padx=3, pady=3)

        # Row 2: 아래
        tk.Label(grid, bg=BG, width=6).grid(row=2, column=0)
        self._btn_down = tk.Button(grid, text="▼", bg="#162035", fg=TEXT_HI,
                                    **btn_size)
        self._btn_down.grid(row=2, column=1, padx=3, pady=3)
        tk.Label(grid, bg=BG, width=6).grid(row=2, column=2)

        self._dpad_btns = {
            'Up': self._btn_up, 'Down': self._btn_down,
            'Left': self._btn_left, 'Right': self._btn_right,
        }

    def _build_speed_panel(self):
        frm = tk.Frame(self.root, bg=BG)
        frm.pack(pady=(10, 6))

        tk.Label(frm, text="Speed:", bg=BG, fg=TEXT_DIM,
                 font=("Consolas", 10)).pack(side=tk.LEFT, padx=(0, 8))

        self._speed_btns = {}
        for i in range(1, 10):
            active = (i == self.speed)
            btn = tk.Button(
                frm, text=str(i), width=3, height=1,
                bg=PHOS if active else "#162035",
                fg=BG if active else TEXT_HI,
                relief=tk.FLAT, font=("Consolas", 10, "bold"), cursor="hand2",
                command=lambda n=i: self._set_speed(n))
            btn.pack(side=tk.LEFT, padx=1)
            self._speed_btns[i] = btn

        self._lbl_speed = tk.Label(frm, text=f" {self.speed * 10}%",
                                    bg=BG, fg=PHOS,
                                    font=("Consolas", 12, "bold"))
        self._lbl_speed.pack(side=tk.LEFT, padx=(10, 0))

    def _build_footer(self):
        tk.Frame(self.root, bg=GRID_MAJ, height=1).pack(fill=tk.X, padx=16, pady=(16, 6))
        tk.Label(self.root,
                 text="키보드 방향키로 조종  |  숫자 1~9 속도 변경  |  ESC 종료",
                 bg=BG, fg=TEXT_DIM, font=("Consolas", 9)).pack(pady=(0, 8))

    # ─────────────────────────────────────────────────────────────────────────
    # Key bindings
    # ─────────────────────────────────────────────────────────────────────────
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
            return  # 이미 누르고 있음

        self.active_key = direction
        self._update_dpad_visual(direction)
        self._lbl_cmd.configure(text=CMD_NAME.get(direction, "?"), fg=PHOS)

        # 반복 전송 시작
        if not self.sending:
            self.sending = True
            self.send_thread = threading.Thread(target=self._send_loop, daemon=True)
            self.send_thread.start()

    def _key_release(self, direction):
        if self.active_key == direction:
            self.active_key = None
            self.sending = False
            self._update_dpad_visual(None)
            self._lbl_cmd.configure(text="STOP", fg=TEXT_DIM)

    def _send_loop(self):
        """키가 눌려있는 동안 ESC 시퀀스 반복 전송 (50ms 간격)"""
        while self.sending and self.connected:
            key = self.active_key
            if key and key in ARROW_MAP:
                try:
                    self.serial_port.write(ARROW_MAP[key])
                except Exception:
                    break
            else:
                break
            time.sleep(SEND_INTERVAL)
        self.sending = False

    def _update_dpad_visual(self, active_dir):
        for d, btn in self._dpad_btns.items():
            if d == active_dir:
                btn.configure(bg=PHOS, fg=BG)
            else:
                btn.configure(bg="#162035", fg=TEXT_HI)

    # ─────────────────────────────────────────────────────────────────────────
    # Speed
    # ─────────────────────────────────────────────────────────────────────────
    def _set_speed(self, n):
        if n < 1: n = 1
        if n > 9: n = 9
        self.speed = n

        for i, btn in self._speed_btns.items():
            if i == n:
                btn.configure(bg=PHOS, fg=BG)
            else:
                btn.configure(bg="#162035", fg=TEXT_HI)

        self._lbl_speed.configure(text=f" {n * 10}%")

        # 속도 명령 전송
        if self.connected:
            try:
                self.serial_port.write(str(n).encode())
            except Exception:
                pass

    # ─────────────────────────────────────────────────────────────────────────
    # Serial
    # ─────────────────────────────────────────────────────────────────────────
    def _refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        descs = [f"{p.device}   {p.description}" for p in ports]
        self._combo["values"] = descs
        if descs:
            for i, d in enumerate(descs):
                if any(k in d for k in USB_KW):
                    self._combo.current(i)
                    return
            self._combo.current(0)

    def _toggle_connect(self):
        if self.connected:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        sel = self._combo.get()
        if not sel:
            messagebox.showwarning("Warning", "COM 포트를 선택하세요.")
            return
        port = sel.split()[0].strip()
        baud = int(self._baud_combo.get())
        try:
            self.serial_port = serial.Serial(port, baud, timeout=0.1)
            self.connected = True
            self._btn_conn.configure(text="Disconnect", fg=WARN)
            self._status_var.set(f"  {port}  @  {baud} bps")
            self._led_cv.itemconfigure(self._led, fill=PHOS)
            self._combo.configure(state="disabled")
            self._baud_combo.configure(state="disabled")
            self.root.focus_set()  # 포커스를 메인 윈도우로 이동
        except Exception as e:
            messagebox.showerror("Connect Error", f"{port} 열기 실패\n\n{e}")

    def _disconnect(self):
        self.connected = False
        self.sending = False
        self.active_key = None
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self._btn_conn.configure(text="Connect", fg=TEXT_HI)
        self._status_var.set("Disconnected")
        self._led_cv.itemconfigure(self._led, fill=TEXT_DIM)
        self._combo.configure(state="readonly")
        self._baud_combo.configure(state="readonly")
        self._update_dpad_visual(None)
        self._lbl_cmd.configure(text="STOP", fg=TEXT_DIM)

    # ─────────────────────────────────────────────────────────────────────────
    def on_close(self):
        self.connected = False
        self.sending = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = RcControllerApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
