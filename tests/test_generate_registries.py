#!/usr/bin/env python3
"""Determinism, conflict hygiene, and longest-prefix generator integration tests."""

from __future__ import annotations

import csv
import json
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "tools" / "generate_registries.py"


class RegistryGeneratorTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="radio-explorer-registry-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        self.inputs = self.root / "inputs"
        self.inputs.mkdir()

    def write_csv(self, name: str, registry: str, assignments: list[tuple[str, str]]) -> pathlib.Path:
        path = self.inputs / name
        with path.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.writer(stream)
            writer.writerow(["Registry", "Assignment", "Organization Name", "Organization Address"])
            writer.writerows((registry, prefix, label, "") for prefix, label in assignments)
        return path

    def write_json(self, name: str, rows: list[dict]) -> pathlib.Path:
        path = self.inputs / name
        path.write_text(json.dumps(rows), encoding="utf-8")
        return path

    def fixture_args(self, output: pathlib.Path) -> list[str]:
        files = {
            "--ma-l": self.write_csv("ma-l.csv", "MA-L", [("001122", "large")]),
            "--ma-m": self.write_csv("ma-m.csv", "MA-M", [("0011223", "medium")]),
            "--ma-s": self.write_csv("ma-s.csv", "MA-S", [("001122334", "small")]),
        }
        args = ["python3", str(GENERATOR), "--mode", "ieee-local"]
        for name, path in files.items():
            args.extend([name, str(path)])
        args.extend(["--output", str(output), "--ieee-date", "2026-10-05"])
        return args

    def test_identical_inputs_generate_byte_identical_artifacts(self) -> None:
        out_a, out_b = self.root / "out-a", self.root / "out-b"
        subprocess.run(self.fixture_args(out_a), check=True, capture_output=True, text=True)
        subprocess.run(self.fixture_args(out_b), check=True, capture_output=True, text=True)
        for name in ("registry_data_ieee_local.c", "registry_metadata_ieee_local.json"):
            self.assertEqual((out_a / name).read_bytes(), (out_b / name).read_bytes(), name)

    def test_public_generation_matches_committed_nordic_only_provider(self) -> None:
        output = self.root / "public"
        subprocess.run(["python3", str(GENERATOR), "--mode", "public", "--output", str(output)],
                       check=True, capture_output=True, text=True)
        identify = ROOT / "main" / "identify"
        for name in ("registry_data.c", "registry_data.h", "registry_data_ieee_empty.c", "registry_metadata.json"):
            self.assertEqual((output / name).read_bytes(), (identify / name).read_bytes(), name)
        metadata = json.loads((output / "registry_metadata.json").read_text(encoding="utf-8"))
        self.assertEqual(metadata["ieee_records"], 0)
        self.assertNotIn("radio_ieee_ma_l[] = {", (output / "registry_data.c").read_text(encoding="utf-8"))

    def test_ieee_longest_prefix_lookup_prefers_36_then_28_then_24(self) -> None:
        output = self.root / "generated"
        subprocess.run(self.fixture_args(output), check=True, capture_output=True, text=True)
        identify = self.root / "identify"
        model = self.root / "model"
        identify.mkdir(); model.mkdir()
        shutil.copyfile(ROOT / "main" / "identify" / "registry.c", identify / "registry.c")
        shutil.copyfile(ROOT / "main" / "identify" / "registry.h", identify / "registry.h")
        shutil.copyfile(ROOT / "main" / "identify" / "registry_data.c", identify / "registry_data.c")
        shutil.copyfile(ROOT / "main" / "identify" / "registry_data.h", identify / "registry_data.h")
        shutil.copyfile(output / "registry_data_ieee_local.c", identify / "registry_data_ieee_local.c")
        shutil.copyfile(ROOT / "main" / "model" / "observation.h", model / "observation.h")
        shutil.copyfile(ROOT / "main" / "model" / "observation.c", model / "observation.c")
        harness = self.root / "lookup.c"
        harness.write_text(
            '#include "registry.h"\n#include <assert.h>\n#include <string.h>\n'
            'int main(void){uint8_t bits=0;const uint8_t s[]={0x00,0x11,0x22,0x33,0x4f,0x01};'
            'const uint8_t m[]={0x00,0x11,0x22,0x3f,0x4f,0x01};'
            'const uint8_t l[]={0x00,0x11,0x22,0x50,0x00,0x01};'
            'assert(strcmp(radio_ieee_lookup(s,&bits),"small")==0&&bits==36);'
            'assert(strcmp(radio_ieee_lookup(m,&bits),"medium")==0&&bits==28);'
            'assert(strcmp(radio_ieee_lookup(l,&bits),"large")==0&&bits==24);return 0;}\n',
            encoding="utf-8",
        )
        executable = self.root / "lookup"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", str(identify),
            "-I", str(model), str(harness), str(identify / "registry.c"),
            str(identify / "registry_data.c"), str(identify / "registry_data_ieee_local.c"),
            str(model / "observation.c"), "-o", str(executable),
        ], check=True, capture_output=True, text=True)
        subprocess.run([str(executable)], check=True)

    def test_conflicting_source_assignments_are_explicitly_excluded(self) -> None:
        ma_l = self.write_csv("conflict.csv", "MA-L", [("001122", "First Org"), ("001122", "Second Org")])
        args = self.fixture_args(self.root / "conflict-out")
        args[args.index("--ma-l") + 1] = str(ma_l)
        completed = subprocess.run(args, check=True, capture_output=True, text=True)
        metadata = json.loads((self.root / "conflict-out" / "registry_metadata_ieee_local.json").read_text())
        self.assertEqual(
            metadata["excluded_ambiguous_prefixes"]["ieee_ma_l"]["001122000000"],
            ["First Org", "Second Org"],
        )
        self.assertIn("excluded_ambiguous_prefixes", completed.stdout)

    def test_wrong_registry_or_malformed_ieee_assignment_fails_closed(self) -> None:
        output = self.root / "bad"
        args = self.fixture_args(output)
        ma_m_path = pathlib.Path(args[args.index("--ma-m") + 1])
        ma_m_path.write_text(ma_m_path.read_text(encoding="utf-8").replace("MA-M", "MA-L"), encoding="utf-8")
        completed = subprocess.run(args, capture_output=True, text=True)
        self.assertNotEqual(completed.returncode, 0)
        self.assertIn("unexpected registry type", completed.stderr)


if __name__ == "__main__":
    unittest.main()
