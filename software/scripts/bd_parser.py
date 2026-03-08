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


# -----------------------------------------------------------------------------
# utilities
# -----------------------------------------------------------------------------

def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def ensure_output_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def normalize_header(value: Any) -> str:
    if value is None:
        return ""
    text = str(value).strip().lower()
    text = text.replace("*", "")
    text = text.replace(" ", "_")
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


def comment_escape(value: Any) -> str:
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


def make_banner(source: str) -> str:
    return (
        "/* -------------------------------------------------------------------------- */\n"
        f"/* AUTO-GENERATED FILE. Source: {source:<49} */\n"
        "/* Do not edit manually.                                                     */\n"
        "/* -------------------------------------------------------------------------- */\n\n"
    )


def build_header_map(headers: list[Any]) -> dict[str, int]:
    return {normalize_header(h): i for i, h in enumerate(headers)}


def get_by_header(row: list[Any], header_map: dict[str, int], *names: str, default: Any = None) -> Any:
    for name in names:
        idx = header_map.get(normalize_header(name))
        if idx is not None and idx < len(row):
            return row[idx]
    return default


def type_size_bytes(type_name: str) -> int:
    normalized = sanitize_identifier(type_name)
    if normalized == "D":
        return 1
    if normalized == "A":
        return 8
    if normalized == "AP":
        return 16
    fail(f"Неизвестный тип параметра: {type_name}")
    return 0


def align_up(value: int, alignment: int) -> int:
    if alignment <= 0:
        return value
    return ((value + alignment - 1) // alignment) * alignment


def sign_macro(sign: str) -> str:
    return "TYPE_SIGN" if sanitize_identifier(sign) == "S" else "TYPE_UNSIGN"


def field_line(c_type: str, name: str, comment: str) -> str:
    indent = " " * 4
    return f"{indent}{c_type:<10} {name:<18} /** {comment} */"


def align_table_rows(init_rows: list[str], comments: list[str], trailing_comma: list[bool]) -> list[str]:
    merged_rows: list[str] = []
    init_with_commas = [f"{init}{',' if has_comma else ''}" for init, has_comma in zip(init_rows, trailing_comma)]
    max_len = max((len(row) for row in init_with_commas), default=0)

    for row, comment in zip(init_with_commas, comments):
        merged_rows.append(f"{row:<{max_len}}  {comment}")

    return merged_rows


# -----------------------------------------------------------------------------
# models
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
# reading/parsing
# -----------------------------------------------------------------------------

def load_data_book(path: Path):
    if not path.exists():
        fail(f"Excel-файл не найден: {path}")
    return load_workbook(path, data_only=True)


def validate_ids_dense(rows: list[BaseRow], scope: str) -> None:
    ids = [row.id for row in rows]
    if len(ids) != len(set(ids)):
        fail(f"{scope}: обнаружены повторяющиеся ID")

    expected = list(range(1, len(ids) + 1))
    actual = sorted(ids)
    if actual != expected:
        fail(f"{scope}: ID должны быть непрерывными 1..{len(ids)}. Получено: {actual}")


def parse_params(wb) -> list[ParamRow]:
    if "Params" not in wb.sheetnames:
        fail("В книге отсутствует лист 'Params'")

    sheet = wb["Params"]
    headers = [sheet.cell(1, c).value for c in range(1, sheet.max_column + 1)]
    header_map = build_header_map(headers)

    rows: list[ParamRow] = []
    for r in range(2, sheet.max_row + 1):
        data = [sheet.cell(r, c).value for c in range(1, sheet.max_column + 1)]

        row_id = to_int(get_by_header(data, header_map, "ID"))
        if row_id is None:
            continue

        name = str(get_by_header(data, header_map, "Name", default="") or "").strip()
        if not name:
            fail(f"Params: пустое Name в строке {r}")

        msg_offset = to_int(get_by_header(data, header_map, "MSG_Offset"))
        msg_block_offset = to_int(get_by_header(data, header_map, "MSG_Block_Offset"))
        msg_block_n = to_int(get_by_header(data, header_map, "MSG_Block_N"))
        if msg_offset is None or msg_block_offset is None or msg_block_n is None:
            fail(f"Params: пустые поля MSG_* в строке {r}")

        rows.append(
            ParamRow(
                row_num=r,
                id=row_id,
                name=name,
                description=str(get_by_header(data, header_map, "Description", default="") or ""),
                system_name=str(get_by_header(data, header_map, "System", default="NONE") or "NONE").strip(),
                type_name=str(get_by_header(data, header_map, "Type", default="NONE") or "NONE").strip().upper(),
                sign=str(get_by_header(data, header_map, "Syg", "Sign", default="U") or "U").strip().upper(),
                tar_a=get_by_header(data, header_map, "tar_A"),
                tar_b=get_by_header(data, header_map, "tar_B"),
                tar_c=get_by_header(data, header_map, "tar_C"),
                alg_name=str(get_by_header(data, header_map, "Alg", "ALG", default="NONE") or "NONE").strip(),
                msg_offset=msg_offset,
                msg_block_offset=msg_block_offset,
                msg_block_n=msg_block_n,
            )
        )

    if not rows:
        fail("Лист 'Params' пуст")

    validate_ids_dense(rows, "Params")
    return sorted(rows, key=lambda x: x.id)


def parse_commands(wb) -> list[CommandRow]:
    if "Commands" not in wb.sheetnames:
        fail("В книге отсутствует лист 'Commands'")

    sheet = wb["Commands"]
    headers = [sheet.cell(1, c).value for c in range(1, sheet.max_column + 1)]
    header_map = build_header_map(headers)

    rows: list[CommandRow] = []
    for r in range(2, sheet.max_row + 1):
        data = [sheet.cell(r, c).value for c in range(1, sheet.max_column + 1)]

        row_id = to_int(get_by_header(data, header_map, "ID"))
        if row_id is None:
            continue

        name = str(get_by_header(data, header_map, "Name", default="") or "").strip()
        if not name:
            fail(f"Commands: пустое Name в строке {r}")

        msg_offset = to_int(get_by_header(data, header_map, "MSG_Offset"))
        msg_block_offset = to_int(get_by_header(data, header_map, "MSG_Block_Offset"))
        msg_block_n = to_int(get_by_header(data, header_map, "MSG_Block_N"))
        if msg_offset is None or msg_block_offset is None or msg_block_n is None:
            fail(f"Commands: пустые поля MSG_* в строке {r}")

        rows.append(
            CommandRow(
                row_num=r,
                id=row_id,
                name=name,
                description=str(get_by_header(data, header_map, "Description", default="") or ""),
                system_name=str(get_by_header(data, header_map, "System", default="NONE") or "NONE").strip(),
                type_name=str(get_by_header(data, header_map, "Type", default="NONE") or "NONE").strip().upper(),
                sign=str(get_by_header(data, header_map, "Syg", "Sign", default="U") or "U").strip().upper(),
                tar_a=get_by_header(data, header_map, "tar_A"),
                tar_b=get_by_header(data, header_map, "tar_B"),
                tar_c=get_by_header(data, header_map, "tar_C"),
                alg_name=str(get_by_header(data, header_map, "Alg", "ALG", default="NONE") or "NONE").strip(),
                msg_offset=msg_offset,
                msg_block_offset=msg_block_offset,
                msg_block_n=msg_block_n,
            )
        )

    if not rows:
        fail("Лист 'Commands' пуст")

    validate_ids_dense(rows, "Commands")
    return sorted(rows, key=lambda x: x.id)


# -----------------------------------------------------------------------------
# data transforms
# -----------------------------------------------------------------------------

def ordered_unique(values: list[str], head: str = "NONE") -> list[str]:
    seen: set[str] = set()
    result: list[str] = []

    def add(value: str) -> None:
        key = sanitize_identifier(value)
        if key in seen:
            return
        seen.add(key)
        result.append(value)

    add(head)
    for value in values:
        if sanitize_identifier(value) != sanitize_identifier(head):
            add(value)

    return result


def collect_systems(params: list[ParamRow], commands: list[CommandRow]) -> list[str]:
    return ordered_unique([*map(lambda x: x.system_name, params), *map(lambda x: x.system_name, commands)])


def collect_algorithms(params: list[ParamRow], commands: list[CommandRow]) -> list[str]:
    return ordered_unique([*map(lambda x: x.alg_name, params), *map(lambda x: x.alg_name, commands)])


def collect_types(params: list[ParamRow], commands: list[CommandRow]) -> list[str]:
    preset = ["NONE", "D", "A", "AP", "SIGN", "UNSIGN"]
    used = {sanitize_identifier(x) for x in preset}

    source = {sanitize_identifier(r.type_name): r.type_name for r in [*params, *commands]}
    result = list(preset)

    for _, original in sorted(source.items(), key=lambda item: item[0]):
        key = sanitize_identifier(original)
        if key not in used:
            result.append(original)
            used.add(key)

    return result


def compute_cs_offsets(params: list[ParamRow]) -> None:
    groups: dict[str, list[ParamRow]] = {}
    order: list[str] = []

    for row in params:
        key = sanitize_identifier(row.system_name)
        if key not in groups:
            groups[key] = []
            order.append(key)
        groups[key].append(row)

    current = 0
    base_by_system: dict[str, int] = {}

    for key in order:
        rows = groups[key]
        base_by_system[key] = current

        max_end = 0
        for row in rows:
            max_end = max(max_end, row.msg_offset + type_size_bytes(row.type_name))

        current += align_up(max_end, 16)

    for row in params:
        base = base_by_system[sanitize_identifier(row.system_name)]
        row.cs_data_offset = base + row.msg_offset
        row.cs_data_offset16 = row.cs_data_offset // 16


# -----------------------------------------------------------------------------
# generation
# -----------------------------------------------------------------------------

def macro_line(name: str, value: int, comment: str) -> str:
    return f"#define {name:<28} {value:<6} /** {comment_escape(comment)} */"


def make_define_file(items: list[tuple[str, int, str]], max_name: str, max_value: int) -> str:
    lines: list[str] = [make_banner("BD.xlsx"), "#pragma once\n"]
    for name, value, comment in items:
        lines.append(macro_line(name, value, comment))
    lines.append("")
    lines.append(f"#define {max_name:<24} {max_value}")
    lines.append("")
    return "\n".join(lines)


def make_sys_list(systems: list[str]) -> str:
    items: list[tuple[str, int, str]] = []
    for i, name in enumerate(systems):
        comment = "id системы" if i == 0 and sanitize_identifier(name) == "NONE" else name
        items.append((f"SYS_{sanitize_identifier(name)}", i, comment))
    return make_define_file(items, "SYS_max", len(systems))


def make_alg_list(algs: list[str]) -> str:
    items: list[tuple[str, int, str]] = []
    for i, name in enumerate(algs):
        comment = "алгоритм обработки" if i == 0 and sanitize_identifier(name) == "NONE" else name
        items.append((f"ALG_{sanitize_identifier(name)}", i, comment))
    return make_define_file(items, "ALG_max", len(algs))


def make_type_list(types: list[str]) -> str:
    items: list[tuple[str, int, str]] = []
    for i, name in enumerate(types):
        comment = "тип данных" if i == 0 and sanitize_identifier(name) == "NONE" else name
        items.append((f"TYPE_{sanitize_identifier(name)}", i, comment))
    return make_define_file(items, "TYPE_max", len(types))


def make_param_list(params: list[ParamRow]) -> str:
    items = [(sanitize_identifier(row.name), row.id, row.description) for row in params]
    return make_define_file(items, "Param_max", len(params) + 1)


def make_command_list(commands: list[CommandRow]) -> str:
    items = [(sanitize_identifier(row.name), row.id, row.description) for row in commands]
    return make_define_file(items, "Comm_max", len(commands) + 1)


def make_table_comment(name: str, description: str) -> str:
    return f'/** "{comment_escape(name)}"  "{comment_escape(description)}" */'


def make_param_tab(params: list[ParamRow]) -> str:
    lines: list[str] = [
        make_banner("BD.xlsx"),
        "#pragma once",
        "#include <stdint.h>\n",
        '#include "sys_list.hpp"',
        '#include "type_list.hpp"',
        '#include "alg_list.hpp"',
        '#include "param_list.hpp"\n',
        "struct S_paramtab{",
        field_line("char", "sys_id;", "id системы"),
        field_line("char", "type;", "тип данных"),
        field_line("char", "sign;", "TYPE_SIGN / TYPE_UNSIGN"),
        field_line("float", "tar_a;", "тарировка A"),
        field_line("float", "tar_b;", "тарировка B"),
        field_line("float", "tar_c;", "тарировка C"),
        field_line("uint16_t", "msg_offset;", "смещение в сообщении"),
        field_line("char", "msg_block_offset;", "смещение в блоке данных"),
        field_line("char", "msg_block_n;", "номер блока данных"),
        field_line("uint16_t", "cs_data_offset;", "смещение в CS data"),
        field_line("uint16_t", "cs_data_offset16;", "смещение 16-битного CS data"),
        field_line("char", "alg;", "алгоритм обработки"),
        "};\n",
        "struct S_paramtab S_baseparamtab[Param_max]={",
    ]

    init_rows: list[str] = []
    comments: list[str] = []
    trailing_comma: list[bool] = []
    for idx, row in enumerate(params):
        init_rows.append(
            (
                f"    {{SYS_{sanitize_identifier(row.system_name)}, "
                f"TYPE_{sanitize_identifier(row.type_name)}, "
                f"{sign_macro(row.sign)}, "
                f"{to_cpp_number(row.tar_a)}, "
                f"{to_cpp_number(row.tar_b)}, "
                f"{to_cpp_number(row.tar_c)}, "
                f"{row.msg_offset}, "
                f"{row.msg_block_offset}, "
                f"{row.msg_block_n}, "
                f"{row.cs_data_offset}, "
                f"{row.cs_data_offset16}, "
                f"ALG_{sanitize_identifier(row.alg_name)}}}"
            )
        )
        comments.append(make_table_comment(row.name, row.description))
        trailing_comma.append(idx != len(params) - 1)

    lines.extend(align_table_rows(init_rows, comments, trailing_comma))

    lines.append("};\n")
    return "\n".join(lines)


def make_command_tab(commands: list[CommandRow]) -> str:
    lines: list[str] = [
        make_banner("BD.xlsx"),
        "#pragma once",
        "#include <stdint.h>\n",
        '#include "sys_list.hpp"',
        '#include "type_list.hpp"',
        '#include "alg_list.hpp"',
        '#include "command_list.hpp"\n',
        "struct S_commtab{",
        field_line("char", "sys_id;", "id системы"),
        field_line("char", "type;", "тип данных"),
        field_line("char", "sign;", "TYPE_SIGN / TYPE_UNSIGN"),
        field_line("float", "tar_a;", "тарировка A"),
        field_line("float", "tar_b;", "тарировка B"),
        field_line("float", "tar_c;", "тарировка C"),
        field_line("uint16_t", "msg_offset;", "смещение в сообщении"),
        field_line("char", "msg_block_offset;", "смещение в блоке данных"),
        field_line("char", "msg_block_n;", "номер блока данных"),
        field_line("char", "alg;", "алгоритм обработки"),
        "};\n",
        "struct S_commtab S_basecommtab[Comm_max]={",
    ]

    init_rows: list[str] = []
    comments: list[str] = []
    trailing_comma: list[bool] = []
    for idx, row in enumerate(commands):
        init_rows.append(
            (
                f"    {{SYS_{sanitize_identifier(row.system_name)}, "
                f"TYPE_{sanitize_identifier(row.type_name)}, "
                f"{sign_macro(row.sign)}, "
                f"{to_cpp_number(row.tar_a)}, "
                f"{to_cpp_number(row.tar_b)}, "
                f"{to_cpp_number(row.tar_c)}, "
                f"{row.msg_offset}, "
                f"{row.msg_block_offset}, "
                f"{row.msg_block_n}, "
                f"ALG_{sanitize_identifier(row.alg_name)}}}"
            )
        )
        comments.append(make_table_comment(row.name, row.description))
        trailing_comma.append(idx != len(commands) - 1)

    lines.extend(align_table_rows(init_rows, comments, trailing_comma))

    lines.append("};\n")
    return "\n".join(lines)


def write_file(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8", newline="\n")


# -----------------------------------------------------------------------------
# entry point
# -----------------------------------------------------------------------------

def main() -> None:
    workbook = load_data_book(EXCEL_PATH)

    params = parse_params(workbook)
    commands = parse_commands(workbook)
    compute_cs_offsets(params)

    systems = collect_systems(params, commands)
    algorithms = collect_algorithms(params, commands)
    types = collect_types(params, commands)

    ensure_output_dir(OUTPUT_DIR)

    outputs: dict[str, str] = {
        "sys_list.hpp": make_sys_list(systems),
        "alg_list.hpp": make_alg_list(algorithms),
        "type_list.hpp": make_type_list(types),
        "param_list.hpp": make_param_list(params),
        "param_tab.hpp": make_param_tab(params),
        "command_list.hpp": make_command_list(commands),
        "command_tab.hpp": make_command_tab(commands),
    }

    for filename, content in outputs.items():
        write_file(OUTPUT_DIR / filename, content)

    print(f"Generated {len(outputs)} files in: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
