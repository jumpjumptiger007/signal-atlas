#!/usr/bin/env python3
"""Build deterministic, flash-resident Signal Atlas registry tables."""

import argparse
import csv
import hashlib
import json
import pathlib
import re
import sys

NORDIC_REVISION = "d379cc06c9c3681bc1c1cd4d61c246a0b0f80b0e"
MAX_NAME_BYTES = 0xFFFFFFFF


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_json(path):
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, list):
        raise ValueError(f"{path}: expected a JSON array")
    return value


def ieee_rows(path, expected_registry, bits):
    grouped = {}
    with path.open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        required = {"Registry", "Assignment", "Organization Name"}
        if not reader.fieldnames or not required.issubset(reader.fieldnames):
            raise ValueError(f"{path}: unexpected IEEE CSV header")
        for number, row in enumerate(reader, 2):
            if row.get("Registry") != expected_registry:
                raise ValueError(f"{path}:{number}: unexpected registry type")
            assignment = (row.get("Assignment") or "").strip().replace("-", "").replace(":", "")
            name = (row.get("Organization Name") or "").strip()
            if len(assignment) * 4 != bits or not re.fullmatch(r"[0-9A-Fa-f]+", assignment) or not name:
                raise ValueError(f"{path}:{number}: malformed assignment or organization")
            prefix = bytes.fromhex(assignment.ljust(12, "0"))
            grouped.setdefault(prefix, set()).add(name)
    conflicts = {prefix.hex().upper(): sorted(names) for prefix, names in grouped.items() if len(names) > 1}
    # Ambiguous source assignments must not be presented as factual vendors.
    # Preserve the input and expose each excluded prefix in generated metadata.
    rows = [(prefix, next(iter(names))) for prefix, names in grouped.items() if len(names) == 1]
    rows.sort(key=lambda item: item[0])
    return rows, conflicts


def keyed_rows(path, key_name, key_field, *, hex_key=False, max_key=0xFFFF):
    rows = []
    for index, row in enumerate(load_json(path)):
        try:
            raw = row[key_field]
            key = int(raw, 16) if hex_key else int(raw)
            name = str(row[key_name]).strip()
        except (KeyError, TypeError, ValueError) as exc:
            raise ValueError(f"{path}: entry {index} is malformed") from exc
        if not 0 <= key <= max_key or not name:
            raise ValueError(f"{path}: entry {index} is outside the compact representation")
        rows.append((key, name))
    rows.sort()
    for left, right in zip(rows, rows[1:]):
        if left[0] == right[0]:
            raise ValueError(f"{path}: duplicate key {left[0]}")
    return rows


def appearance_rows(path):
    rows = []
    for index, row in enumerate(load_json(path)):
        try:
            category = int(row["category"])
            if not 0 <= category <= 0x3FF:
                raise ValueError
            # Assigned Numbers stores a category at its six-bit-aligned base;
            # flatten subcategories into the 16-bit GAP Appearance code.
            if "subcategory" in row:
                rows.append((category << 6, str(row["name"]).strip()))
                for item in row["subcategory"]:
                    value = int(item["value"])
                    rows.append(((category << 6) | value, str(item["name"]).strip()))
            else:
                rows.append((category << 6, str(row["name"]).strip()))
        except (KeyError, TypeError, ValueError) as exc:
            raise ValueError(f"{path}: appearance entry {index} is malformed") from exc
    rows.sort()
    for left, right in zip(rows, rows[1:]):
        if left[0] == right[0]:
            raise ValueError(f"{path}: duplicate appearance {left[0]}")
    return rows


def service_rows(path):
    rows = []
    base = bytes.fromhex("0000000000001000800000805f9b34fb")
    for index, row in enumerate(load_json(path)):
        try:
            compact = re.sub(r"-", "", row["uuid"]).upper()
            if not re.fullmatch(r"(?:[0-9A-F]{4}|[0-9A-F]{8}|[0-9A-F]{32})", compact):
                raise ValueError
            raw = bytes.fromhex(compact)
            if len(raw) == 2:
                expanded = bytearray(base); expanded[2:4] = raw
            elif len(raw) == 4:
                expanded = bytearray(base); expanded[0:4] = raw
            else:
                expanded = bytearray(raw)
            rows.append((bytes(expanded), str(row["name"]).strip()))
        except (KeyError, TypeError, ValueError) as exc:
            raise ValueError(f"{path}: service UUID entry {index} is malformed") from exc
    rows.sort()
    for left, right in zip(rows, rows[1:]):
        if left[0] == right[0]:
            raise ValueError(f"{path}: duplicate service UUID {left[0].hex()}")
    return rows


def build(args):
    inputs = {
        "ieee_ma_l": pathlib.Path(args.ma_l), "ieee_ma_m": pathlib.Path(args.ma_m),
        "ieee_ma_s": pathlib.Path(args.ma_s), "company": pathlib.Path(args.company),
        "service": pathlib.Path(args.service), "appearance": pathlib.Path(args.appearance),
    }
    for path in inputs.values():
        if not path.is_file():
            raise ValueError(f"missing input: {path}")

    ieee_inputs = {
        "ieee_ma_l": ieee_rows(inputs["ieee_ma_l"], "MA-L", 24),
        "ieee_ma_m": ieee_rows(inputs["ieee_ma_m"], "MA-M", 28),
        "ieee_ma_s": ieee_rows(inputs["ieee_ma_s"], "MA-S", 36),
    }
    ieee = {key: value[0] for key, value in ieee_inputs.items()}
    conflicts = {key: value[1] for key, value in ieee_inputs.items() if value[1]}
    bluetooth = {
        "company": keyed_rows(inputs["company"], "name", "code"),
        "service": service_rows(inputs["service"]),
        "appearance": appearance_rows(inputs["appearance"]),
    }

    pool = bytearray(b"\0")
    offsets = {"": 0}
    def intern(name):
        if name not in offsets:
            encoded = name.encode("utf-8") + b"\0"
            if len(pool) + len(encoded) > MAX_NAME_BYTES:
                raise ValueError("deduplicated registry names exceed 32-bit offset boundary")
            offsets[name] = len(pool)
            pool.extend(encoded)
            if len(pool) > 0xFFFFFF:
                raise ValueError("deduplicated registry names exceed 24-bit offset boundary")
        return offsets[name]

    tables = {}
    for table, rows in ieee.items():
        tables[table] = [(prefix, intern(name)) for prefix, name in rows]
    for table, rows in bluetooth.items():
        if table == "service": tables[table] = [(key, intern(name)) for key, name in rows]
        else: tables[table] = [(key, intern(name)) for key, name in rows]

    digest = hashlib.sha256()
    for name, path in inputs.items():
        digest.update(name.encode() + b"\0" + bytes.fromhex(sha(path)))
    digest.update(NORDIC_REVISION.encode())
    build_id = digest.hexdigest()[:12]

    output = pathlib.Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    header = '''/* Generated by tools/generate_registries.py; do not edit. */
#ifndef RADIO_EXPLORER_REGISTRY_DATA_H
#define RADIO_EXPLORER_REGISTRY_DATA_H
#include <stddef.h>
#include <stdint.h>
typedef struct __attribute__((packed)) { uint8_t prefix[6]; uint8_t name_offset[3]; } registry_ieee_record_t;
typedef struct __attribute__((packed)) { uint16_t key; uint8_t name_offset[3]; } registry_name_record_t;
typedef struct __attribute__((packed)) { uint8_t uuid[16]; uint8_t name_offset[3]; } registry_uuid_record_t;
extern const registry_ieee_record_t radio_ieee_ma_l[];
extern const size_t radio_ieee_ma_l_count;
extern const registry_ieee_record_t radio_ieee_ma_m[];
extern const size_t radio_ieee_ma_m_count;
extern const registry_ieee_record_t radio_ieee_ma_s[];
extern const size_t radio_ieee_ma_s_count;
extern const registry_name_record_t radio_company_records[];
extern const size_t radio_company_records_count;
extern const registry_uuid_record_t radio_service_records[];
extern const size_t radio_service_records_count;
extern const registry_name_record_t radio_appearance_records[];
extern const size_t radio_appearance_records_count;
extern const char radio_registry_strings[];
extern const size_t radio_registry_strings_size;
#define RADIO_REGISTRY_BUILD_ID "''' + build_id + '''"
#endif
'''
    source = ['/* Generated by tools/generate_registries.py; do not edit. */', '#include "registry_data.h"', '']
    for table in ("ieee_ma_l", "ieee_ma_m", "ieee_ma_s"):
        source.append(f"const registry_ieee_record_t radio_{table}[] = {{")
        source.extend(f"    {{{{{', '.join(f'0x{b:02x}' for b in prefix)}}}, {{{', '.join(f'0x{(off >> shift) & 0xff:02x}' for shift in (0, 8, 16))}}}}}," for prefix, off in tables[table])
        source.append("};")
        source.append(f"const size_t radio_{table}_count = sizeof(radio_{table}) / sizeof(radio_{table}[0]);\n")
    for table in ("company", "appearance"):
        source.append(f"const registry_name_record_t radio_{table}_records[] = {{")
        source.extend(f"    {{{key}u, {{{', '.join(f'0x{(off >> shift) & 0xff:02x}' for shift in (0, 8, 16))}}}}}," for key, off in tables[table])
        source.append("};")
        source.append(f"const size_t radio_{table}_records_count = sizeof(radio_{table}_records) / sizeof(radio_{table}_records[0]);\n")
    source.append("const registry_uuid_record_t radio_service_records[] = {")
    source.extend(f"    {{{{{', '.join(f'0x{b:02x}' for b in uuid)}}}, {{{', '.join(f'0x{(off >> shift) & 0xff:02x}' for shift in (0, 8, 16))}}}}}," for uuid, off in tables["service"])
    source.append("};")
    source.append("const size_t radio_service_records_count = sizeof(radio_service_records) / sizeof(radio_service_records[0]);\n")
    # Emit bytes instead of a huge C string literal to preserve every UTF-8 name exactly.
    source.append("const char radio_registry_strings[] = {")
    source.extend("    " + ", ".join(f"0x{b:02x}" for b in pool[i:i+16]) + "," for i in range(0, len(pool), 16))
    source.append("};")
    source.append(f"const size_t radio_registry_strings_size = {len(pool)}u;")

    metadata = {
        "generator": "tools/generate_registries.py", "build_id": build_id,
        "sources": {name: {"sha256": sha(path), "path": {
            "ieee_ma_l": "tools/registry_sources/ieee/ma-l.csv",
            "ieee_ma_m": "tools/registry_sources/ieee/ma-m.csv",
            "ieee_ma_s": "tools/registry_sources/ieee/ma-s.csv",
            "company": "tools/registry_sources/nordic/v1/company_ids.json",
            "service": "tools/registry_sources/nordic/v1/service_uuids.json",
            "appearance": "tools/registry_sources/nordic/v1/gap_appearance.json",
        }[name]} for name, path in inputs.items()},
        "nordic_revision": NORDIC_REVISION,
        "ieee_retrieved_utc_date": args.ieee_date,
        "excluded_ambiguous_ieee_prefixes": conflicts,
        "counts": {key: len(value) for key, value in tables.items()},
        "unique_names": len(offsets),
        "logical_flash_bytes": sum(len(rows) * (9 if key.startswith("ieee_") else 19 if key == "service" else 5) for key, rows in tables.items()) + len(pool),
        "strings_bytes": len(pool),
    }
    (output / "registry_data.h").write_text(header, encoding="utf-8", newline="\n")
    (output / "registry_data.c").write_text("\n".join(source) + "\n", encoding="utf-8", newline="\n")
    (output / "registry_metadata.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(metadata, sort_keys=True))


def registry_header(build_id):
    return '''/* Generated by tools/generate_registries.py; do not edit. */
#ifndef RADIO_EXPLORER_REGISTRY_DATA_H
#define RADIO_EXPLORER_REGISTRY_DATA_H
#include <stddef.h>
#include <stdint.h>
typedef struct __attribute__((packed)) { uint8_t prefix[6]; uint8_t name_offset[3]; } registry_ieee_record_t;
typedef struct __attribute__((packed)) { uint16_t key; uint8_t name_offset[3]; } registry_name_record_t;
typedef struct __attribute__((packed)) { uint8_t uuid[16]; uint8_t name_offset[3]; } registry_uuid_record_t;
extern const registry_ieee_record_t radio_ieee_ma_l[];
extern const size_t radio_ieee_ma_l_count;
extern const registry_ieee_record_t radio_ieee_ma_m[];
extern const size_t radio_ieee_ma_m_count;
extern const registry_ieee_record_t radio_ieee_ma_s[];
extern const size_t radio_ieee_ma_s_count;
extern const char radio_ieee_strings[];
extern const size_t radio_ieee_strings_size;
extern const registry_name_record_t radio_company_records[];
extern const size_t radio_company_records_count;
extern const registry_uuid_record_t radio_service_records[];
extern const size_t radio_service_records_count;
extern const registry_name_record_t radio_appearance_records[];
extern const size_t radio_appearance_records_count;
extern const char radio_registry_strings[];
extern const size_t radio_registry_strings_size;
#define RADIO_REGISTRY_BUILD_ID "''' + build_id + '''"
#endif
'''


def string_pool(tables):
    pool = bytearray(b"\0")
    offsets = {"": 0}

    def intern(name):
        if name not in offsets:
            encoded = name.encode("utf-8") + b"\0"
            if len(pool) + len(encoded) > MAX_NAME_BYTES:
                raise ValueError("deduplicated registry names exceed 32-bit offset boundary")
            offsets[name] = len(pool)
            pool.extend(encoded)
            if len(pool) > 0xFFFFFF:
                raise ValueError("deduplicated registry names exceed 24-bit offset boundary")
        return offsets[name]

    packed = {}
    for table, rows in tables.items():
        packed[table] = [(key, intern(name)) for key, name in rows]
    return pool, packed


def emit_ieee_source(packed, pool):
    source = ['/* Generated locally from developer-supplied IEEE CSVs; do not redistribute. */', '#include "registry_data.h"', '']
    for table in ("ieee_ma_l", "ieee_ma_m", "ieee_ma_s"):
        source.append(f"const registry_ieee_record_t radio_{table}[] = {{")
        source.extend(
            f"    {{{{{', '.join(f'0x{b:02x}' for b in prefix)}}}, {{{', '.join(f'0x{(off >> shift) & 0xff:02x}' for shift in (0, 8, 16))}}}}},"
            for prefix, off in packed[table]
        )
        if not packed[table]:
            source.append("    {{0}, {0}},")
        source.append("};")
        source.append(f"const size_t radio_{table}_count = {len(packed[table])}u;\n")
    source.append("const char radio_ieee_strings[] = {")
    source.extend("    " + ", ".join(f"0x{b:02x}" for b in pool[i:i+16]) + "," for i in range(0, len(pool), 16))
    source.append("};")
    source.append(f"const size_t radio_ieee_strings_size = {len(pool)}u;")
    return "\n".join(source) + "\n"


def build_public(args):
    inputs = {
        "company": pathlib.Path(args.company),
        "service": pathlib.Path(args.service),
        "appearance": pathlib.Path(args.appearance),
    }
    for path in inputs.values():
        if not path.is_file():
            raise ValueError(f"missing input: {path}")
    bluetooth = {
        "company": keyed_rows(inputs["company"], "name", "code"),
        "service": service_rows(inputs["service"]),
        "appearance": appearance_rows(inputs["appearance"]),
    }
    pool, tables = string_pool(bluetooth)
    digest = hashlib.sha256()
    for name, path in inputs.items():
        digest.update(name.encode() + b"\0" + bytes.fromhex(sha(path)))
    digest.update(NORDIC_REVISION.encode())
    build_id = digest.hexdigest()[:12]

    output = pathlib.Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    source = ['/* Generated by tools/generate_registries.py; do not edit. */', '#include "registry_data.h"', '']
    for table in ("company", "appearance"):
        source.append(f"const registry_name_record_t radio_{table}_records[] = {{")
        source.extend(f"    {{{key}u, {{{', '.join(f'0x{(off >> shift) & 0xff:02x}' for shift in (0, 8, 16))}}}}}," for key, off in tables[table])
        source.append("};")
        source.append(f"const size_t radio_{table}_records_count = {len(tables[table])}u;\n")
    source.append("const registry_uuid_record_t radio_service_records[] = {")
    source.extend(f"    {{{{{', '.join(f'0x{b:02x}' for b in uuid)}}}, {{{', '.join(f'0x{(off >> shift) & 0xff:02x}' for shift in (0, 8, 16))}}}}}," for uuid, off in tables["service"])
    source.append("};")
    source.append(f"const size_t radio_service_records_count = {len(tables['service'])}u;\n")
    source.append("const char radio_registry_strings[] = {")
    source.extend("    " + ", ".join(f"0x{b:02x}" for b in pool[i:i+16]) + "," for i in range(0, len(pool), 16))
    source.append("};")
    source.append(f"const size_t radio_registry_strings_size = {len(pool)}u;")
    metadata = {
        "generator": "tools/generate_registries.py", "build_id": build_id,
        "sources": {name: {"sha256": sha(path), "file": path.name} for name, path in inputs.items()},
        "nordic_revision": NORDIC_REVISION, "ieee_records": 0,
        "counts": {key: len(value) for key, value in tables.items()},
        "unique_names": len({"", *(name for rows in bluetooth.values() for _, name in rows)}),
        "logical_flash_bytes": sum(len(rows) * (19 if key == "service" else 5) for key, rows in tables.items()) + len(pool),
        "strings_bytes": len(pool),
    }
    (output / "registry_data.h").write_text(registry_header(build_id), encoding="utf-8", newline="\n")
    (output / "registry_data.c").write_text("\n".join(source) + "\n", encoding="utf-8", newline="\n")
    (output / "registry_data_ieee_empty.c").write_text(
        '/* Default public build: IEEE-derived data is not redistributed. */\n'
        '#include "registry_data.h"\n'
        'const registry_ieee_record_t radio_ieee_ma_l[1] = {{{0}, {0}}};\n'
        'const size_t radio_ieee_ma_l_count = 0u;\n'
        'const registry_ieee_record_t radio_ieee_ma_m[1] = {{{0}, {0}}};\n'
        'const size_t radio_ieee_ma_m_count = 0u;\n'
        'const registry_ieee_record_t radio_ieee_ma_s[1] = {{{0}, {0}}};\n'
        'const size_t radio_ieee_ma_s_count = 0u;\n'
        'const char radio_ieee_strings[] = {0};\n'
        'const size_t radio_ieee_strings_size = 1u;\n', encoding="utf-8", newline="\n")
    (output / "registry_metadata.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(metadata, sort_keys=True))


def build_ieee_local(args):
    inputs = {"ieee_ma_l": pathlib.Path(args.ma_l), "ieee_ma_m": pathlib.Path(args.ma_m), "ieee_ma_s": pathlib.Path(args.ma_s)}
    for path in inputs.values():
        if not path.is_file():
            raise ValueError(f"missing input: {path}")
    parsed = {
        "ieee_ma_l": ieee_rows(inputs["ieee_ma_l"], "MA-L", 24),
        "ieee_ma_m": ieee_rows(inputs["ieee_ma_m"], "MA-M", 28),
        "ieee_ma_s": ieee_rows(inputs["ieee_ma_s"], "MA-S", 36),
    }
    rows = {key: value[0] for key, value in parsed.items()}
    conflicts = {key: value[1] for key, value in parsed.items() if value[1]}
    pool, tables = string_pool(rows)
    output = pathlib.Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    (output / "registry_data_ieee_local.c").write_text(emit_ieee_source(tables, pool), encoding="utf-8", newline="\n")
    metadata = {
        "generator": "tools/generate_registries.py", "distribution": "local-only-not-for-redistribution",
        "sources": {name: {"sha256": sha(path), "path": str(path)} for name, path in inputs.items()},
        "retrieved_utc_date": args.ieee_date,
        "counts": {key: len(value) for key, value in tables.items()},
        "excluded_ambiguous_prefixes": conflicts, "strings_bytes": len(pool),
    }
    (output / "registry_metadata_ieee_local.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(metadata, sort_keys=True))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=("public", "ieee-local"), default="public")
    parser.add_argument("--ma-l"); parser.add_argument("--ma-m"); parser.add_argument("--ma-s")
    parser.add_argument("--company", default="tools/registry_sources/nordic/v1/company_ids.json")
    parser.add_argument("--service", default="tools/registry_sources/nordic/v1/service_uuids.json")
    parser.add_argument("--appearance", default="tools/registry_sources/nordic/v1/gap_appearance.json")
    parser.add_argument("--output", default="main/identify")
    parser.add_argument("--ieee-date")
    args = parser.parse_args()
    try:
        if args.mode == "public":
            build_public(args)
        else:
            if not all((args.ma_l, args.ma_m, args.ma_s, args.ieee_date)):
                raise ValueError("ieee-local mode requires --ma-l, --ma-m, --ma-s and --ieee-date")
            build_ieee_local(args)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"registry generation failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
