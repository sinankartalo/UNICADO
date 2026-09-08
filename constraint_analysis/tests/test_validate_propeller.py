"""Regression checks for user-defined propeller validation conditions."""
import importlib.util
from pathlib import Path
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/validate_propeller.py"
spec = importlib.util.spec_from_file_location("validator", SCRIPT)
v = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)


class PerformanceConditionsTest(unittest.TestCase):
    def test_turn_uses_own_weight_fraction(self):
        settings = v.constraint_set()
        original = v.performance_checks(settings)
        settings.find("./constant_speed_turn/weight_fraction/value").text = "0.8"
        changed = v.performance_checks(settings)
        self.assertLess(changed["propeller_turn_constraint"],
                        original["propeller_turn_constraint"])
        self.assertEqual(changed["propeller_subsonic_cruise_constraint"],
                         original["propeller_subsonic_cruise_constraint"])

    def test_acceleration_increment_matches_energy_balance(self):
        settings = v.constraint_set()
        original = v.performance_checks(settings)["propeller_acceleration_constraint"]
        node = settings.find("./horizontal_acceleration/acceleration/value")
        node.text = str(float(node.text) + 0.5)
        changed = v.performance_checks(settings)["propeller_acceleration_constraint"]
        ct, cp, _, rpm, _, _ = v.best_airborne_prop_row(8000.0, 180.0)
        expected_increment = 0.98 * 0.5 / v.G0 * cp / ct * rpm / 60 * v.DIAMETER_M
        self.assertAlmostEqual(changed - original, expected_increment)

    def test_missing_requirement_fails_explicitly(self):
        settings = v.constraint_set()
        climb = settings.find("./climb")
        climb.remove(climb.find("weight_fraction"))
        with self.assertRaisesRegex(ValueError, "climb/weight_fraction"):
            v.performance_checks(settings)


if __name__ == "__main__":
    unittest.main()
