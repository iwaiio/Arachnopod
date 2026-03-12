import os
import tkinter as tk
from tkinter import ttk
from pathlib import Path

REFRESH_MS = 500
MAX_LINES = 2000
TAIL_MAX_BYTES = 200_000

SYSTEM_LOGS = [
    ("CS", "cs"),
    ("PSS", "pss"),
    ("TCS", "tcs"),
    ("TMS", "tms"),
    ("MNS", "mns"),
    ("LS", "ls"),
]

MODEL_LOGS = [
    ("SIM_CS", "sim_cs"),
    ("SIM_PSS", "sim_pss"),
    ("SIM_TCS", "sim_tcs"),
    ("SIM_TMS", "sim_tms"),
    ("SIM_MNS", "sim_mns"),
    ("SIM_INM", "sim_inm"),
    ("SIM_DCM", "sim_dcm"),
    ("SIM_NM", "sim_nm"),
    ("SIM_CVM", "sim_cvm"),
    ("SIM_AM", "sim_am"),
    ("SIM_LS", "sim_ls"),
]

EXTRA_LOGS = [
    ("BUS", "bus"),
    ("CONTROL", "control"),
]


def resolve_log_dir() -> Path:
    env = os.getenv("ARP_LOG_DIR")
    if env:
        return Path(env)

    here = Path(__file__).resolve()
    for base in [here] + list(here.parents):
        candidate = base / "software" / "sim" / "log"
        if candidate.is_dir():
            return candidate

    cwd = Path.cwd().resolve()
    for base in [cwd] + list(cwd.parents):
        candidate = base / "software" / "sim" / "log"
        if candidate.is_dir():
            return candidate

    return Path("software") / "sim" / "log"


def resolve_control_file(log_dir: Path) -> Path:
    env = os.getenv("ARP_CONTROL_FILE")
    if env and env != "0":
        return Path(env)
    return log_dir / "control_in.txt"


class LogTail:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.offset = 0
        self.initialized = False

    def _read_tail(self) -> str:
        if not self.path.exists():
            self.offset = 0
            return ""

        size = self.path.stat().st_size
        if size == 0:
            self.offset = 0
            return ""

        start = 0
        if size > TAIL_MAX_BYTES:
            start = size - TAIL_MAX_BYTES

        with self.path.open("r", encoding="utf-8", errors="replace") as f:
            if start > 0:
                f.seek(start)
            data = f.read()

        self.offset = size
        return data

    def read_new(self) -> str:
        if not self.initialized:
            self.initialized = True
            return self._read_tail()

        if not self.path.exists():
            self.offset = 0
            return ""

        size = self.path.stat().st_size
        if size < self.offset:
            self.offset = 0
            return self._read_tail()

        if size == self.offset:
            return ""

        with self.path.open("r", encoding="utf-8", errors="replace") as f:
            f.seek(self.offset)
            data = f.read()
            self.offset = f.tell()
        return data


class LogTab:
    def __init__(self, parent: ttk.Notebook, title: str, path: Path) -> None:
        self.frame = ttk.Frame(parent)
        parent.add(self.frame, text=title)

        header = ttk.Frame(self.frame)
        header.pack(side=tk.TOP, fill=tk.X)

        self.path_label = ttk.Label(header, text=str(path))
        self.path_label.pack(side=tk.LEFT, padx=6, pady=4)

        self.text = tk.Text(self.frame, wrap="none", height=20)
        self.text.configure(state="disabled")

        y_scroll = ttk.Scrollbar(self.frame, orient="vertical", command=self.text.yview)
        x_scroll = ttk.Scrollbar(self.frame, orient="horizontal", command=self.text.xview)
        self.text.configure(yscrollcommand=y_scroll.set, xscrollcommand=x_scroll.set)

        self.text.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        y_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        x_scroll.pack(side=tk.BOTTOM, fill=tk.X)

        self.tail = LogTail(path)

    def append(self, text: str) -> None:
        if not text:
            return
        self.text.configure(state="normal")
        self.text.insert("end", text)

        lines = int(self.text.index("end-1c").split(".")[0])
        if lines > MAX_LINES:
            cut = lines - MAX_LINES
            self.text.delete("1.0", f"{cut}.0")

        self.text.see("end")
        self.text.configure(state="disabled")

    def update(self) -> None:
        self.append(self.tail.read_new())


class ControlTab:
    def __init__(self, parent: ttk.Notebook, control_file: Path) -> None:
        self.frame = ttk.Frame(parent)
        parent.add(self.frame, text="CONTROL")

        self.control_file = control_file

        info = ttk.Label(self.frame, text=f"Control file: {control_file}")
        info.pack(side=tk.TOP, anchor="w", padx=6, pady=4)

        form = ttk.Frame(self.frame)
        form.pack(side=tk.TOP, fill=tk.X, padx=6, pady=6)

        self.target_var = tk.StringVar(value="cs")
        self.action_var = tk.StringVar(value="exchange")
        self.arg0_var = tk.StringVar(value="pss")
        self.arg1_var = tk.StringVar()
        self.value_var = tk.StringVar()
        self.raw_var = tk.StringVar()

        ttk.Label(form, text="Target").grid(row=0, column=0, sticky="w")
        ttk.Combobox(form, textvariable=self.target_var, values=[
            "cs", "pss", "tcs", "tms", "mns", "ls", "any"
        ], width=10).grid(row=0, column=1, sticky="w", padx=6)

        ttk.Label(form, text="Action").grid(row=0, column=2, sticky="w")
        ttk.Combobox(form, textvariable=self.action_var, values=[
            "exchange", "cmd", "enable", "disable", "help"
        ], width=12).grid(row=0, column=3, sticky="w", padx=6)

        ttk.Label(form, text="Arg0").grid(row=1, column=0, sticky="w")
        ttk.Entry(form, textvariable=self.arg0_var, width=12).grid(row=1, column=1, sticky="w", padx=6)

        ttk.Label(form, text="Arg1").grid(row=1, column=2, sticky="w")
        ttk.Entry(form, textvariable=self.arg1_var, width=20).grid(row=1, column=3, sticky="w", padx=6)

        ttk.Label(form, text="Value").grid(row=2, column=0, sticky="w")
        ttk.Entry(form, textvariable=self.value_var, width=12).grid(row=2, column=1, sticky="w", padx=6)

        ttk.Label(form, text="Raw").grid(row=2, column=2, sticky="w")
        ttk.Entry(form, textvariable=self.raw_var, width=40).grid(row=2, column=3, sticky="w", padx=6)

        btns = ttk.Frame(self.frame)
        btns.pack(side=tk.TOP, fill=tk.X, padx=6, pady=6)

        send_btn = ttk.Button(btns, text="Send", command=self.send)
        send_btn.pack(side=tk.LEFT)

        self.status = ttk.Label(btns, text="")
        self.status.pack(side=tk.LEFT, padx=8)

    def build_command(self) -> str:
        raw = self.raw_var.get().strip()
        if raw:
            return raw

        parts = []
        target = self.target_var.get().strip()
        if target and target != "any":
            parts.append(target)

        action = self.action_var.get().strip()
        if not action:
            return ""
        parts.append(action)

        arg0 = self.arg0_var.get().strip()
        arg1 = self.arg1_var.get().strip()
        value = self.value_var.get().strip()

        if action in ("exchange", "ex"):
            if arg0:
                parts.append(arg0)
        elif action == "cmd":
            if arg0:
                parts.append(arg0)
            if arg1:
                parts.append(arg1)
            if value:
                parts.append(f"value={value}")
        elif action in ("enable", "disable"):
            if arg0:
                parts.append(arg0)
        elif action == "help":
            pass

        return " ".join(parts)

    def send(self) -> None:
        command = self.build_command()
        if not command:
            self.status.configure(text="empty command")
            return

        try:
            self.control_file.parent.mkdir(parents=True, exist_ok=True)
            with self.control_file.open("a", encoding="utf-8") as f:
                f.write(command + "\n")
            self.status.configure(text=f"sent: {command}")
        except OSError as exc:
            self.status.configure(text=f"error: {exc}")


class WatcherApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Arachnopod Watcher")

        self.log_dir = resolve_log_dir()
        self.control_file = resolve_control_file(self.log_dir)

        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill=tk.BOTH, expand=True)

        self.tabs = []

        for title, name in SYSTEM_LOGS:
            path = self.log_dir / f"{name}_log.log"
            self.tabs.append(LogTab(self.notebook, title, path))

        for title, name in MODEL_LOGS:
            path = self.log_dir / f"{name}_log.log"
            self.tabs.append(LogTab(self.notebook, title, path))

        for title, name in EXTRA_LOGS:
            path = self.log_dir / f"{name}_log.log"
            self.tabs.append(LogTab(self.notebook, title, path))

        self.control_tab = ControlTab(self.notebook, self.control_file)

        self.root.after(REFRESH_MS, self.update_all)

    def update_all(self) -> None:
        for tab in self.tabs:
            tab.update()
        self.root.after(REFRESH_MS, self.update_all)


def main() -> None:
    root = tk.Tk()
    app = WatcherApp(root)
    root.geometry("1200x800")
    root.mainloop()


if __name__ == "__main__":
    main()
