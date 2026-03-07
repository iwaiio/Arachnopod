#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence
import math
import re
import sys

from openpyxl import load_workbook


SCRIPT_DIR = Path(__file__).resolve().parent
SOFTWARE_DIR = SCRIPT_DIR.parent
EXCEL_PATH = SOFTWARE_DIR / "data_base" / "bd.xlsx"
OUTPUT_DIR = SOFTWARE_DIR / "generated"


# -----------------------------------------------------------------------------
# helpers
# -----------------------------------------------------------------------------

def die(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def normalize_header(value: Any) -> str:
    if value is None:
        return ""
    s = str(value).strip()
    s = s.replace("*", "")
    s = s.replace(" ", "_")
    return s.lower()


def make_header_map(headers: Sequence[Any]) -> Dict[str, int]:
    return {normalize_header(h): idx for idx, h in enumerate(headers)}


def get_cell(row: Sequence[Any], header_map: Dict[str, int], *names: str, default=None):
    for name in names:
        idx = header_map.get(normalize_header(name))
        if idx is not None and idx < len(row):
            return row[idx]
    return default


def as_int(value: Any) -> Optional[int]:
    if value is None or value == "":
        return None
    if isinstance(value, bool):
        return int(value)
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        if math.isnan(value):
            return None
        return int(value)
    s = str(value).strip()
    if not s:
        return None
    try:
        return int(float(s))
    except ValueError:
        return None


def as_cpp_number(value: Any) -> str:
    if value is None or value == "":
        return "0"
    if isinstance(value, bool):
        return "1" if value else "0"
    if isinstance(value, int):
        return str(value)
    if isinstance(value, float):
        if value.is_integer():
            return str(int(value))
        return str(value).replace(",", ".")
    s = str(value).strip().replace(",", ".")
    return s if s else "0"


def sanitize_identifier(name: Any) -> str:
    s = str(name).strip().upper()
    s = s.replace("Ё", "Е")
    s = re.sub(r"[^A-Z0-9_]", "_", s)
    s = re.sub(r"_+", "_", s).strip("_")
    if not s:
        s = "UNNAMED"
    if s[0].isdigit():
        s = "_" + s
    return s


def c_comment_text(value: Any) -> str:
    s = "" if value is None else str(value)
    return s.replace("/*", "/ *").replace("*/", "* /")


def file_banner(source_name: str) -> str:
    return (
        "/* -------------------------------------------------------------------------- */\n"
        f"/* AUTO-GENERATED FILE. Source: {source_name:<49} */\n"
        "/* Do not edit manually.                                                     */\n"
        "/* -------------------------------------------------------------------------- */\n\n"
    )


def validate_dense_ids(items: Sequence["BaseRow"], kind: str) -> None:
    ids = [item.id for item in items]
    if len(ids) != len(set(ids)):
        die(f"{kind}: обнаружены повторяющиеся ID")
    expected = list(range(1, len(items) + 1))
    actual = sorted(ids)
    if actual != expected:
        die(
            f"{kind}: ID должны быть непрерывными в диапазоне 1..{len(items)}. "
            f"Получено: {actual}"
        )


def param_type_size(type_name: str) -> int:
    t = sanitize_identifier(type_name)
    if t == "D":
        return 1
    if t == "A":
        return 8
    if t == "AP":
        return 16
    die(f"Неизвестный тип параметра для расчета размера: {type_name}")
    return 0


def align_up(value: int, alignment: int) -> int:
    if alignment <= 0:
        return value
    return ((value + alignment - 1) // alignment) * alignment


def sign_macro(sign: str) -> str:
    return "TYPE_SIGN" if sanitize_identifier(sign) == "S" else "TYPE_UNSIGN"


def format_struct_field(c_type: str, name: str, comment: str) -> str:
    return f"\t{c_type:<10}\t{name:<16}\t/** {comment} */"


def format_table_comment(name: str, description: str) -> str:
    return f'/** "{c_comment_text(name)}"\t"{c_comment_text(description)}" */'


# -----------------------------------------------------------------------------
# data models
# -----------------------------------------------------------------------------

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


@dataclass
class ParamRow(BaseRow):
    msg_offset: int
    msg_block_offset: int
    msg_block_n: int
    cs_data_offset: int = 0
    cs_data_offset16: int = 0


@dataclass
class CommandRow(BaseRow):
    msg_offset: int
    msg_block_offset: int
    msg_block_n: int


# -----------------------------------------------------------------------------
# parsing
# -----------------------------------------------------------------------------

def read_workbook(path: Path):
    if not path.exists():
        die(f"Excel-файл не найден: {path}")
    return load_workbook(path, data_only=True)


def parse_params_sheet(wb) -> List[ParamRow]:
    if "Params" not in wb.sheetnames:
        die("В книге отсутствует лист 'Params'")

    ws = wb["Params"]
    headers = [ws.cell(1, c).value for c in range(1, ws.max_column + 1)]
    hm = make_header_map(headers)

    items: List[ParamRow] = []
    for r in range(2, ws.max_row + 1):
        row = [ws.cell(r, c).value for c in range(1, ws.max_column + 1)]
        row_id = as_int(get_cell(row, hm, "ID"))
        if row_id is None:
            continue

        name = str(get_cell(row, hm, "Name", default="") or "").strip()
        if not name:
            die(f"Params: пустое Name в строке {r}")

        msg_offset = as_int(get_cell(row, hm, "MSG_Offset"))
        msg_block_offset = as_int(get_cell(row, hm, "MSG_Block_Offset"))
        msg_block_n = as_int(get_cell(row, hm, "MSG_Block_N"))
        if msg_offset is None or msg_block_offset is None or msg_block_n is None:
            die(f"Params: пустые поля MSG_* в строке {r}")

        items.append(
            ParamRow(
                row_num=r,
                id=row_id,
                name=name,
                description=str(get_cell(row, hm, "Description", default="") or ""),
                system_name=str(get_cell(row, hm, "System", default="NONE") or "NONE").strip(),
                type_name=str(get_cell(row, hm, "Type", default="NONE") or "NONE").strip().upper(),
                sign=str(get_cell(row, hm, "Syg", "Sign", default="U") or "U").strip().upper(),
                tar_a=get_cell(row, hm, "tar_A"),
                tar_b=get_cell(row, hm, "tar_B"),
                tar_c=get_cell(row, hm, "tar_C"),
                alg_name=str(get_cell(row, hm, "Alg", "ALG", default="NONE") or "NONE").strip(),
                msg_offset=msg_offset,
                msg_block_offset=msg_block_offset,
                msg_block_n=msg_block_n,
            )
        )

    if not items:
        die("Лист 'Params' пуст")

    validate_dense_ids(items, "Params")
    return sorted(items, key=lambda x: x.id)


def parse_commands_sheet(wb) -> List[CommandRow]:
    if "Commands" not in wb.sheetnames:
        die("В книге отсутствует лист 'Commands'")

    ws = wb["Commands"]
    headers = [ws.cell(1, c).value for c in range(1, ws.max_column + 1)]
    hm = make_header_map(headers)

    items: List[CommandRow] = []
    for r in range(2, ws.max_row + 1):
        row = [ws.cell(r, c).value for c in range(1, ws.max_column + 1)]
        row_id = as_int(get_cell(row, hm, "ID"))
        if row_id is None:
            continue

        name = str(get_cell(row, hm, "Name", default="") or "").strip()
        if not name:
            die(f"Commands: пустое Name в строке {r}")

        msg_offset = as_int(get_cell(row, hm, "MSG_Offset"))
        msg_block_offset = as_int(get_cell(row, hm, "MSG_Block_Offset"))
        msg_block_n = as_int(get_cell(row, hm, "MSG_Block_N"))
        if msg_offset is None or msg_block_offset is None or msg_block_n is None:
            die(f"Commands: пустые поля MSG_* в строке {r}")

        items.append(
            CommandRow(
                row_num=r,
                id=row_id,
                name=name,
                description=str(get_cell(row, hm, "Description", default="") or ""),
                system_name=str(get_cell(row, hm, "System", default="NONE") or "NONE").strip(),
                type_name=str(get_cell(row, hm, "Type", default="NONE") or "NONE").strip().upper(),
                sign=str(get_cell(row, hm, "Syg", "Sign", default="U") or "U").strip().upper(),
                tar_a=get_cell(row, hm, "tar_A"),
                tar_b=get_cell(row, hm, "tar_B"),
                tar_c=get_cell(row, hm, "tar_C"),
                alg_name=str(get_cell(row, hm, "Alg", "ALG", default="NONE") or "NONE").strip(),
                msg_offset=msg_offset,
                msg_block_offset=msg_block_offset,
                msg_block_n=msg_block_n,
            )
        )

    if not items:
        die("Лист 'Commands' пуст")

    validate_dense_ids(items, "Commands")
    return sorted(items, key=lambda x: x.id)


# -----------------------------------------------------------------------------
# dictionaries
# -----------------------------------------------------------------------------

def ordered_unique(values: Sequence[str], head: str = "NONE") -> List[str]:
    seen = set()
    result: List[str] = []

    def add(v: str) -> None:
        sv = sanitize_identifier(v)
        if sv not in seen:
            seen.add(sv)
            result.append(v)

    add(head)
    for value in values:
        if value and sanitize_identifier(value) != sanitize_identifier(head):
            add(value)
    return result


def collect_systems(params: Sequence[ParamRow], commands: Sequence[CommandRow]) -> List[str]:
    values = [x.system_name for x in params] + [x.system_name for x in commands]
    return ordered_unique(values, "NONE")


def collect_algs(params: Sequence[ParamRow], commands: Sequence[CommandRow]) -> List[str]:
    values = [x.alg_name for x in params] + [x.alg_name for x in commands]
    return ordered_unique(values, "NONE")


def collect_types(params: Sequence[ParamRow], commands: Sequence[CommandRow]) -> List[str]:
    values = {sanitize_identifier(x.type_name): x.type_name for x in list(params) + list(commands)}
    result: List[str] = ["NONE", "D", "A", "AP", "SIGN", "UNSIGN"]
    used = {sanitize_identifier(x) for x in result}

    for _, original in sorted(values.items(), key=lambda x: sanitize_identifier(x[1])):
        s = sanitize_identifier(original)
        if s not in used:
            result.append(original)
            used.add(s)

    return result


def compute_param_cs_offsets(params: Sequence[ParamRow]) -> None:
    order: List[str] = []
    system_rows: Dict[str, List[ParamRow]] = {}

    for item in params:
        sys_name = sanitize_identifier(item.system_name)
        if sys_name not in system_rows:
            order.append(sys_name)
            system_rows[sys_name] = []
        system_rows[sys_name].append(item)

    current_base = 0
    base_map: Dict[str, int] = {}

    for sys_name in order:
        rows = system_rows[sys_name]
        base_map[sys_name] = current_base
        system_size = 0
        for row in rows:
            end_offset = row.msg_offset + param_type_size(row.type_name)
            if end_offset > system_size:
                system_size = end_offset
        current_base += align_up(system_size, 16)

    for item in params:
        base = base_map[sanitize_identifier(item.system_name)]
        item.cs_data_offset = base + item.msg_offset
        item.cs_data_offset16 = item.cs_data_offset // 16


# -----------------------------------------------------------------------------
# code generation
# -----------------------------------------------------------------------------

def format_define(name: str, value: int, comment: str) -> str:
    return f"#define {name:<24} {value:<6} /** {c_comment_text(comment)} */"


def generate_define_file(
    macro_items: Sequence[tuple[str, int, str]],
    max_macro_name: str,
    max_value: int,
) -> str:
    lines: List[str] = []
    lines.append(file_banner("BD.xlsx"))
    lines.append("#pragma once\n")
    for name, value, comment in macro_items:
        lines.append(format_define(name, value, comment))
    lines.append("")
    lines.append(f"#define {max_macro_name:<24} {max_value}")
    lines.append("")
    return "\n".join(lines)


def generate_sys_list_hpp(systems: Sequence[str]) -> str:
    items = []
    for idx, name in enumerate(systems):
        macro = f"SYS_{sanitize_identifier(name)}"
        comment = "id системы" if idx == 0 and sanitize_identifier(name) == "NONE" else str(name)
        items.append((macro, idx, comment))
    return generate_define_file(items, "SYS_max", len(systems))


def generate_alg_list_hpp(algs: Sequence[str]) -> str:
    items = []
    for idx, name in enumerate(algs):
        macro = f"ALG_{sanitize_identifier(name)}"
        comment = "алгоритм обработки" if idx == 0 and sanitize_identifier(name) == "NONE" else str(name)
        items.append((macro, idx, comment))
    return generate_define_file(items, "ALG_max", len(algs))


def generate_type_list_hpp(types: Sequence[str]) -> str:
    items = []
    for idx, name in enumerate(types):
        macro = f"TYPE_{sanitize_identifier(name)}"
        comment = "тип данных" if idx == 0 and sanitize_identifier(name) == "NONE" else str(name)
        items.append((macro, idx, comment))
    return generate_define_file(items, "TYPE_max", len(types))


def generate_param_list_hpp(params: Sequence[ParamRow]) -> str:
    items = []
    for item in params:
        items.append((sanitize_identifier(item.name), item.id, item.description))
    return generate_define_file(items, "Param_max", len(params) + 1)


def generate_command_list_hpp(commands: Sequence[CommandRow]) -> str:
    items = []
    for item in commands:
        items.append((sanitize_identifier(item.name), item.id, item.description))
    return generate_define_file(items, "Comm_max", len(commands) + 1)


def generate_param_tab_hpp(params: Sequence[ParamRow]) -> str:
    lines: List[str] = []
    lines.append(file_banner("BD.xlsx"))
    lines.append("#pragma once")
    lines.append("#include <stdint.h>\n")
    lines.append('#include "sys_list.hpp"')
    lines.append('#include "type_list.hpp"')
    lines.append('#include "alg_list.hpp"')
    lines.append('#include "param_list.hpp"\n')

    lines.append("struct S_paramtab{")
    lines.append(format_struct_field("char", "sys_id;", "id системы"))
    lines.append(format_struct_field("char", "type;", "тип данных"))
    lines.append(format_struct_field("char", "sign;", "TYPE_SIGN / TYPE_UNSIGN"))
    lines.append(format_struct_field("float", "tar_a;", "тарировка A"))
    lines.append(format_struct_field("float", "tar_b;", "тарировка B"))
    lines.append(format_struct_field("float", "tar_c;", "тарировка C"))
    lines.append(format_struct_field("uint16_t", "msg_offset;", "смещение в сообщении"))
    lines.append(format_struct_field("char", "msg_block_offset;", "смещение в блоке данных"))
    lines.append(format_struct_field("char", "msg_block_n;", "номер блока данных"))
    lines.append(format_struct_field("uint16_t", "cs_data_offset;", "смещение в CS data"))
    lines.append(format_struct_field("uint16_t", "cs_data_offset16;", "смещение 16-битного CS data"))
    lines.append(format_struct_field("char", "alg;", "алгоритм обработки"))
    lines.append("};\n")

    lines.append("struct S_paramtab S_baseparamtab[Param_max]={")
    for i, item in enumerate(params):
        init = (
            f"\t{{SYS_{sanitize_identifier(item.system_name)}, "
            f"TYPE_{sanitize_identifier(item.type_name)}, "
            f"{sign_macro(item.sign)}, "
            f"{as_cpp_number(item.tar_a)}, "
            f"{as_cpp_number(item.tar_b)}, "
            f"{as_cpp_number(item.tar_c)}, "
            f"{item.msg_offset}, "
            f"{item.msg_block_offset}, "
            f"{item.msg_block_n}, "
            f"{item.cs_data_offset}, "
            f"{item.cs_data_offset16}, "
            f"ALG_{sanitize_identifier(item.alg_name)}}}"
        )
        comma = "," if i != len(params) - 1 else ""
        lines.append(f"{init}{comma}\t{format_table_comment(item.name, item.description)}")
    lines.append("};\n")
    return "\n".join(lines)


def generate_command_tab_hpp(commands: Sequence[CommandRow]) -> str:
    lines: List[str] = []
    lines.append(file_banner("BD.xlsx"))
    lines.append("#pragma once")
    lines.append("#include <stdint.h>\n")
    lines.append('#include "sys_list.hpp"')
    lines.append('#include "type_list.hpp"')
    lines.append('#include "alg_list.hpp"')
    lines.append('#include "command_list.hpp"\n')

    lines.append("struct S_commtab{")
    lines.append(format_struct_field("char", "sys_id;", "id системы"))
    lines.append(format_struct_field("char", "type;", "тип данных"))
    lines.append(format_struct_field("char", "sign;", "TYPE_SIGN / TYPE_UNSIGN"))
    lines.append(format_struct_field("float", "tar_a;", "тарировка A"))
    lines.append(format_struct_field("float", "tar_b;", "тарировка B"))
    lines.append(format_struct_field("float", "tar_c;", "тарировка C"))
    lines.append(format_struct_field("uint16_t", "msg_offset;", "смещение в сообщении"))
    lines.append(format_struct_field("char", "msg_block_offset;", "смещение в блоке данных"))
    lines.append(format_struct_field("char", "msg_block_n;", "номер блока данных"))
    lines.append(format_struct_field("char", "alg;", "алгоритм обработки"))
    lines.append("};\n")

    lines.append("struct S_commtab S_basecommtab[Comm_max]={")
    for i, item in enumerate(commands):
        init = (
            f"\t{{SYS_{sanitize_identifier(item.system_name)}, "
            f"TYPE_{sanitize_identifier(item.type_name)}, "
            f"{sign_macro(item.sign)}, "
            f"{as_cpp_number(item.tar_a)}, "
            f"{as_cpp_number(item.tar_b)}, "
            f"{as_cpp_number(item.tar_c)}, "
            f"{item.msg_offset}, "
            f"{item.msg_block_offset}, "
            f"{item.msg_block_n}, "
            f"ALG_{sanitize_identifier(item.alg_name)}}}"
        )
        comma = "," if i != len(commands) - 1 else ""
        lines.append(f"{init}{comma}\t{format_table_comment(item.name, item.description)}")
    lines.append("};\n")
    return "\n".join(lines)


# -----------------------------------------------------------------------------
# main
# -----------------------------------------------------------------------------

def write_text_file(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8", newline="\n")


def generate_all() -> None:
    wb = read_workbook(EXCEL_PATH)

    params = parse_params_sheet(wb)
    commands = parse_commands_sheet(wb)

    compute_param_cs_offsets(params)

    systems = collect_systems(params, commands)
    types = collect_types(params, commands)
    algs = collect_algs(params, commands)

    ensure_dir(OUTPUT_DIR)

    files = {
        "sys_list.hpp": generate_sys_list_hpp(systems),
        "alg_list.hpp": generate_alg_list_hpp(algs),
        "type_list.hpp": generate_type_list_hpp(types),
        "param_list.hpp": generate_param_list_hpp(params),
        "param_tab.hpp": generate_param_tab_hpp(params),
        "command_list.hpp": generate_command_list_hpp(commands),
        "command_tab.hpp": generate_command_tab_hpp(commands),
    }

    for filename, content in files.items():
        write_text_file(OUTPUT_DIR / filename, content)

    print(f"Generated {len(files)} files in: {OUTPUT_DIR}")


if __name__ == "__main__":
    generate_all()
