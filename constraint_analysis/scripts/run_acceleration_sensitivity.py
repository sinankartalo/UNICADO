"""Run a controlled acceleration-requirement sensitivity study.

Only the horizontal-acceleration requirement is changed. All aerodynamic,
mission, propulsion, design-space, and remaining performance inputs are read
from the selected baseline configuration without modification.

The script creates temporary XML configurations, runs the existing C++
constraint-analysis executable, and collects the resulting design points in a
single CSV file. The C++ application remains the only source of sizing physics.
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


DEFAULT_ACCELERATIONS_MS2 = (0.0, 0.5, 1.0, 1.5)
BASELINE_CASES = (
    "JET_PW1127GJM_BASELINE",
    "PROPELLER_UNICADO_BASELINE",
)


def value_node(parent: ET.Element, path: str) -> ET.Element:
    node = parent.find(path)
    if node is None:
        raise RuntimeError(f"Missing XML input: {path}/value")
    return node


def case_tag(acceleration_ms2: float) -> str:
    text = f"{acceleration_ms2:g}".replace("-", "M").replace(".", "P")
    return f"ACCEL_{text}MS2"


def generated_case_id(baseline_case_id: str, acceleration_ms2: float) -> str:
    architecture = "JET" if baseline_case_id.startswith("JET_") else "PROPELLER"
    return f"{architecture}_SENSITIVITY_{case_tag(acceleration_ms2)}"


def write_study_config(
    source_config: Path,
    destination: Path,
    baseline_case_id: str,
    acceleration_ms2: float,
) -> str:
    tree = ET.parse(source_config)
    root = tree.getroot()

    active_value = value_node(
        root, "./control_settings/active_constraint_case_ID/value"
    )
    acceleration_value = value_node(
        root,
        "./program_settings/constraint_selection/standard_set/"
        "horizontal_acceleration/acceleration/value",
    )

    cases_parent = root.find("./program_settings/constraint_cases")
    if cases_parent is None:
        raise RuntimeError("Missing XML input: program_settings/constraint_cases")

    selected_case = None
    for candidate in list(cases_parent):
        if candidate.get("ID") == baseline_case_id:
            selected_case = candidate
        else:
            cases_parent.remove(candidate)

    if selected_case is None:
        raise RuntimeError(f"Baseline case not found: {baseline_case_id}")

    study_case_id = generated_case_id(baseline_case_id, acceleration_ms2)
    selected_case.set("ID", study_case_id)
    selected_case.set(
        "description",
        f"Acceleration sensitivity case at {acceleration_ms2:g} m/s^2",
    )
    active_value.text = study_case_id
    acceleration_value.text = f"{acceleration_ms2:.12g}"

    ET.indent(tree, space="  ")
    tree.write(destination, encoding="utf-8", xml_declaration=True)
    return study_case_id


def read_single_row(path: Path) -> dict[str, str]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != 1:
        raise RuntimeError(f"Expected one data row in {path}, found {len(rows)}")
    return rows[0]


def read_best_design_point(path: Path) -> dict[str, str]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    for row in rows:
        if row.get("method") == "best_design_point":
            return row
    raise RuntimeError(f"No best_design_point row found in {path}")


def curve_value_at(path: Path, wing_loading: float) -> float:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    points = sorted((float(row["x"]), float(row["y"])) for row in rows)
    if not points or wing_loading < points[0][0] or wing_loading > points[-1][0]:
        return -math.inf
    for index in range(len(points) - 1):
        x0, y0 = points[index]
        x1, y1 = points[index + 1]
        if x0 <= wing_loading <= x1:
            if x1 == x0:
                return y0
            fraction = (wing_loading - x0) / (x1 - x0)
            return y0 + fraction * (y1 - y0)
    return points[-1][1]


def governing_constraint(output_dir: Path, wing_loading: float) -> str:
    candidates: list[tuple[str, float]] = []
    for path in output_dir.glob("*_constraint.csv"):
        if path.name == "constraint_envelope.csv" or "range_fuel" in path.name:
            continue
        try:
            value = curve_value_at(path, wing_loading)
        except (KeyError, ValueError):
            continue
        if math.isfinite(value):
            name = path.stem
            name = name.removeprefix("jet_").removeprefix("propeller_")
            name = name.removesuffix("_constraint").replace("_", " ")
            candidates.append((name, value))
    if not candidates:
        raise RuntimeError(f"No propulsion constraint curves found in {output_dir}")
    return max(candidates, key=lambda item: item[1])[0]


def collect_result(
    output_dir: Path,
    baseline_case_id: str,
    study_case_id: str,
    acceleration_ms2: float,
) -> dict[str, str | float]:
    metadata = read_single_row(output_dir / "analysis_metadata.csv")
    design = read_best_design_point(output_dir / "design_point.csv")
    propulsion_type = metadata["propulsion_type"]
    value_column = (
        "thrust_to_weight" if propulsion_type == "jet"
        else "shaft_power_to_weight"
    )
    wing_loading = float(design["wing_loading"])
    return {
        "study_case_id": study_case_id,
        "baseline_case_id": baseline_case_id,
        "propulsion_type": propulsion_type,
        "acceleration_ms2": acceleration_ms2,
        "wing_loading_N_m2": wing_loading,
        "propulsion_loading": float(design[value_column]),
        "propulsion_loading_unit": "-" if propulsion_type == "jet" else "W/N",
        "wing_area_m2": float(design["wing_area_m2"]),
        "governing_constraint": governing_constraint(output_dir, wing_loading),
        "vertically_feasible": design["vertical_constraints_feasible"],
    }


def write_summary(path: Path, results: list[dict[str, str | float]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = (
        "study_case_id",
        "baseline_case_id",
        "propulsion_type",
        "acceleration_ms2",
        "wing_loading_N_m2",
        "propulsion_loading",
        "propulsion_loading_unit",
        "wing_area_m2",
        "governing_constraint",
        "vertically_feasible",
    )
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(results)


def write_plot(path: Path, results: list[dict[str, str | float]]) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError as error:
        raise RuntimeError(
            "matplotlib is required to create the sensitivity plot."
        ) from error

    path.parent.mkdir(parents=True, exist_ok=True)
    fig, axes = plt.subplots(1, 3, figsize=(16.0, 5.2))
    styles = {
        "jet": ("#0065BD", "o", "Jet"),
        "propeller": ("#A2AD00", "s", "Propeller"),
    }

    for propulsion_type, (color, marker, label) in styles.items():
        selected = sorted(
            (
                row for row in results
                if row["propulsion_type"] == propulsion_type
            ),
            key=lambda row: float(row["acceleration_ms2"]),
        )
        accelerations = [float(row["acceleration_ms2"]) for row in selected]
        wing_loadings = [float(row["wing_loading_N_m2"]) for row in selected]
        propulsion_loadings = [
            float(row["propulsion_loading"]) for row in selected
        ]

        axes[0].plot(
            accelerations,
            wing_loadings,
            color=color,
            marker=marker,
            linewidth=2.0,
            markersize=7,
            label=label,
        )
        loading_axis = axes[1] if propulsion_type == "jet" else axes[2]
        loading_axis.plot(
            accelerations,
            propulsion_loadings,
            color=color,
            marker=marker,
            linewidth=2.0,
            markersize=7,
            label=label,
        )

        for row, acceleration, loading in zip(
            selected, accelerations, propulsion_loadings
        ):
            governing = str(row["governing_constraint"]).title()
            loading_axis.annotate(
                governing,
                (acceleration, loading),
                xytext=(0, 8),
                textcoords="offset points",
                ha="center",
                fontsize=8,
                color=color,
            )

    axes[0].set_xlabel(r"Acceleration requirement, $\dot{V}$ [m/s$^2$]")
    axes[0].set_ylabel(r"Selected wing loading, $W/S$ [N/m$^2$]")
    axes[0].set_title("Wing-loading sensitivity")
    axes[1].set_xlabel(r"Acceleration requirement, $\dot{V}$ [m/s$^2$]")
    axes[1].set_ylabel(r"Required thrust loading, $T/W$ [-]")
    axes[1].set_title("Jet propulsion sensitivity")
    axes[2].set_xlabel(r"Acceleration requirement, $\dot{V}$ [m/s$^2$]")
    axes[2].set_ylabel(r"Required shaft-power loading, $P/W$ [W/N]")
    axes[2].set_title("Propeller propulsion sensitivity")

    for axis in axes:
        axis.grid(True, alpha=0.25)
        axis.legend(frameon=False)

    fig.suptitle(
        "Sensitivity of the minimum feasible design point to acceleration",
        fontsize=13,
    )
    fig.tight_layout()
    fig.savefig(path, dpi=220, bbox_inches="tight")
    plt.close(fig)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run jet and propeller acceleration sensitivity cases."
    )
    parser.add_argument(
        "--executable",
        type=Path,
        default=Path("build/constraint_analysis_app.exe"),
        help="Path to the compiled constraint-analysis executable.",
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=Path("config/constraint_analysis_conf.xml"),
        help="Baseline XML configuration.",
    )
    parser.add_argument(
        "--accelerations",
        type=float,
        nargs="+",
        default=list(DEFAULT_ACCELERATIONS_MS2),
        help="Acceleration requirements in m/s^2.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Generate and validate temporary XML files without running C++.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_arguments()
    working_dir = Path.cwd()
    source_config = args.config.resolve()
    executable = args.executable.resolve()

    if not source_config.is_file():
        raise RuntimeError(f"Configuration file not found: {source_config}")
    if not args.dry_run and not executable.is_file():
        raise RuntimeError(f"Executable not found: {executable}")
    if any(value < 0.0 for value in args.accelerations):
        raise RuntimeError("Acceleration requirements must be non-negative.")

    results: list[dict[str, str | float]] = []
    with tempfile.TemporaryDirectory(prefix="acceleration_sensitivity_") as temp:
        temp_dir = Path(temp)
        for baseline_case_id in BASELINE_CASES:
            for acceleration_ms2 in args.accelerations:
                temp_config = temp_dir / (
                    f"{baseline_case_id}_{case_tag(acceleration_ms2)}.xml"
                )
                study_case_id = write_study_config(
                    source_config,
                    temp_config,
                    baseline_case_id,
                    acceleration_ms2,
                )
                print(
                    f"Prepared {study_case_id}: "
                    f"acceleration={acceleration_ms2:g} m/s^2"
                )
                if args.dry_run:
                    continue

                subprocess.run(
                    [str(executable), str(temp_config)],
                    cwd=working_dir,
                    check=True,
                )
                output_dir = working_dir / "output" / study_case_id
                results.append(
                    collect_result(
                        output_dir,
                        baseline_case_id,
                        study_case_id,
                        acceleration_ms2,
                    )
                )

    if args.dry_run:
        print("Dry run completed; no solver or output files were created.")
        return 0

    summary_path = working_dir / "output" / "acceleration_sensitivity_summary.csv"
    write_summary(summary_path, results)
    plot_path = working_dir / "plots" / "acceleration_sensitivity.png"
    write_plot(plot_path, results)
    print(f"Sensitivity summary written to: {summary_path}")
    print(f"Sensitivity plot written to: {plot_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Sensitivity study failed: {error}", file=sys.stderr)
        raise SystemExit(1)
