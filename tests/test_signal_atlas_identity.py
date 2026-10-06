from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class SignalAtlasIdentityTests(unittest.TestCase):
    def read(self, relative_path):
        return (ROOT / relative_path).read_text()

    def test_current_product_surfaces_use_signal_atlas(self):
        current_docs = (
            "README.md",
            "AGENTS.md",
            "docs/implementation-spec.md",
            "docs/gates.md",
            "docs/hardware-acceptance.md",
            "docs/codex-autopilot-goal.md",
            "docs/donor-audit.md",
        )
        for path in current_docs:
            with self.subTest(path=path):
                self.assertNotIn("Radio Explorer", self.read(path))

        self.assertTrue(self.read("README.md").startswith("# Signal Atlas\n"))
        self.assertIn('menu "Signal Atlas"', self.read("main/Kconfig.projbuild"))
        self.assertIn('"Signal Atlas v0.1 boot"', self.read("main/main.c"))
        self.assertIn('"SIGNAL ATLAS"', self.read("main/ui/ui_app.c"))
        self.assertIn('"Signal Atlas v0.1\\nOffline · read-only', self.read("main/ui/ui_app.c"))

    def test_historical_and_stable_identifiers_are_preserved(self):
        report = self.read("docs/execution-report.md")
        self.assertTrue(report.startswith("# Signal Atlas v0.1 Execution Report\n"))
        self.assertIn("originally executed under the Radio Explorer working name", report)
        self.assertIn("feature/radio-explorer", report)
        self.assertIn("config RADIO_EXPLORER_SIMULATOR", self.read("main/Kconfig.projbuild"))
        self.assertIn('"radio_exp"', self.read("main/storage/history_store.c"))
        self.assertIn('"history"', self.read("main/storage/history_store.c"))


if __name__ == "__main__":
    unittest.main()
