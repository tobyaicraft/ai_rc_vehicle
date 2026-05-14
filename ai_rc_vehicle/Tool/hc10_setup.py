"""
HC-10 BLE 모듈 AT 명령어 설정 도구
──────────────────────────────────────────────────────────────────────────────
USB-Serial 어댑터로 HC-10에 연결하여 AT 명령으로 BLE 설정.

사용법:
    1. HC-10 모듈을 USB-Serial 어댑터에 연결
       - VCC -> 3.3V
       - GND -> GND
       - TXD -> 어댑터 RX
       - RXD -> 어댑터 TX
    2. 이 스크립트 실행
    3. [자동 설정] 버튼으로 Slave 모드 + 이름 설정
       또는 수동으로 AT 명령 입력
"""

import serial
import serial.tools.list_ports
import threading
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext

# ── Theme ────────────────────────────────────────────────────────────────────
BG       = "#0a0e1a"
PANEL_BG = "#0c1120"
PHOS     = "#00ff88"
TEXT_HI  = "#cce8ff"
TEXT_DIM = "#3a5070"
WARN     = "#ff4422"
CYAN     = "#00ccff"
YELLOW   = "#ffcc00"

USB_KW = ("PL2303", "Prolific", "CH340", "FTDI", "Silicon", "CP210", "USB Serial")

# ── AT 명령 목록 (자동 설정용) ────────────────────────────────────────────────
AUTO_SETUP_CMDS = [
    ("AT",            "통신 확인"),
    ("AT+NAMEBLE",    "이름을 'BLE'로 설정"),
    ("AT+ROLE0",      "Slave 모드 설정 (스마트폰=Master)"),
    ("AT+ADDR",       "MAC 주소 확인"),
    ("AT+ROLE",       "역할 확인"),
    ("AT+NAME",       "이름 확인"),
    ("AT+BAUD",       "보드레이트 확인"),
]


class Hc10SetupApp:
    def __init__(self):
        self.ser = None
        self.running = True

        self.root = tk.Tk()
        self.root.title("HC-10 BLE Setup Tool")
        self.root.configure(bg=BG)
        self.root.geometry("700x600")
        self.root.resizable(True, True)

        self._build_ui()
        self._refresh_ports()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ── UI ────────────────────────────────────────────────────────────────────
    def _build_ui(self):
        # -- 연결 패널 --
        conn_frame = tk.Frame(self.root, bg=PANEL_BG, padx=10, pady=8)
        conn_frame.pack(fill='x', padx=8, pady=(8, 4))

        tk.Label(conn_frame, text="PORT", bg=PANEL_BG, fg=TEXT_DIM,
                 font=("Consolas", 9)).grid(row=0, column=0, sticky='w')
        self.port_cb = ttk.Combobox(conn_frame, width=22, state='readonly',
                                     font=("Consolas", 10))
        self.port_cb.grid(row=0, column=1, padx=(4, 8))

        tk.Label(conn_frame, text="BAUD", bg=PANEL_BG, fg=TEXT_DIM,
                 font=("Consolas", 9)).grid(row=0, column=2, sticky='w')
        self.baud_cb = ttk.Combobox(conn_frame, width=8, state='readonly',
                                     font=("Consolas", 10),
                                     values=["9600", "19200", "38400", "57600", "115200"])
        self.baud_cb.set("9600")
        self.baud_cb.grid(row=0, column=3, padx=(4, 8))

        self.btn_refresh = tk.Button(conn_frame, text="Refresh", bg=PANEL_BG,
                                      fg=CYAN, font=("Consolas", 9, "bold"),
                                      relief='flat', command=self._refresh_ports)
        self.btn_refresh.grid(row=0, column=4, padx=4)

        self.btn_connect = tk.Button(conn_frame, text="Connect", bg=PANEL_BG,
                                      fg=PHOS, font=("Consolas", 10, "bold"),
                                      relief='flat', command=self._toggle_connect)
        self.btn_connect.grid(row=0, column=5, padx=4)

        self.status_label = tk.Label(conn_frame, text="Disconnected", bg=PANEL_BG,
                                      fg=WARN, font=("Consolas", 9))
        self.status_label.grid(row=0, column=6, padx=(8, 0))

        # -- 로그 창 --
        log_frame = tk.Frame(self.root, bg=BG)
        log_frame.pack(fill='both', expand=True, padx=8, pady=4)

        self.log_text = scrolledtext.ScrolledText(
            log_frame, bg="#050810", fg=TEXT_HI, insertbackground=PHOS,
            font=("Consolas", 10), wrap='word', state='disabled',
            relief='flat', borderwidth=2)
        self.log_text.pack(fill='both', expand=True)

        self.log_text.tag_configure("tx", foreground=YELLOW)
        self.log_text.tag_configure("rx", foreground=PHOS)
        self.log_text.tag_configure("info", foreground=CYAN)
        self.log_text.tag_configure("error", foreground=WARN)

        # -- 수동 명령 입력 --
        cmd_frame = tk.Frame(self.root, bg=PANEL_BG, padx=10, pady=8)
        cmd_frame.pack(fill='x', padx=8, pady=4)

        tk.Label(cmd_frame, text="AT CMD:", bg=PANEL_BG, fg=TEXT_DIM,
                 font=("Consolas", 9)).pack(side='left')
        self.cmd_entry = tk.Entry(cmd_frame, bg="#050810", fg=TEXT_HI,
                                   insertbackground=PHOS, font=("Consolas", 11),
                                   relief='flat', width=30)
        self.cmd_entry.pack(side='left', padx=(4, 8), fill='x', expand=True)
        self.cmd_entry.insert(0, "AT")
        self.cmd_entry.bind('<Return>', lambda e: self._send_manual())

        self.btn_send = tk.Button(cmd_frame, text="Send", bg=PANEL_BG,
                                   fg=PHOS, font=("Consolas", 10, "bold"),
                                   relief='flat', command=self._send_manual)
        self.btn_send.pack(side='left', padx=4)

        # -- 빠른 명령 버튼 --
        quick_frame = tk.Frame(self.root, bg=PANEL_BG, padx=10, pady=8)
        quick_frame.pack(fill='x', padx=8, pady=(4, 8))

        self.btn_auto = tk.Button(quick_frame, text=">>> 자동 설정 (Slave + 이름) <<<",
                                   bg="#1a2040", fg=PHOS,
                                   font=("Consolas", 11, "bold"),
                                   relief='flat', command=self._auto_setup)
        self.btn_auto.pack(fill='x', pady=(0, 6))

        btn_row = tk.Frame(quick_frame, bg=PANEL_BG)
        btn_row.pack(fill='x')

        quick_cmds = [
            ("AT", "AT"),
            ("이름확인", "AT+NAME"),
            ("역할확인", "AT+ROLE"),
            ("주소확인", "AT+ADDR"),
            ("보드레이트", "AT+BAUD"),
            ("버전", "AT+VERSION"),
        ]
        for label, cmd in quick_cmds:
            b = tk.Button(btn_row, text=label, bg="#0c1120", fg=CYAN,
                          font=("Consolas", 9), relief='flat',
                          command=lambda c=cmd: self._send_at(c))
            b.pack(side='left', padx=2, expand=True, fill='x')

    # ── 포트 ──────────────────────────────────────────────────────────────────
    def _refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        port_list = []
        auto_select = None
        for p in ports:
            desc = f"{p.device} - {p.description}"
            port_list.append(desc)
            if any(kw.lower() in (p.description or "").lower() for kw in USB_KW):
                auto_select = desc
        self.port_cb['values'] = port_list
        if auto_select:
            self.port_cb.set(auto_select)
        elif port_list:
            self.port_cb.set(port_list[0])

    def _toggle_connect(self):
        if self.ser and self.ser.is_open:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        sel = self.port_cb.get()
        if not sel:
            messagebox.showwarning("Warning", "포트를 선택하세요")
            return
        port = sel.split(" - ")[0].strip()
        baud = int(self.baud_cb.get())
        try:
            self.ser = serial.Serial(port, baud, timeout=0.5)
            self.btn_connect.configure(text="Disconnect", fg=WARN)
            self.status_label.configure(text=f"Connected ({port})", fg=PHOS)
            self._log(f"Serial port {port} opened @ {baud} bps\n", "info")
        except Exception as e:
            self._log(f"Connection failed: {e}\n", "error")

    def _disconnect(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.ser = None
        self.btn_connect.configure(text="Connect", fg=PHOS)
        self.status_label.configure(text="Disconnected", fg=WARN)
        self._log("Disconnected\n", "info")

    # ── AT 명령 전송 ─────────────────────────────────────────────────────────
    def _send_at(self, cmd):
        if not self.ser or not self.ser.is_open:
            self._log("Not connected!\n", "error")
            return None

        self._log(f"TX: {cmd}\n", "tx")
        self.ser.write((cmd + "\r\n").encode())
        self.ser.flush()

        # 응답 대기
        import time
        time.sleep(0.3)
        response = ""
        while self.ser.in_waiting:
            chunk = self.ser.read(self.ser.in_waiting).decode(errors='replace')
            response += chunk
            time.sleep(0.05)

        if response.strip():
            self._log(f"RX: {response.strip()}\n", "rx")
        else:
            self._log("RX: (no response)\n", "error")

        return response.strip()

    def _send_manual(self):
        cmd = self.cmd_entry.get().strip()
        if cmd:
            self._send_at(cmd)

    # ── 자동 설정 ────────────────────────────────────────────────────────────
    def _auto_setup(self):
        if not self.ser or not self.ser.is_open:
            self._log("Not connected! 먼저 연결하세요.\n", "error")
            return

        self._log("\n========== HC-10 자동 설정 시작 ==========\n", "info")

        def run():
            import time
            for cmd, desc in AUTO_SETUP_CMDS:
                self._log(f"\n--- {desc} ---\n", "info")
                self._send_at(cmd)
                time.sleep(0.3)

            self._log("\n========== 설정 완료! ==========\n", "info")
            self._log("HC-10을 MCU (P20.9/P20.10)에 연결하세요.\n", "info")

        threading.Thread(target=run, daemon=True).start()

    # ── 로그 ──────────────────────────────────────────────────────────────────
    def _log(self, msg, tag=None):
        def _append():
            self.log_text.configure(state='normal')
            self.log_text.insert('end', msg, tag)
            self.log_text.see('end')
            self.log_text.configure(state='disabled')
        self.root.after(0, _append)

    # ── 종료 ──────────────────────────────────────────────────────────────────
    def _on_close(self):
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.root.destroy()

    def run(self):
        self.root.mainloop()


if __name__ == '__main__':
    app = Hc10SetupApp()
    app.run()
