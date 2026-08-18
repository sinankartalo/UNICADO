"""Independent hand-calculation checks for the propeller matching chart."""

from __future__ import annotations

import csv
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PROP_CSV = ROOT / "libraries/aerodynamics/test/stubs/engine/propeller/prop.csv"
MISSION_CSV = (
    ROOT
    / "UNICADO-SMR-01/clean_sheet_design/mission_data"
    / "UNICADO-SMR-01_design_mission_R2450_PL19300_out.csv"
)
OUTPUT_ROOT = ROOT / "constraint_analysis/output"
CASE_OUTPUT = OUTPUT_ROOT / "UNICADO_PROPELLER"
OUTPUT = CASE_OUTPUT if CASE_OUTPUT.exists() else OUTPUT_ROOT

CD0 = 0.00455002
K = 0.0217487
CLMAX_TO = 2.111
DIAMETER_M = 3.96
RPM = 1200.0
WS = 4000.0
G0 = 9.80665
TIP_MACH_LIMIT = 0.95


def isa_density(altitude_m: float) -> float:
    temperature = 288.15 - 0.0065 * altitude_m
    pressure = 101325.0 * (temperature / 288.15) ** 5.255877
    return pressure / (287.05287 * temperature)


def isa_speed_of_sound(altitude_m: float) -> float:
    temperature = 288.15 - 0.0065 * altitude_m
    return math.sqrt(1.4 * 287.05287 * temperature)


def tip_mach(altitude_m: float, speed_ms: float, rpm: float = RPM) -> float:
    rotational_tip_speed = math.pi * DIAMETER_M * rpm / 60.0
    helical_tip_speed = math.hypot(speed_ms, rotational_tip_speed)
    return helical_tip_speed / isa_speed_of_sound(altitude_m)


def prop_row(pitch_deg: float, advance_ratio: float) -> tuple[float, float, float]:
    rows: list[tuple[float, float, float, float]] = []
    with PROP_CSV.open(encoding="utf-8-sig") as stream:
        for row in csv.reader(stream):
            if float(row[0]) == pitch_deg:
                rows.append(tuple(map(float, row[1:5])))

    for left, right in zip(rows, rows[1:]):
        if left[0] <= advance_ratio <= right[0]:
            ratio = (advance_ratio - left[0]) / (right[0] - left[0])
            values = [
                left[index] + ratio * (right[index] - left[index])
                for index in range(1, 4)
            ]
            return values[0], values[1], values[2]
    raise ValueError(f"J={advance_ratio} lies outside pitch={pitch_deg} deck data")


def best_takeoff_prop_row(
    advance_ratio: float,
) -> tuple[float, float, float]:
    candidates = []
    for pitch, low, high in ((15.0, 0.0, 1.05),
                             (30.0, 0.5, 1.5),
                             (45.0, 0.75, 2.8)):
        if low <= advance_ratio <= high:
            ct, cp, eta = prop_row(pitch, advance_ratio)
            candidates.append((cp / ct, ct, cp, eta))
    if not candidates:
        raise ValueError(f"No takeoff pitch slice covers J={advance_ratio}")
    _, ct, cp, eta = min(candidates)
    return ct, cp, eta


def mission_betas() -> dict[str, float]:
    with MISSION_CSV.open(encoding="utf-8-sig") as stream:
        rows = list(csv.reader(stream, delimiter=";"))
    header = [cell.strip() for cell in rows[0]]
    altitude_index = header.index("Altitude [m]")
    mode_index = header.index("Mode name [-]")
    mass_index = header.index("Total mass [kg]")
    data = [
        (float(row[altitude_index]), row[mode_index].strip(), float(row[mass_index]))
        for row in rows[1:]
    ]
    initial_mass = data[0][2]

    def at_altitude(mode: str, altitude_m: float) -> float:
        return next(
            mass / initial_mass
            for altitude, row_mode, mass in data
            if row_mode == mode and altitude >= altitude_m
        )

    cruise_end = next(
        data[index][2] / initial_mass
        for index in range(1, len(data))
        if data[index - 1][1] == "cruise" and data[index][1] != "cruise"
    )
    return {
        "takeoff": at_altitude("takeoff", 0.0),
        "cruise": at_altitude("cruise", 6000.0),
        "climb": at_altitude("climb", 3000.0),
        "cruise_end": cruise_end,
    }


def takeoff_weight_N() -> float:
    with MISSION_CSV.open(encoding="utf-8-sig") as stream:
        rows = list(csv.reader(stream, delimiter=";"))
    header = [cell.strip() for cell in rows[0]]
    mass_index = header.index("Total mass [kg]")
    return float(rows[1][mass_index]) * G0


def airborne_power_loading(
    altitude_m: float,
    speed_ms: float,
    beta: float,
    load_factor: float = 1.0,
    roc_ms: float = 0.0,
    acceleration_ms2: float = 0.0,
) -> tuple[float, float, float, float]:
    rho = isa_density(altitude_m)
    q = 0.5 * rho * speed_ms**2
    cl = load_factor * beta * WS / q
    cd = CD0 + K * cl**2
    drag_to_weight = q * cd / (beta * WS)
    thrust_to_weight = beta * (
        drag_to_weight + roc_ms / speed_ms + acceleration_ms2 / G0
    )
    n = RPM / 60.0
    advance_ratio = speed_ms / (n * DIAMETER_M)
    ct, cp, eta = prop_row(45.0, advance_ratio)
    power_to_thrust = (cp / ct) * n * DIAMETER_M
    return thrust_to_weight * power_to_thrust, advance_ratio, eta, cl


def takeoff_distance(power_loading: float, beta: float) -> float:
    rho = isa_density(0.0)
    stall_speed = math.sqrt(2.0 * beta * WS / (rho * CLMAX_TO))
    takeoff_speed = 1.2 * stall_speed
    steps = 240
    dv = takeoff_speed / steps
    n = RPM / 60.0
    distance = 0.0
    for index in range(steps):
        speed = (index + 0.5) * dv
        advance_ratio = speed / (n * DIAMETER_M)
        ct, cp, _ = best_takeoff_prop_row(advance_ratio)
        thrust_to_weight = power_loading / ((cp / ct) * n * DIAMETER_M)
        q = 0.5 * rho * speed**2
        lift_to_weight = q * (0.8 * CLMAX_TO) / WS
        drag_to_weight = q * 0.04 / WS
        rolling_to_weight = 0.02 * max(0.0, beta - lift_to_weight)
        acceleration = (
            G0 / beta
            * (thrust_to_weight - drag_to_weight - rolling_to_weight)
        )
        if acceleration <= 0.0:
            return math.inf
        distance += speed * dv / acceleration
    return distance


def takeoff_power_loading(beta: float) -> float:
    lower = 0.0
    upper = 1.0
    while takeoff_distance(upper, beta) > 500.0:
        upper *= 2.0
    for _ in range(70):
        trial = 0.5 * (lower + upper)
        if takeoff_distance(trial, beta) > 500.0:
            lower = trial
        else:
            upper = trial
    return upper


def main() -> None:
    beta = mission_betas()
    checks = {
        "propeller_takeoff_constraint": takeoff_power_loading(beta["takeoff"]),
        "propeller_acceleration_constraint": airborne_power_loading(
            8000.0, 180.0, beta["cruise_end"], acceleration_ms2=1.5
        )[0],
        "propeller_cruise_constraint": airborne_power_loading(
            6000.0, 180.0, beta["cruise"]
        )[0],
        "propeller_climb_constraint": airborne_power_loading(
            3000.0, 120.0, beta["climb"], roc_ms=5.0
        )[0],
        "propeller_turn_constraint": airborne_power_loading(
            3000.0, 120.0, beta["cruise_end"], load_factor=2.5
        )[0],
    }

    print(f"Independent hand checks at W/S = {WS:.0f} N/m^2")
    for name, expected in checks.items():
        output_path = OUTPUT / f"{name}.csv"
        status = "program output not present"
        if output_path.exists():
            with output_path.open() as stream:
                rows = list(csv.DictReader(stream))
            matching = min(rows, key=lambda row: abs(float(row["x"]) - WS))
            actual = float(matching["y"])
            error = abs(actual - expected)
            status = f"program={actual:.8f}, abs_error={error:.3e}"
            if error > 2.0e-3:
                raise AssertionError(f"{name}: {status}")
        print(f"{name}: hand={expected:.8f} W/N, {status}")

    if not all(math.isfinite(value) and value > 0.0 for value in checks.values()):
        raise AssertionError("All propeller power-loading checks must be finite and positive")
    if not (checks["propeller_acceleration_constraint"] >
            checks["propeller_turn_constraint"] >
            checks["propeller_climb_constraint"] >
            checks["propeller_cruise_constraint"]):
        raise AssertionError("Airborne constraint ordering is physically inconsistent")

    required_envelope_W_N = max(checks.values())
    required_total_shaft_power_W = required_envelope_W_N * takeoff_weight_N()
    print(
        "required shaft-power check: "
        f"envelope={required_envelope_W_N:.8f} W/N, "
        f"total={required_total_shaft_power_W / 1.0e6:.8f} MW"
    )

    operating_points_path = OUTPUT / "propeller_operating_points.csv"
    if operating_points_path.exists():
        with operating_points_path.open() as stream:
            operating_points = list(csv.DictReader(stream))
        for row in operating_points:
            expected = tip_mach(
                float(row["altitude_m"]),
                float(row["speed_ms"]),
                float(row["rpm"]),
            )
            actual = float(row["tip_mach"])
            if abs(actual - expected) > 2.0e-5:
                raise AssertionError(
                    f"tip Mach mismatch: program={actual}, hand={expected}"
                )
            expected_status = "PASS" if expected <= TIP_MACH_LIMIT else "FAIL"
            if row["tip_mach_status"] != expected_status:
                raise AssertionError(
                    f"tip Mach status mismatch: {row['tip_mach_status']} "
                    f"!= {expected_status}"
                )
        maximum = max(float(row["tip_mach"]) for row in operating_points)
        print(
            f"tip Mach check: maximum={maximum:.6f}, "
            f"limit={TIP_MACH_LIMIT:.2f}, "
            f"status={'PASS' if maximum <= TIP_MACH_LIMIT else 'FAIL'}"
        )


if __name__ == "__main__":
    main()
