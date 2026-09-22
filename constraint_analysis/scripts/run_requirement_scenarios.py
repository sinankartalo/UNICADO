"""Run requirement-profile cases for governing-constraint comparison.

The existing C++ executable remains the only source of sizing physics. This
script creates temporary XML configurations, runs both propulsion
architectures, calls the standard plotting script, and exports one traceable
summary CSV.
"""

from __future__ import annotations

import argparse
import csv
import math
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path


BASELINE_CASES = (
    "JET_PW1127GJM_BASELINE",
    "PROPELLER_UNICADO_BASELINE",
)

# These are performance-requirement profiles, not different aircraft models.
# All inputs not listed here remain identical to the baseline configuration.
SCENARIOS = {
    "BASELINE": {
        "takeoff_runway_m": 1200.0,
        "acceleration_ms2": 1.5,
        "climb_rate_ms": 5.0,
    },
    "SHORT_FIELD": {
        "takeoff_runway_m": 600.0,
        "acceleration_ms2": 0.5,
        "climb_rate_ms": 5.0,
    },
    "CLIMB_FOCUSED": {
        "takeoff_runway_m": 1200.0,
        "acceleration_ms2": 0.5,
        "climb_rate_ms": 18.0,
    },
}

XML_REQUIREMENT_PATHS = {
    "takeoff_runway_m": (
        "./program_settings/constraint_selection/standard_set/"
        "takeoff_ground_roll/takeoff_ground_roll_m/value"
    ),
    "acceleration_ms2": (
        "./program_settings/constraint_selection/standard_set/"
        "horizontal_acceleration/acceleration/value"
    ),
    "climb_rate_ms": (
        "./program_settings/constraint_selection/standard_set/"
        "climb/climb_rate/value"
    ),
}


def required_node(parent: ET.Element, path: str) -> ET.Element:
    node = parent.find(path)
    if node is None:
        raise RuntimeError(f"Missing XML input: {path}")
    return node


def architecture_name(baseline_case_id: str) -> str:
    return "JET" if baseline_case_id.startswith("JET_") else "PROPELLER"


def write_scenario_config(
    source_config: Path,
    destination: Path,
    baseline_case_id: str,
    scenario_name: str,
) -> str:
    tree = ET.parse(source_config)
    root = tree.getroot()

    scenario = SCENARIOS[scenario_name]
    for input_name, value in scenario.items():
        required_node(root, XML_REQUIREMENT_PATHS[input_name]).text = f"{value:g}"

    cases_parent = required_node(
        root, "./program_settings/constraint_cases"
    )
    selected_case = None
    for candidate in list(cases_parent):
        if candidate.get("ID") == baseline_case_id:
            selected_case = candidate
        else:
            cases_parent.remove(candidate)
    if selected_case is None:
        raise RuntimeError(f"Baseline case not found: {baseline_case_id}")

    case_id = f"{architecture_name(baseline_case_id)}_{scenario_name}"
    selected_case.set("ID", case_id)
    selected_case.set(
        "description",
        f"{scenario_name.replace('_', ' ').title()} requirement profile",
    )
    required_node(
        root, "./control_settings/active_constraint_case_ID/value"
    ).text = case_id

    ET.indent(tree, space="  ")
    tree.write(destination, encoding="utf-8", xml_declaration=True)
    return case_id


def read_best_design_point(path: Path) -> dict[str, str]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    for row in rows:
        if row.get("method") == "best_design_point":
            return row
    raise RuntimeError(f"No best_design_point row found in {path}")


def interpolated_curve_value(path: Path, wing_loading: float) -> float:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    points = sorted((float(row["x"]), float(row["y"])) for row in rows)
    if not points or wing_loading < points[0][0] or wing_loading > points[-1][0]:
        return -math.inf
    for index in range(len(points) - 1):
        x0, y0 = points[index]
        x1, y1 = points[index + 1]
        if x0 <= wing_loading <= x1:
            fraction = 0.0 if x1 == x0 else (wing_loading - x0) / (x1 - x0)
            return y0 + fraction * (y1 - y0)
    return points[-1][1]


def governing_constraint(output_dir: Path, wing_loading: float) -> str:
    candidates: list[tuple[str, float]] = []
    for path in output_dir.glob("*_constraint.csv"):
        if path.name == "constraint_envelope.csv" or "range_fuel" in path.name:
            continue
        try:
            value = interpolated_curve_value(path, wing_loading)
        except (KeyError, ValueError):
            continue
        if math.isfinite(value):
            label = path.stem
            label = label.removeprefix("jet_").removeprefix("propeller_")
            label = label.removesuffix("_constraint").replace("_", " ")
            candidates.append((label, value))
    if not candidates:
        raise RuntimeError(f"No constraint curves found in {output_dir}")
    return max(candidates, key=lambda item: item[1])[0]


def collect_result(
    output_dir: Path,
    scenario_name: str,
    case_id: str,
) -> dict[str, str | float]:
    design = read_best_design_point(output_dir / "design_point.csv")
    is_jet = case_id.startswith("JET_")
    value_column = "thrust_to_weight" if is_jet else "shaft_power_to_weight"
    wing_loading = float(design["wing_loading"])
    inputs = SCENARIOS[scenario_name]
    return {
        "case_id": case_id,
        "architecture": "jet" if is_jet else "propeller",
        "requirement_profile": scenario_name.lower(),
        "takeoff_runway_m": inputs["takeoff_runway_m"],
        "acceleration_ms2": inputs["acceleration_ms2"],
        "climb_rate_ms": inputs["climb_rate_ms"],
        "wing_loading_N_m2": wing_loading,
        "propulsion_loading": float(design[value_column]),
        "propulsion_loading_unit": "-" if is_jet else "W/N",
        "wing_area_m2": float(design["wing_area_m2"]),
        "governing_constraint": governing_constraint(output_dir, wing_loading),
        "vertically_feasible": design["vertical_constraints_feasible"],
    }


def write_summary(path: Path, results: list[dict[str, str | float]]) -> None:
    fields = (
        "case_id",
        "architecture",
        "requirement_profile",
        "takeoff_runway_m",
        "acceleration_ms2",
        "climb_rate_ms",
        "wing_loading_N_m2",
        "propulsion_loading",
        "propulsion_loading_unit",
        "wing_area_m2",
        "governing_constraint",
        "vertically_feasible",
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(results)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run baseline, short-field, and climb-focused cases."
    )
    parser.add_argument(
        "--executable",
        type=Path,
        default=Path("build/constraint_analysis_app.exe"),
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=Path("config/constraint_analysis_conf.xml"),
    )
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_arguments()
    working_dir = Path.cwd()
    source_config = args.config.resolve()
    executable = args.executable.resolve()
    plotting_script = (working_dir / "scripts" / "plot_results.py").resolve()

    if not source_config.is_file():
        raise RuntimeError(f"Configuration file not found: {source_config}")
    if not args.dry_run and not executable.is_file():
        raise RuntimeError(f"Executable not found: {executable}")
    if not plotting_script.is_file():
        raise RuntimeError(f"Plotting script not found: {plotting_script}")

    results: list[dict[str, str | float]] = []
    with tempfile.TemporaryDirectory(prefix="requirement_scenarios_") as temp:
        temp_dir = Path(temp)
        for baseline_case_id in BASELINE_CASES:
            for scenario_name in SCENARIOS:
                temp_config = temp_dir / f"{baseline_case_id}_{scenario_name}.xml"
                case_id = write_scenario_config(
                    source_config,
                    temp_config,
                    baseline_case_id,
                    scenario_name,
                )
                inputs = SCENARIOS[scenario_name]
                print(
                    f"Prepared {case_id}: takeoff={inputs['takeoff_runway_m']:g} m, "
                    f"acceleration={inputs['acceleration_ms2']:g} m/s^2, "
                    f"climb={inputs['climb_rate_ms']:g} m/s"
                )
                if args.dry_run:
                    continue

                subprocess.run(
                    [str(executable), str(temp_config)],
                    cwd=working_dir,
                    check=True,
                )
                subprocess.run(
                    [sys.executable, str(plotting_script), case_id],
                    cwd=working_dir,
                    check=True,
                )
                output_dir = working_dir / "output" / case_id
                results.append(collect_result(output_dir, scenario_name, case_id))

    if args.dry_run:
        print("Dry run completed; no solver or plot output was created.")
        return 0

    summary_path = working_dir / "output" / "requirement_scenario_summary.csv"
    write_summary(summary_path, results)
    print(f"Scenario summary written to: {summary_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Requirement-scenario study failed: {error}", file=sys.stderr)
        raise SystemExit(1)
