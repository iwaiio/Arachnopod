import os
import re
import tkinter as tk
from pathlib import Path
from tkinter import ttk

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

CS_TARGETS = ["pss", "tcs", "tms", "mns", "ls"]


def resolve_repo_root() -> Path:
    here = Path(__file__).resolve()
    for base in [here] + list(here.parents):
        if (base / "software" / "sim").is_dir():
            return base

    cwd = Path.cwd().resolve()
    for base in [cwd] + list(cwd.parents):
        if (base / "software" / "sim").is_dir():
            return base

    return Path.cwd().resolve()


def resolve_log_dir() -> Path:
    env = os.getenv("ARP_LOG_DIR")
    if env:
        return Path(env)
    return resolve_repo_root() / "software" / "sim" / "log"


def resolve_control_file(log_dir: Path) -> Path:
    env = os.getenv("ARP_CONTROL_FILE")
    if env and env != "0":
        return Path(env)
    return log_dir / "control_in.txt"


def resolve_command_tab() -> Path:
    return resolve_repo_root() / "software" / "sim" / "sim" / "systems" / "base" / "command_tab.hpp"


def load_command_catalog(path: Path) -> dict[str, list[str]]:
    catalog = {target: [] for target in CS_TARGETS}
    if not path.exists():
        return catalog

    pattern = re.compile(r'\{SYS_([A-Z]+),.*?"([A-Z0-9_]+)",\s*ALG_')
    seen: dict[str, set[str]] = {target: set() for target in CS_TARGETS}

    with path.open("r", encoding="utf-8", errors="replace") as file:
        for line in file:
            match = pattern.search(line)
            if not match:
                continue

            target = match.group(1).lower()
            key = match.group(2)
            if target not in catalog:
                continue
            if key in seen[target]:
                continue

            catalog[target].append(key)
            seen[target].add(key)

    return catalog


class CommandSender:
    def __init__(self, control_file: Path) -> None:
        self.control_file = control_file

    def send(self, command: str) -> str:
        self.control_file.parent.mkdir(parents=True, exist_ok=True)
        with self.control_file.open("a", encoding="utf-8") as file:
            file.write(command + "\n")
        return command


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

        with self.path.open("r", encoding="utf-8", errors="replace") as file:
            if start > 0:
                file.seek(start)
            data = file.read()

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

        with self.path.open("r", encoding="utf-8", errors="replace") as file:
            file.seek(self.offset)
            data = file.read()
            self.offset = file.tell()
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


class RawControlTab:
    def __init__(self, parent: ttk.Notebook, sender: CommandSender) -> None:
        self.frame = ttk.Frame(parent)
        parent.add(self.frame, text="RAW CONTROL")

        self.sender = sender
        self.raw_var = tk.StringVar()
        self.status_var = tk.StringVar()

        info = ttk.Label(
            self.frame,
            text="Low-level access to control_in.txt. Use this when the dedicated CS UV panel is not enough.",
        )
        info.pack(side=tk.TOP, anchor="w", padx=8, pady=8)

        row = ttk.Frame(self.frame)
        row.pack(side=tk.TOP, fill=tk.X, padx=8, pady=8)

        ttk.Entry(row, textvariable=self.raw_var, width=100).pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Button(row, text="Send", command=self.send).pack(side=tk.LEFT, padx=8)

        ttk.Label(self.frame, textvariable=self.status_var).pack(side=tk.TOP, anchor="w", padx=8)

    def send(self) -> None:
        command = self.raw_var.get().strip()
        if not command:
            self.status_var.set("empty command")
            return

        try:
            sent = self.sender.send(command)
        except OSError as exc:
            self.status_var.set(f"error: {exc}")
            return

        self.status_var.set(f"sent: {sent}")


class CommandActionFrame:
    def __init__(
        self,
        parent: ttk.LabelFrame,
        sender: CommandSender,
        catalog: dict[str, list[str]],
        button_text: str,
        verb: str,
        status_var: tk.StringVar,
    ) -> None:
        self.sender = sender
        self.catalog = catalog
        self.verb = verb
        self.status_var = status_var

        self.target_var = tk.StringVar(value=CS_TARGETS[0])
        self.command_var = tk.StringVar()
        self.value_var = tk.StringVar()

        ttk.Label(parent, text="Target").grid(row=0, column=0, sticky="w", padx=6, pady=4)
        self.target_combo = ttk.Combobox(parent, textvariable=self.target_var, values=CS_TARGETS, width=10, state="readonly")
        self.target_combo.grid(row=0, column=1, sticky="w", padx=6, pady=4)
        self.target_combo.bind("<<ComboboxSelected>>", self._on_target_changed)

        ttk.Label(parent, text="Command").grid(row=0, column=2, sticky="w", padx=6, pady=4)
        self.command_combo = ttk.Combobox(parent, textvariable=self.command_var, width=28, state="readonly")
        self.command_combo.grid(row=0, column=3, sticky="w", padx=6, pady=4)

        ttk.Label(parent, text="Value").grid(row=0, column=4, sticky="w", padx=6, pady=4)
        ttk.Entry(parent, textvariable=self.value_var, width=14).grid(row=0, column=5, sticky="w", padx=6, pady=4)

        ttk.Button(parent, text=button_text, command=self.send).grid(row=0, column=6, sticky="w", padx=6, pady=4)

        self._refresh_commands()

    def _refresh_commands(self) -> None:
        target = self.target_var.get().strip().lower()
        commands = self.catalog.get(target, [])
        self.command_combo.configure(values=commands)
        if commands:
            if self.command_var.get() not in commands:
                self.command_var.set(commands[0])
        else:
            self.command_var.set("")

    def _on_target_changed(self, _event: object) -> None:
        self._refresh_commands()

    def send(self) -> None:
        target = self.target_var.get().strip().lower()
        command = self.command_var.get().strip()
        value = self.value_var.get().strip()

        if not target:
            self.status_var.set("target is required")
            return
        if not command:
            self.status_var.set("command key is required")
            return

        parts = ["cs", self.verb, target, command]
        if value:
            parts.append(f"value={value}")

        line = " ".join(parts)
        try:
            sent = self.sender.send(line)
        except OSError as exc:
            self.status_var.set(f"error: {exc}")
            return

        self.status_var.set(f"sent: {sent}")


class SimpleTargetActionFrame:
    def __init__(
        self,
        parent: ttk.LabelFrame,
        sender: CommandSender,
        button_text: str,
        verb: str,
        status_var: tk.StringVar,
    ) -> None:
        self.sender = sender
        self.verb = verb
        self.status_var = status_var
        self.target_var = tk.StringVar(value=CS_TARGETS[0])

        ttk.Label(parent, text="Target").grid(row=0, column=0, sticky="w", padx=6, pady=4)
        ttk.Combobox(parent, textvariable=self.target_var, values=CS_TARGETS, width=10, state="readonly").grid(
            row=0, column=1, sticky="w", padx=6, pady=4
        )
        ttk.Button(parent, text=button_text, command=self.send).grid(row=0, column=2, sticky="w", padx=6, pady=4)

    def send(self) -> None:
        target = self.target_var.get().strip().lower()
        if not target:
            self.status_var.set("target is required")
            return

        line = f"cs {self.verb} {target}"
        try:
            sent = self.sender.send(line)
        except OSError as exc:
            self.status_var.set(f"error: {exc}")
            return

        self.status_var.set(f"sent: {sent}")


class CsUvTab:
    def __init__(self, parent: ttk.Notebook, sender: CommandSender, catalog: dict[str, list[str]], control_file: Path) -> None:
        self.frame = ttk.Frame(parent)
        parent.add(self.frame, text="CS UV")

        self.sender = sender
        self.catalog = catalog
        self.status_var = tk.StringVar(value="ready")

        info = ttk.Label(
            self.frame,
            text=(
                "CS UV panel writes commands to the control module.\n"
                "Actions map to: exchange, cmd, stagecmd, sendcmd."
            ),
            justify="left",
        )
        info.pack(side=tk.TOP, anchor="w", padx=8, pady=8)

        file_label = ttk.Label(self.frame, text=f"Control file: {control_file}")
        file_label.pack(side=tk.TOP, anchor="w", padx=8)

        request_box = ttk.LabelFrame(self.frame, text="Request Data From System")
        request_box.pack(side=tk.TOP, fill=tk.X, padx=8, pady=8)
        SimpleTargetActionFrame(request_box, sender, "Request", "exchange", self.status_var)

        immediate_box = ttk.LabelFrame(self.frame, text="Send One Command Immediately")
        immediate_box.pack(side=tk.TOP, fill=tk.X, padx=8, pady=8)
        CommandActionFrame(immediate_box, sender, catalog, "Send Command", "cmd", self.status_var)

        stage_box = ttk.LabelFrame(self.frame, text="Add Command To CS MSG_CMD")
        stage_box.pack(side=tk.TOP, fill=tk.X, padx=8, pady=8)
        CommandActionFrame(stage_box, sender, catalog, "Stage Command", "stagecmd", self.status_var)

        send_array_box = ttk.LabelFrame(self.frame, text="Send CS MSG_CMD Window To System")
        send_array_box.pack(side=tk.TOP, fill=tk.X, padx=8, pady=8)
        SimpleTargetActionFrame(send_array_box, sender, "Send MSG_CMD", "sendcmd", self.status_var)

        ttk.Label(self.frame, textvariable=self.status_var).pack(side=tk.TOP, anchor="w", padx=8, pady=8)


class WatcherApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Arachnopod Watcher")

        self.log_dir = resolve_log_dir()
        self.control_file = resolve_control_file(self.log_dir)
        self.command_catalog = load_command_catalog(resolve_command_tab())
        self.sender = CommandSender(self.control_file)

        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill=tk.BOTH, expand=True)

        self.tabs: list[LogTab] = []

        self.cs_uv_tab = CsUvTab(self.notebook, self.sender, self.command_catalog, self.control_file)

        for title, name in SYSTEM_LOGS:
            path = self.log_dir / f"{name}_log.log"
            self.tabs.append(LogTab(self.notebook, title, path))

        for title, name in MODEL_LOGS:
            path = self.log_dir / f"{name}_log.log"
            self.tabs.append(LogTab(self.notebook, title, path))

        for title, name in EXTRA_LOGS:
            path = self.log_dir / f"{name}_log.log"
            self.tabs.append(LogTab(self.notebook, title, path))

        self.raw_control_tab = RawControlTab(self.notebook, self.sender)

        self.root.after(REFRESH_MS, self.update_all)

    def update_all(self) -> None:
        for tab in self.tabs:
            tab.update()
        self.root.after(REFRESH_MS, self.update_all)


def main() -> None:
    root = tk.Tk()
    WatcherApp(root)
    root.geometry("1280x860")
    root.mainloop()


if __name__ == "__main__":
    main()
