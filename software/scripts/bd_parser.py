#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any
import math
import re
import sys

from openpyxl import load_workbook


SCRIPT_DIR = Path(__file__).resolve().parent
SOFTWARE_DIR = SCRIPT_DIR.parent
EXCEL_PATH = SOFTWARE_DIR / "data_base" / "BD.xlsx"
OUTPUT_DIR = SOFTWARE_DIR / "generated"

LINE_WIDTH = 140
BLOCK_BITS = 16
TAR_NONE = 0


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def warn(message: str) -> None:
    print(f"warning: {message}")


def error_log(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)


def info(message: str) -> None:
    print(f"info: {message}")


def normalize_header(value: Any) -> str:
    if value is None:
        return ""
    text = str(value).strip().lower().replace("*", "").replace(" ", "_")
    return text


def sanitize_identifier(value: Any) -> str:
    text = str(value or "").strip().upper()
    text = text.replace("Ё", "Е")
    text = re.sub(r"[^A-Z0-9_]", "_", text)
    text = re.sub(r"_+", "_", text).strip("_")
    if not text:
        text = "UNNAMED"
    if text[0].isdigit():
        text = f"_{text}"
    return text


def c_comment(value: Any) -> str:
    text = "" if value is None else str(value)
    return text.replace("/*", "/ *").replace("*/", "* /")


def to_int(value: Any) -> int | None:
    if value in (None, ""):
        return None
    if isinstance(value, bool):
        return int(value)
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        if math.isnan(value):
            return None
        return int(value)
    text = str(value).strip().replace(",", ".")
    if not text:
        return None
    try:
        return int(float(text))
    except ValueError:
        return None


def to_cpp_number(value: Any) -> str:
    if value in (None, ""):
        return "0"
    if isinstance(value, bool):
        return "1" if value else "0"
    if isinstance(value, int):
        return str(value)
    if isinstance(value, float):
        if value.is_integer():
            return str(int(value))
        return str(value).replace(",", ".")
    text = str(value).strip().replace(",", ".")
    return text if text else "0"


def pad140(line: str) -> str:
    if len(line) >= LINE_WIDTH:
        return line
    return line + (" " * (LINE_WIDTH - len(line)))


def banner(source: str) -> str:
    rows = [
        "/* -------------------------------------------------------------------------- */",
        f"/* AUTO-GENERATED FILE. Source: {source:<49} */",
        "/* Do not edit manually.                                                     */",
        "/* -------------------------------------------------------------------------- */",
        "",
    ]
    return "\n".join(rows)


def build_header_map(headers: list[Any]) -> dict[str, int]:
    return {normalize_header(h): i for i, h in enumerate(headers)}


def get_by_header(row: list[Any], header_map: dict[str, int], *names: str, default: Any = None) -> Any:
    for name in names:
        idx = header_map.get(normalize_header(name))
        if idx is not None and idx < len(row):
            return row[idx]
    return default


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def type_size_bits(type_name: str) -> int:
    normalized = sanitize_identifier(type_name)
    if normalized == "D":
        return 1
    if normalized == "A":
        return 8
    if normalized == "AP":
        return 16
    fail(f"Неизвестный тип параметра: {type_name}")
    return 0


def sign_macro(sign: str) -> str:
    return "TYPE_SIGN" if sanitize_identifier(sign) == "S" else "TYPE_UNSIGN"


def field_line(c_type: str, name: str, comment: str) -> str:
    return pad140(f"\t{c_type:<10}\t{name:<20}\t/** {comment} */")


@dataclass
class BaseRow:
    row_num: int
    id: int
    name: str
    description: str
    system_name: str
    type_name: str
    sign: str
    tar_a: Any
    tar_b: Any
    tar_c: Any
    alg_name: str
    msg_offset: int
    msg_block_offset: int
    msg_block_n: int
    msg_cs_offset: int = 0
    msg_cs_block_n: int = 0
    is_placeholder: bool = False


@dataclass
class ParamRow(BaseRow):
    pass


@dataclass
class CommandRow(BaseRow):
    pass


@dataclass
class SystemStats:
    blocks: int
    bits: int


def load_data_book(path: Path):
    if not path.exists():
        fail(f"Excel-файл не найден: {path}")
    return load_workbook(path, data_only=True)


def parse_sheet(wb, sheet_name: str, kind: str):
    if sheet_name not in wb.sheetnames:
        fail(f"В книге отсутствует лист '{sheet_name}'")

    sheet = wb[sheet_name]
    headers = [sheet.cell(1, c).value for c in range(1, sheet.max_column + 1)]
    hm = build_header_map(headers)

    raw_rows: list[dict[str, Any]] = []
    used_ids: set[int] = set()

    for r in range(2, sheet.max_row + 1):
        data = [sheet.cell(r, c).value for c in range(1, sheet.max_column + 1)]

        row_id = to_int(get_by_header(data, hm, "ID"))
        name = str(get_by_header(data, hm, "Name", default="") or "").strip()

        msg_offset = to_int(get_by_header(data, hm, "MSG_Offset"))
        msg_block_offset = to_int(get_by_header(data, hm, "MSG_Block_Offset"))
        msg_block_n = to_int(get_by_header(data, hm, "MSG_Block_N"))
        if msg_offset is None or msg_block_offset is None or msg_block_n is None:
            fail(f"{kind}: пустые поля MSG_* в строке {r}")

        tar_a = get_by_header(data, hm, "tar_A")
        tar_b = get_by_header(data, hm, "tar_B")
        tar_c = get_by_header(data, hm, "tar_C")

        if tar_a in (None, ""):
            warn(f"{kind}: строка {r}, отсутствует tar_A -> подставлен TAR_NONE=0")
            tar_a = TAR_NONE
        if tar_b in (None, ""):
            warn(f"{kind}: строка {r}, отсутствует tar_B -> подставлен TAR_NONE=0")
            tar_b = TAR_NONE
        if tar_c in (None, ""):
            warn(f"{kind}: строка {r}, отсутствует tar_C -> подставлен TAR_NONE=0")
            tar_c = TAR_NONE

        if row_id is not None:
            if row_id in used_ids:
                fail(f"{kind}: обнаружен повторяющийся ID={row_id} (строка {r})")
            used_ids.add(row_id)

        raw_rows.append(
            {
                "row_num": r,
                "id": row_id,
                "name": name,
                "description": str(get_by_header(data, hm, "Description", default="") or ""),
                "system_name": str(get_by_header(data, hm, "System", default="NONE") or "NONE").strip(),
                "type_name": str(get_by_header(data, hm, "Type", default="NONE") or "NONE").strip().upper(),
                "sign": str(get_by_header(data, hm, "Syg", "Sign", default="U") or "U").strip().upper(),
                "tar_a": tar_a,
                "tar_b": tar_b,
                "tar_c": tar_c,
                "alg_name": str(get_by_header(data, hm, "Alg", "ALG", default="NONE") or "NONE").strip(),
                "msg_offset": msg_offset,
                "msg_block_offset": msg_block_offset,
                "msg_block_n": msg_block_n,
            }
        )

    if not raw_rows:
        fail(f"Лист '{sheet_name}' пуст")

    next_id = min(used_ids) if used_ids else 0

    def allocate_id() -> int:
        nonlocal next_id
        while next_id in used_ids:
            next_id += 1
        value = next_id
        used_ids.add(value)
        next_id += 1
        return value

    rows: list[BaseRow] = []
    for item in raw_rows:
        row_id = item["id"]
        if row_id is None:
            row_id = allocate_id()
            error_log(
                f"{kind}: строка {item['row_num']}, отсутствует ID -> присвоен автоматически ID={row_id}"
            )

        name = item["name"]
        if not name:
            name = f"NONE_{row_id}"
            warn(f"{kind}: строка {item['row_num']}, отсутствует Name -> использован {name}")

        rows.append(
            BaseRow(
                row_num=item["row_num"],
                id=row_id,
                name=name,
                description=item["description"],
                system_name=item["system_name"],
                type_name=item["type_name"],
                sign=item["sign"],
                tar_a=item["tar_a"],
                tar_b=item["tar_b"],
                tar_c=item["tar_c"],
                alg_name=item["alg_name"],
                msg_offset=item["msg_offset"],
                msg_block_offset=item["msg_block_offset"],
                msg_block_n=item["msg_block_n"],
            )
        )

    return rows


def fill_id_gaps(rows: list[BaseRow], kind: str):
    ids = [r.id for r in rows]
    if len(ids) != len(set(ids)):
        fail(f"{kind}: обнаружены повторяющиеся ID")

    max_id = max(ids)
    by_id = {r.id: r for r in rows}
    result: list[BaseRow] = []

    for idx in range(0, max_id + 1):
        if idx in by_id:
            result.append(by_id[idx])
            continue

        warn(f"{kind}: отсутствует ID={idx}, вставлен резервный NONE")
        result.append(
            BaseRow(
                row_num=0,
                id=idx,
                name=f"NONE_{idx}",
                description="RESERVED",
                system_name="NONE",
                type_name="D",
                sign="U",
                tar_a=TAR_NONE,
                tar_b=TAR_NONE,
                tar_c=TAR_NONE,
                alg_name="NONE",
                msg_offset=0,
                msg_block_offset=0,
                msg_block_n=0,
                is_placeholder=True,
            )
        )

    return result


def cast_params(rows: list[BaseRow]) -> list[ParamRow]:
    return [ParamRow(**r.__dict__) for r in rows]


def cast_commands(rows: list[BaseRow]) -> list[CommandRow]:
    return [CommandRow(**r.__dict__) for r in rows]


def ordered_unique(values: list[str], head: str = "NONE") -> list[str]:
    seen: set[str] = set()
    out: list[str] = []

    def add(v: str) -> None:
        k = sanitize_identifier(v)
        if k in seen:
            return
        seen.add(k)
        out.append(v)

    add(head)
    for value in values:
        if sanitize_identifier(value) != sanitize_identifier(head):
            add(value)
    return out


def collect_systems(params: list[ParamRow], commands: list[CommandRow]) -> list[str]:
    return ordered_unique([*(x.system_name for x in params), *(x.system_name for x in commands)], "NONE")


def collect_algs(params: list[ParamRow], commands: list[CommandRow]) -> list[str]:
    return ordered_unique([*(x.alg_name for x in params), *(x.alg_name for x in commands)], "NONE")


def collect_types(params: list[ParamRow], commands: list[CommandRow]) -> list[str]:
    base = ["NONE", "D", "A", "AP", "SIGN", "UNSIGN"]
    used = {sanitize_identifier(x) for x in base}
    values = {sanitize_identifier(x.type_name): x.type_name for x in [*params, *commands]}
    out = list(base)
    for _, value in sorted(values.items(), key=lambda kv: kv[0]):
        key = sanitize_identifier(value)
        if key not in used:
            out.append(value)
            used.add(key)
    return out


def system_local_bits(rows: list[BaseRow], system_key: str) -> tuple[int, list[tuple[int, int, BaseRow]]]:
    local_rows = [r for r in rows if sanitize_identifier(r.system_name) == system_key and not r.is_placeholder]
    if not local_rows:
        return 0, []

    min_block = min(r.msg_block_n for r in local_rows)
    intervals: list[tuple[int, int, BaseRow]] = []
    max_end = 0

    for row in local_rows:
        size = type_size_bits(row.type_name)
        start = (row.msg_block_n - min_block) * BLOCK_BITS + row.msg_block_offset
        end = start + size
        max_end = max(max_end, end)
        intervals.append((start, end, row))

    blocks = max(1, math.ceil(max_end / BLOCK_BITS))
    return blocks, intervals


def check_overlaps(rows: list[BaseRow], label: str) -> None:
    systems = sorted({sanitize_identifier(r.system_name) for r in rows})
    for sys_key in systems:
        blocks, intervals = system_local_bits(rows, sys_key)
        if blocks == 0:
            continue
        intervals.sort(key=lambda x: x[0])
        for i in range(1, len(intervals)):
            p_start, p_end, p_row = intervals[i - 1]
            c_start, c_end, c_row = intervals[i]
            if c_start < p_end:
                fail(
                    f"{label}/{sys_key}: пересечение битовых полей: "
                    f"{p_row.name}[{p_start}:{p_end}] и {c_row.name}[{c_start}:{c_end}]"
                )


def system_stats(rows: list[BaseRow], systems: list[str], label: str) -> dict[str, SystemStats]:
    result: dict[str, SystemStats] = {}

    for sys in systems:
        sys_key = sanitize_identifier(sys)
        blocks, intervals = system_local_bits(rows, sys_key)
        bits = blocks * BLOCK_BITS
        result[sys_key] = SystemStats(blocks=blocks, bits=bits)

        if blocks == 0:
            info(f"{label}/{sys_key}: данных нет")
            continue

        occupied = [False] * bits
        for start, end, _ in intervals:
            for b in range(max(0, start), min(bits, end)):
                occupied[b] = True

        free_ranges: list[tuple[int, int]] = []
        idx = 0
        while idx < bits:
            if occupied[idx]:
                idx += 1
                continue
            begin = idx
            while idx < bits and not occupied[idx]:
                idx += 1
            free_ranges.append((begin, idx))

        if free_ranges:
            pretty = ", ".join([f"[{a}:{b})" for a, b in free_ranges])
            info(f"{label}/{sys_key}: незанятые биты: {pretty}")
        else:
            info(f"{label}/{sys_key}: незанятых бит нет")

    return result


def compute_cs_maps(systems: list[str], stats: dict[str, SystemStats]) -> tuple[dict[str, int], int]:
    base_bits: dict[str, int] = {}
    current = 0
    for sys in systems:
        key = sanitize_identifier(sys)
        if key in ("NONE", "CS"):
            continue
        base_bits[key] = current
        current += stats.get(key, SystemStats(0, 0)).bits
    return base_bits, current


def assign_cs_offsets(rows: list[BaseRow], cs_base_bits: dict[str, int]) -> None:
    # local base block index for each system
    min_block_by_system: dict[str, int] = {}
    for row in rows:
        if row.is_placeholder:
            continue
        key = sanitize_identifier(row.system_name)
        if key not in min_block_by_system:
            min_block_by_system[key] = row.msg_block_n
        else:
            min_block_by_system[key] = min(min_block_by_system[key], row.msg_block_n)

    for row in rows:
        key = sanitize_identifier(row.system_name)
        local_min = min_block_by_system.get(key, 0)
        local_bit = (row.msg_block_n - local_min) * BLOCK_BITS + row.msg_block_offset
        base = cs_base_bits.get(key, 0)
        row.msg_cs_offset = base + local_bit
        row.msg_cs_block_n = row.msg_cs_offset // BLOCK_BITS


def make_define(name: str, value: Any, comment: str) -> str:
    line = f"#define\t{name:<36}\t{value:<8}\t/** {c_comment(comment)} */"
    return pad140(line)


def make_define_file(items: list[tuple[str, Any, str]], max_name: str, max_value: int) -> str:
    lines: list[str] = [banner("BD.xlsx"), pad140("#pragma once"), ""]
    for name, value, comment in items:
        lines.append(make_define(name, value, comment))
    lines.append("")
    lines.append(pad140(f"#define\t{max_name:<36}\t{max_value}"))
    lines.append("")
    return "\n".join(lines)


def make_sys_list(
    systems: list[str],
    pstats: dict[str, SystemStats],
    cstats: dict[str, SystemStats],
    cs_param_bits: int,
    cs_comm_bits: int,
) -> str:
    items: list[tuple[str, Any, str]] = [("CS", 1, "включение CS")]

    for idx, name in enumerate(systems):
        comment = "id системы" if idx == 0 and sanitize_identifier(name) == "NONE" else name
        items.append((f"SYS_{sanitize_identifier(name)}", idx, comment))

    for name in systems:
        key = sanitize_identifier(name)
        p = pstats.get(key, SystemStats(0, 0))
        c = cstats.get(key, SystemStats(0, 0))

        if key == "CS":
            p = SystemStats(cs_param_bits // BLOCK_BITS, cs_param_bits)
            c = SystemStats(cs_comm_bits // BLOCK_BITS, cs_comm_bits)

        items.extend(
            [
                (f"{key}_PARAM_MSG_BLOCKS", p.blocks, f"{name}: max блоков параметров"),
                (f"{key}_PARAM_MSG_BITS", p.bits, f"{name}: max длина параметров, бит"),
                (f"{key}_COMM_MSG_BLOCKS", c.blocks, f"{name}: max блоков команд"),
                (f"{key}_COMM_MSG_BITS", c.bits, f"{name}: max длина команд, бит"),
            ]
        )

    return make_define_file(items, "SYS_max", len(systems))


def make_alg_list(algs: list[str]) -> str:
    items = []
    for idx, name in enumerate(algs):
        comment = "алгоритм обработки" if idx == 0 and sanitize_identifier(name) == "NONE" else name
        items.append((f"ALG_{sanitize_identifier(name)}", idx, comment))
    return make_define_file(items, "ALG_max", len(algs))


def make_type_list(types: list[str]) -> str:
    items = []
    for idx, name in enumerate(types):
        comment = "тип данных" if idx == 0 and sanitize_identifier(name) == "NONE" else name
        items.append((f"TYPE_{sanitize_identifier(name)}", idx, comment))
    return make_define_file(items, "TYPE_max", len(types))


def make_param_list(params: list[ParamRow]) -> str:
    items = [(sanitize_identifier(r.name), r.id, r.description) for r in params]
    return make_define_file(items, "Param_max", max((r.id for r in params), default=0) + 1)


def make_command_list(commands: list[CommandRow]) -> str:
    items = [(sanitize_identifier(r.name), r.id, r.description) for r in commands]
    return make_define_file(items, "Comm_max", max((r.id for r in commands), default=0) + 1)


def table_comment(name: str, desc: str) -> str:
    return f'/** "{c_comment(name)}"\t"{c_comment(desc)}" */'


def align_rows(init_rows: list[str], comments: list[str], commas: list[bool]) -> list[str]:
    rows_with_commas = [f"{r}{',' if c else ''}" for r, c in zip(init_rows, commas)]
    max_len = max((len(r) for r in rows_with_commas), default=0)
    out = []
    for row, comment in zip(rows_with_commas, comments):
        out.append(pad140(f"{row:<{max_len}}\t{comment}"))
    return out


def make_param_tab(params: list[ParamRow]) -> str:
    lines = [
        banner("BD.xlsx"),
        pad140("#pragma once"),
        pad140("#include <stdint.h>"),
        "",
        pad140('#include "sys_list.hpp"'),
        pad140('#include "type_list.hpp"'),
        pad140('#include "alg_list.hpp"'),
        pad140('#include "param_list.hpp"'),
        "",
        pad140("#define\tTAR_NONE\t0"),
        "",
        pad140("struct S_paramtab{"),
        field_line("char", "sys_id;", "id системы"),
        field_line("char", "type;", "тип данных"),
        field_line("char", "sign;", "TYPE_SIGN / TYPE_UNSIGN"),
        field_line("char", "tar_a;", "тарировка A"),
        field_line("char", "tar_b;", "тарировка B"),
        field_line("char", "tar_c;", "тарировка C"),
        field_line("uint16_t", "msg_offset;", "глобальное смещение в сообщении"),
        field_line("char", "msg_block_offset;", "смещение в блоке данных"),
        field_line("char", "msg_block_n;", "номер блока данных"),
        field_line("uint16_t", "msg_cs_offset;", "глобальное смещение внутри CS"),
        field_line("char", "msg_cs_block_n;", "номер блока внутри CS"),
        field_line("char", "alg;", "алгоритм обработки"),
        pad140("};"),
        "",
        pad140("struct S_paramtab S_baseparamtab[Param_max]={"),
    ]

    init_rows: list[str] = []
    comments: list[str] = []
    commas: list[bool] = []
    for idx, r in enumerate(params):
        init_rows.append(
            (
                f"\t{{SYS_{sanitize_identifier(r.system_name)}, TYPE_{sanitize_identifier(r.type_name)}, {sign_macro(r.sign)}, "
                f"{to_cpp_number(r.tar_a)}, {to_cpp_number(r.tar_b)}, {to_cpp_number(r.tar_c)}, {r.msg_offset}, "
                f"{r.msg_block_offset}, {r.msg_block_n}, {r.msg_cs_offset}, {r.msg_cs_block_n}, ALG_{sanitize_identifier(r.alg_name)}}}"
            )
        )
        comments.append(table_comment(r.name, r.description))
        commas.append(idx != len(params) - 1)

    lines.extend(align_rows(init_rows, comments, commas))
    lines.append(pad140("};"))
    lines.append("")
    return "\n".join(lines)


def make_command_tab(commands: list[CommandRow]) -> str:
    lines = [
        banner("BD.xlsx"),
        pad140("#pragma once"),
        pad140("#include <stdint.h>"),
        "",
        pad140('#include "sys_list.hpp"'),
        pad140('#include "type_list.hpp"'),
        pad140('#include "alg_list.hpp"'),
        pad140('#include "command_list.hpp"'),
        "",
        pad140("#define\tTAR_NONE\t0"),
        "",
        pad140("struct S_commtab{"),
        field_line("char", "sys_id;", "id системы"),
        field_line("char", "type;", "тип данных"),
        field_line("char", "sign;", "TYPE_SIGN / TYPE_UNSIGN"),
        field_line("char", "tar_a;", "тарировка A"),
        field_line("char", "tar_b;", "тарировка B"),
        field_line("char", "tar_c;", "тарировка C"),
        field_line("uint16_t", "msg_offset;", "глобальное смещение в сообщении"),
        field_line("char", "msg_block_offset;", "смещение в блоке данных"),
        field_line("char", "msg_block_n;", "номер блока данных"),
        field_line("uint16_t", "msg_cs_offset;", "глобальное смещение внутри CS"),
        field_line("char", "msg_cs_block_n;", "номер блока внутри CS"),
        field_line("char", "alg;", "алгоритм обработки"),
        pad140("};"),
        "",
        pad140("struct S_commtab S_basecommtab[Comm_max]={"),
    ]

    init_rows: list[str] = []
    comments: list[str] = []
    commas: list[bool] = []
    for idx, r in enumerate(commands):
        init_rows.append(
            (
                f"\t{{SYS_{sanitize_identifier(r.system_name)}, TYPE_{sanitize_identifier(r.type_name)}, {sign_macro(r.sign)}, "
                f"{to_cpp_number(r.tar_a)}, {to_cpp_number(r.tar_b)}, {to_cpp_number(r.tar_c)}, {r.msg_offset}, "
                f"{r.msg_block_offset}, {r.msg_block_n}, {r.msg_cs_offset}, {r.msg_cs_block_n}, ALG_{sanitize_identifier(r.alg_name)}}}"
            )
        )
        comments.append(table_comment(r.name, r.description))
        commas.append(idx != len(commands) - 1)

    lines.extend(align_rows(init_rows, comments, commas))
    lines.append(pad140("};"))
    lines.append("")
    return "\n".join(lines)


def write_file(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8", newline="\n")


def main() -> None:
    wb = load_data_book(EXCEL_PATH)

    params_raw = parse_sheet(wb, "Params", "Params")
    commands_raw = parse_sheet(wb, "Commands", "Commands")

    params = cast_params(fill_id_gaps(params_raw, "Params"))
    commands = cast_commands(fill_id_gaps(commands_raw, "Commands"))

    check_overlaps(params, "Params")
    check_overlaps(commands, "Commands")

    systems = collect_systems(params, commands)
    algs = collect_algs(params, commands)
    types = collect_types(params, commands)

    pstats = system_stats(params, systems, "Params")
    cstats = system_stats(commands, systems, "Commands")

    cs_param_base, cs_param_bits = compute_cs_maps(systems, pstats)
    cs_comm_base, cs_comm_bits = compute_cs_maps(systems, cstats)

    info(f"CS/PARAM: суммарно блоков={cs_param_bits // BLOCK_BITS}, бит={cs_param_bits}")
    info(f"CS/COMM: суммарно блоков={cs_comm_bits // BLOCK_BITS}, бит={cs_comm_bits}")

    assign_cs_offsets(params, cs_param_base)
    assign_cs_offsets(commands, cs_comm_base)

    ensure_dir(OUTPUT_DIR)

    files = {
        "sys_list.hpp": make_sys_list(systems, pstats, cstats, cs_param_bits, cs_comm_bits),
        "alg_list.hpp": make_alg_list(algs),
        "type_list.hpp": make_type_list(types),
        "param_list.hpp": make_param_list(params),
        "param_tab.hpp": make_param_tab(params),
        "command_list.hpp": make_command_list(commands),
        "command_tab.hpp": make_command_tab(commands),
    }

    for fname, content in files.items():
        write_file(OUTPUT_DIR / fname, content)

    print(f"Generated {len(files)} files in: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
