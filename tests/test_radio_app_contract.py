from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class RadioAppContractTests(unittest.TestCase):
    def test_product_entry_path_is_not_the_demo_shell(self):
        main = (ROOT / "main/main.c").read_text()
        cmake = (ROOT / "main/CMakeLists.txt").read_text()
        app = (ROOT / "main/app.c").read_text()
        self.assertIn("radio_explorer_app_init", main)
        self.assertNotIn("demo_", main)
        self.assertNotIn("bsp_audio", main)
        self.assertNotIn("ui_pixel", main + cmake + app)
        # Gate 3 intentionally reuses the frozen radio/NVS preparation donor,
        # but product UI/demo shell sources remain excluded from this target.
        self.assertIn('"demo_radio.c"', cmake)
        for demo_shell in ('"demo_navigation.c"', '"demo_wifi.c"', '"demo_ble.c"',
                           '"demo_display.c"', '"demo_button.c"', '"demo_audio.c"'):
            self.assertNotIn(demo_shell, cmake)
        self.assertNotIn("nimble_port_init", app.lower())

    def test_simulator_profile_is_explicit_and_mock_only(self):
        cmake = (ROOT / "main/CMakeLists.txt").read_text()
        profile = (ROOT / "sdkconfig.simulator.defaults").read_text()
        app = (ROOT / "main/app.c").read_text()
        mock = (ROOT / "main/radio/mock_scanner.c").read_text()
        self.assertIn("if(CONFIG_RADIO_EXPLORER_SIMULATOR)", cmake)
        self.assertIn('"radio/mock_scanner.c"', cmake)
        self.assertIn("CONFIG_RADIO_EXPLORER_SIMULATOR=y", profile)
        self.assertIn("CONFIG_BT_ENABLED=n", profile)
        runner = (ROOT / "tools/validate-simulator.sh").read_text()
        self.assertIn("ESP-IDF v5.5.3", runner)
        self.assertIn("radio-explorer-simulator", runner)
        self.assertIn("verify_firmware.py", runner)
        self.assertIn("mock_scanner_tick", app)
        self.assertIn("radio_ble_observation_from_ad", mock)
        self.assertNotIn("nimble_port_init", app.lower() + mock.lower())


if __name__ == "__main__":
    unittest.main()
