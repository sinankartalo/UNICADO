"""Independent hand-calculation checks for the propeller matching chart."""

from __future__ import annotations

import csv
import math
from pathlib import Path
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[2]
PROP_CSV = ROOT / "libraries/aerodynamics/test/stubs/engine/propeller/prop.csv"
MISSION_CSV = (
    ROOT
    / "UNICADO-SMR-01/clean_sheet_design/mission_data"
    / "UNICADO-SMR-01_design_mission_R2450_PL19300_out.csv"
)
OUTPUT_ROOT = ROOT / "constraint_analysis/output"
CASE_OUTPUT = OUTPUT_ROOT / "PROPELLER_UNICADO_BASELINE"
OUTPUT = CASE_OUTPUT if CASE_OUTPUT.exists() else OUTPUT_ROOT
CONFIG_XML = ROOT / "constraint_analysis/config/constraint_analysis_conf.xml"

CD0 = 0.00455002
K = 0.0217487
CLMAX_TO = 2.111
DIAMETER_M = 3.96
WS = 4000.0
G0 = 9.80665
TIP_MACH_LIMIT = 0.95


def case_ground_roll_requirement_m(case_id: str) -> float:
    """Read the selected case's takeoff ground-roll requirement from XML."""
    root = ET.parse(CONFIG_XML).getroot()
    case = root.find(
        f".//constraint_case[@ID='{case_id}']"
    )
    if case is None:
        raise ValueError(f"Constraint case not found: {case_id}")
    set_ref = case.findtext("./constraints/constraint_set_ref/value")
    standard_set = root.find(
        f".//standard_set[@ID='{set_ref}']"
    )
    if standard_set is None:
        raise ValueError(f"Constraint set not found: {set_ref}")
    value = standard_set.findtext(
        "./takeoff_ground_roll/takeoff_ground_roll_m/value"
    )
    if value is None:
        raise ValueError(
            f"Takeoff ground-roll requirement missing from set: {set_ref}"
        )
    return float(value)


def propeller_cruise_fallback() -> tuple[float, float]:
    root = ET.parse(CONFIG_XML).getroot()
    case = root.find(
        ".//constraint_case[@ID='PROPELLER_UNICADO_BASELINE']"
    )
    if case is None:
        raise ValueError("Propeller constraint case not found")
    altitude = case.findtext(
        "./engine/propeller/cruise_fallback/altitude/value"
    )
    speed = case.findtext(
        "./engine/propeller/cruise_fallback/speed/value"
    )
    if altitude is None or speed is None:
        raise ValueError("Propeller cruise fallback is incomplete")
    return float(altitude), float(speed)


def isa_density(altitude_m: float) -> float:
    temperature = 288.15 - 0.0065 * altitude_m
    pressure = 101325.0 * (temperature / 288.15) ** 5.255877
    return pressure / (287.05287 * temperature)


def isa_speed_of_sound(altitude_m: float) -> float:
    temperature = 288.15 - 0.0065 * altitude_m
    return math.sqrt(1.4 * 287.05287 * temperature)


def tip_mach(altitude_m: float, speed_ms: float, rpm: float) -> float:
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
            if any(
                value <= 0.0
                for value in (left[1], left[2], right[1], right[2])
            ):
                raise ValueError(
                    f"J={advance_ratio} crosses a non-positive "
                    f"pitch={pitch_deg} deck segment"
                )
            ratio = (advance_ratio - left[0]) / (right[0] - left[0])
            values = [
                left[index] + ratio * (right[index] - left[index])
                for index in range(1, 4)
            ]
            return values[0], values[1], values[2]
    raise ValueError(f"J={advance_ratio} lies outside pitch={pitch_deg} deck data")


def best_airborne_prop_row(
    altitude_m: float, speed_ms: float
) -> tuple[float, float, float, float, float, float]:
    candidates = []
    with PROP_CSV.open(encoding="utf-8-sig") as stream:
        for row in csv.reader(stream):
            pitch, advance_ratio, ct, cp, eta = map(float, row[:5])
            if advance_ratio <= 0.0 or min(ct, cp, eta) <= 0.0:
                continue
            rpm = 60.0 * speed_ms / (advance_ratio * DIAMETER_M)
            candidate_tip_mach = tip_mach(altitude_m, speed_ms, rpm)
            if candidate_tip_mach <= TIP_MACH_LIMIT:
                candidates.append(
                    (cp / ct * rpm / 60.0 * DIAMETER_M,
                     ct, cp, eta, rpm, pitch, advance_ratio)
                )
    if not candidates:
        raise ValueError(
            "No tip-Mach-feasible positive propeller deck point"
        )
    _, ct, cp, eta, rpm, pitch, advance_ratio = min(candidates)
    return ct, cp, eta, rpm, pitch, advance_ratio


def best_takeoff_prop_row(
    altitude_m: float, speed_ms: float,
) -> tuple[float, float, float, float, float, float]:
    allowed_helical_tip_speed = (
        0.995 * TIP_MACH_LIMIT * isa_speed_of_sound(altitude_m)
    )
    if speed_ms >= allowed_helical_tip_speed:
        raise ValueError("Takeoff speed exceeds helical tip-speed limit")
    rotational_tip_speed = math.sqrt(
        allowed_helical_tip_speed**2 - speed_ms**2
    )
    rpm = 60.0 * rotational_tip_speed / (math.pi * DIAMETER_M)
    advance_ratio = speed_ms / ((rpm / 60.0) * DIAMETER_M)
    with PROP_CSV.open(encoding="utf-8-sig") as stream:
        pitches = sorted({float(row[0]) for row in csv.reader(stream)})
    candidates = []
    for pitch in pitches:
        try:
            ct, cp, eta = prop_row(pitch, advance_ratio)
        except ValueError:
            continue
        candidates.append(
            ((cp / ct) * (rpm / 60.0) * DIAMETER_M,
             ct, cp, eta, rpm, pitch, advance_ratio)
        )
    if not candidates:
        raise ValueError(f"No takeoff pitch slice covers J={advance_ratio}")
    _, ct, cp, eta, rpm, pitch, advance_ratio = min(candidates)
    return ct, cp, eta, rpm, pitch, advance_ratio


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
    ct, cp, eta, rpm, _, advance_ratio = best_airborne_prop_row(
        altitude_m, speed_ms
    )
    n = rpm / 60.0
    power_to_thrust = (cp / ct) * n * DIAMETER_M
    return thrust_to_weight * power_to_thrust, advance_ratio, eta, cl


def mission_climb_power_loading() -> tuple[float, int, int]:
    with MISSION_CSV.open(encoding="utf-8-sig") as stream:
        rows = list(csv.reader(stream, delimiter=";"))
    header = [cell.strip() for cell in rows[0]]
    data = rows[1:]
    time_index = header.index("Time [s]")
    altitude_index = header.index("Altitude [m]")
    mode_index = header.index("Mode name [-]")
    mass_index = header.index("Total mass [kg]")
    speed_index = header.index("TAS [m/s]")
    roc_index = header.index("ROC [fpm]")
    initial_mass = float(data[0][mass_index])

    worst_power_loading = -math.inf
    valid_points = 0
    invalid_points = 0
    for index, row in enumerate(data):
        mode = row[mode_index].strip()
        roc_ms = float(row[roc_index]) * 0.00508
        if mode in {"takeoff", "landing"} or roc_ms <= 0.0:
            continue

        lower = index - 1 if (
            index > 0 and data[index - 1][mode_index].strip() == mode
        ) else index
        upper = index + 1 if (
            index + 1 < len(data) and
            data[index + 1][mode_index].strip() == mode
        ) else index
        if lower == upper:
            continue
        time_delta = (
            float(data[upper][time_index]) -
            float(data[lower][time_index])
        )
        if time_delta <= 0.0:
            continue

        altitude_m = float(row[altitude_index])
        speed_ms = float(row[speed_index])
        beta = float(row[mass_index]) / initial_mass
        acceleration_ms2 = (
            float(data[upper][speed_index]) -
            float(data[lower][speed_index])
        ) / time_delta
        try:
            power_loading = airborne_power_loading(
                altitude_m,
                speed_ms,
                beta,
                roc_ms=roc_ms,
                acceleration_ms2=acceleration_ms2,
            )[0]
        except ValueError:
            invalid_points += 1
            continue

        valid_points += 1
        worst_power_loading = max(worst_power_loading, power_loading)

    if not math.isfinite(worst_power_loading):
        raise AssertionError("Mission climb has no deck-covered points")
    return worst_power_loading, valid_points, invalid_points


def mission_mode_power_loading(
    modes: set[str], acceleration_scan: bool,
) -> tuple[float, int, int]:
    """Independent mission scan for propeller cruise or acceleration."""
    with MISSION_CSV.open(encoding="utf-8-sig") as stream:
        rows = list(csv.reader(stream, delimiter=";"))
    header = [cell.strip() for cell in rows[0]]
    data = rows[1:]
    time_index = header.index("Time [s]")
    altitude_index = header.index("Altitude [m]")
    mode_index = header.index("Mode name [-]")
    mass_index = header.index("Total mass [kg]")
    speed_index = header.index("TAS [m/s]")
    roc_index = header.index("ROC [fpm]")
    initial_mass = float(data[0][mass_index])
    worst = -math.inf
    valid = 0
    invalid = 0

    for index, row in enumerate(data):
        mode = row[mode_index].strip()
        if mode not in modes:
            continue
        acceleration_ms2 = 0.0
        roc_ms = 0.0
        if acceleration_scan:
            lower = index - 1 if (
                index > 0 and data[index - 1][mode_index].strip() == mode
            ) else index
            upper = index + 1 if (
                index + 1 < len(data) and
                data[index + 1][mode_index].strip() == mode
            ) else index
            if lower == upper:
                continue
            dt = float(data[upper][time_index]) - float(data[lower][time_index])
            if dt <= 0.0:
                continue
            acceleration_ms2 = (
                float(data[upper][speed_index]) -
                float(data[lower][speed_index])
            ) / dt
            if acceleration_ms2 <= 1.0e-6:
                continue
            roc_ms = float(row[roc_index]) * 0.00508

        try:
            value = airborne_power_loading(
                float(row[altitude_index]),
                float(row[speed_index]),
                float(row[mass_index]) / initial_mass,
                roc_ms=roc_ms,
                acceleration_ms2=acceleration_ms2,
            )[0]
        except ValueError:
            invalid += 1
            continue
        valid += 1
        worst = max(worst, value)

    if not math.isfinite(worst):
        raise AssertionError("Mission mode has no deck-covered points")
    return worst, valid, invalid


def takeoff_distance(power_loading: float, beta: float) -> float:
    rho = isa_density(0.0)
    stall_speed = math.sqrt(2.0 * beta * WS / (rho * CLMAX_TO))
    takeoff_speed = 1.2 * stall_speed
    steps = 240
    dv = takeoff_speed / steps
    distance = 0.0
    for index in range(steps):
        speed = (index + 0.5) * dv
        ct, cp, _, rpm, _, _ = best_takeoff_prop_row(0.0, speed)
        n = rpm / 60.0
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


def takeoff_power_loading(beta: float, required_ground_roll_m: float) -> float:
    lower = 0.0
    upper = 1.0
    while takeoff_distance(upper, beta) > required_ground_roll_m:
        upper *= 2.0
    for _ in range(70):
        trial = 0.5 * (lower + upper)
        if takeoff_distance(trial, beta) > required_ground_roll_m:
            lower = trial
        else:
            upper = trial
    return upper


def main() -> None:
    beta = mission_betas()
    takeoff_ground_roll_m = case_ground_roll_requirement_m(
        "PROPELLER_UNICADO_BASELINE"
    )
    climb_power_loading, climb_valid, climb_invalid = (
        mission_climb_power_loading()
    )
    acceleration_power_loading, acceleration_valid, acceleration_invalid = (
        mission_mode_power_loading(
            {"accelerate", "change_speed_to_CAS", "change_speed_to_Mach"},
            acceleration_scan=True,
        )
    )
    try:
        cruise_power_loading, cruise_valid, cruise_invalid = (
            mission_mode_power_loading({"cruise"}, acceleration_scan=False)
        )
        cruise_mode = "mission_scan"
    except AssertionError:
        fallback_altitude_m, fallback_speed_ms = propeller_cruise_fallback()
        cruise_power_loading = airborne_power_loading(
            fallback_altitude_m, fallback_speed_ms, beta["cruise"]
        )[0]
        cruise_valid = 0
        cruise_invalid = 0
        cruise_mode = "explicit_configured_fallback"
    checks = {
        "propeller_takeoff_constraint": takeoff_power_loading(
            beta["takeoff"], takeoff_ground_roll_m
        ),
        "propeller_acceleration_constraint": acceleration_power_loading,
        "propeller_subsonic_cruise_constraint": cruise_power_loading,
        "propeller_subsonic_climb_constraint": climb_power_loading,
        "propeller_turn_constraint": airborne_power_loading(
            3000.0, 120.0, beta["cruise"], load_factor=2.5
        )[0],
    }

    print(f"Independent hand checks at W/S = {WS:.0f} N/m^2")
    print(
        "Mission climb deck coverage: "
        f"valid={climb_valid}, invalid={climb_invalid}"
    )
    print(
        "Mission acceleration deck coverage: "
        f"valid={acceleration_valid}, invalid={acceleration_invalid}"
    )
    print(
        "Mission cruise deck coverage: "
        f"valid={cruise_valid}, invalid={cruise_invalid}, mode={cruise_mode}"
    )
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
    if not (checks["propeller_subsonic_climb_constraint"] >
            checks["propeller_acceleration_constraint"] >
            checks["propeller_turn_constraint"] >
            checks["propeller_subsonic_cruise_constraint"]):
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
