import os
import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from matplotlib.lines import Line2D
from matplotlib.patches import Patch


# ============================================================
# Case-specific paths
# ============================================================
output_root = "output"
plots_root = "plots"

if len(sys.argv) > 2:
    raise SystemExit("Usage: python scripts/plot_results.py [CASE_ID]")

selected_case_id = sys.argv[1] if len(sys.argv) == 2 else None
latest_case_path = os.path.join(output_root, "latest_case.txt")

if selected_case_id is None and os.path.exists(latest_case_path):
    with open(latest_case_path, encoding="utf-8") as latest_case_file:
        selected_case_id = latest_case_file.read().strip()

if selected_case_id:
    output_dir = os.path.join(output_root, selected_case_id)
    save_dir = os.path.join(plots_root, selected_case_id)
else:
    # Backward-compatible fallback for output generated before case folders.
    output_dir = output_root
    save_dir = plots_root

if not os.path.isdir(output_dir):
    raise FileNotFoundError(
        f"No analysis output found for case '{selected_case_id}': {output_dir}. "
        "Run the C++ application for that case before plotting."
    )

os.makedirs(save_dir, exist_ok=True)


# ============================================================
# Global plot style
# ============================================================
plt.rcParams.update({
    "figure.figsize": (11, 6.5),
    "font.size": 11,
    "axes.titlesize": 15,
    "axes.labelsize": 12,
    "legend.fontsize": 9,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "axes.grid": True,
    "axes.axisbelow": True,
    "axes.facecolor": "#fbfbfb",
    "figure.facecolor": "white",
    "grid.alpha": 0.28,
    "grid.linestyle": "--",
    "lines.linewidth": 1.8,
    "savefig.dpi": 300,
    "savefig.facecolor": "white",
})


# ============================================================
# Helper functions
# ============================================================
def load_xy_csv(filename):
    path = os.path.join(output_dir, filename)

    if not os.path.exists(path):
        print(f"Warning: missing file: {path}")
        return None

    df = pd.read_csv(path)

    if "x" in df.columns and "y" in df.columns:
        df = df.rename(columns={"x": "wing_loading", "y": "thrust_to_weight"})

    required = ["wing_loading", "thrust_to_weight"]

    for col in required:
        if col not in df.columns:
            raise ValueError(f"{filename} does not contain column: {col}")

    return df[required].dropna().sort_values("wing_loading")


plot_allowlist = None


def save_plot(name, tight=True):

    if plot_allowlist is not None and name not in plot_allowlist:
        plt.close()
        return

    png_path = os.path.join(save_dir, f"{name}.png")

    if tight:
        plt.tight_layout()

    plt.savefig(
        png_path,
        dpi=300,
        bbox_inches="tight"
    )

    plt.close()

    print(f"Saved: {png_path}")


def clean_axes(ax):
    """Apply one restrained visual language to every engineering plot."""
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.grid(axis="x", alpha=0.16)
    ax.grid(axis="y", alpha=0.24)


# Output-space tolerance assigned to each matching-chart constraint. The two
# values are the fractional distances below and above the nominal boundary.
# Keeping them separate allows asymmetric uncertainty intervals later without
# changing the plotting implementation.
constraint_tolerances = {
    "Acceleration": (0.10, 0.10),
    "Climb": (0.10, 0.10),
    "Cruise": (0.10, 0.10),
    "Max Mach": (0.10, 0.10),
    "Supercruise": (0.10, 0.10),
    "Takeoff": (0.10, 0.10),
    "Turn": (0.10, 0.10),
    "Landing": (0.10, 0.10),
    "Stall speed": (0.10, 0.10),
    "Gust": (0.10, 0.10),
}


def add_gradient_curve_band(
        ax, x, nominal_y, color, lower_fraction, upper_fraction,
        layers=14, draw_edges=True):
    """Shade a curve tolerance band darkest at its nominal centreline."""
    x = np.asarray(x, dtype=float)
    nominal_y = np.asarray(nominal_y, dtype=float)

    # The widest layer reaches the tolerance limits and is seen only at the
    # edges. Progressively narrower translucent layers overlap near the
    # nominal curve, creating a centre-dark/edge-light uncertainty band.
    for scale in np.linspace(1.0, 1.0 / layers, layers):
        lower_y = nominal_y * (1.0 - lower_fraction * scale)
        upper_y = nominal_y * (1.0 + upper_fraction * scale)
        ax.fill_between(
            x,
            lower_y,
            upper_y,
            color=color,
            alpha=0.075,
            linewidth=0.0,
            zorder=1,
        )
    if draw_edges:
        ax.plot(
            x, nominal_y * (1.0 - lower_fraction),
            color=color, linewidth=1.05, alpha=0.82, zorder=2,
        )
        ax.plot(
            x, nominal_y * (1.0 + upper_fraction),
            color=color, linewidth=1.05, alpha=0.82, zorder=2,
        )


def add_gradient_vertical_band(
        ax, nominal_x, color, lower_fraction, upper_fraction, layers=14,
        draw_edges=True):
    """Shade a vertical-limit tolerance band darkest at its nominal value."""
    for scale in np.linspace(1.0, 1.0 / layers, layers):
        lower_x = nominal_x * (1.0 - lower_fraction * scale)
        upper_x = nominal_x * (1.0 + upper_fraction * scale)
        ax.axvspan(
            lower_x,
            upper_x,
            color=color,
            alpha=0.075,
            linewidth=0.0,
            zorder=1,
        )
    if draw_edges:
        ax.axvline(
            nominal_x * (1.0 - lower_fraction),
            color=color, linewidth=1.05, alpha=0.82, zorder=2,
        )
        ax.axvline(
            nominal_x * (1.0 + upper_fraction),
            color=color, linewidth=1.05, alpha=0.82, zorder=2,
        )


def add_curve_family_region(
        ax, family, parameter_column, parameter_values, color, label):
    """Convert a parameter curve family into a centre-dark gradient area."""
    curves = []
    reference_x = None
    for value in parameter_values:
        curve = family[np.isclose(
            family[parameter_column], value,
            rtol=1.0e-9, atol=1.0e-12,
        )].sort_values("wing_loading")
        x = curve["wing_loading"].to_numpy(dtype=float)
        y = curve["thrust_to_weight"].to_numpy(dtype=float)
        if reference_x is None:
            reference_x = x
        elif len(x) != len(reference_x) or not np.allclose(x, reference_x):
            raise RuntimeError(
                f"{label} sensitivity curves do not share one W/S grid."
            )
        curves.append(y)

    curve_matrix = np.asarray(curves)
    for inward_index in range(len(parameter_values) // 2):
        lower = np.minimum(
            curve_matrix[inward_index], curve_matrix[-1 - inward_index]
        )
        upper = np.maximum(
            curve_matrix[inward_index], curve_matrix[-1 - inward_index]
        )
        ax.fill_between(
            reference_x, lower, upper, color=color,
            alpha=0.13, linewidth=0.0, zorder=1,
        )

    outer_lower = np.min(curve_matrix, axis=0)
    outer_upper = np.max(curve_matrix, axis=0)
    ax.plot(reference_x, outer_lower, color=color, linewidth=1.25,
            alpha=0.9, zorder=2)
    ax.plot(reference_x, outer_upper, color=color, linewidth=1.25,
            alpha=0.9, zorder=2)
    return Patch(
        facecolor=color, edgecolor=color, linewidth=1.0, alpha=0.42,
        label=label,
    )


def set_feasible_sensitivity_xlim(ax, family, lower_limit, upper_limit):
    """Frame a sensitivity family around the physically feasible W/S window."""
    data_min = float(family["wing_loading"].min())
    data_max = float(family["wing_loading"].max())
    left = max(data_min, float(lower_limit))
    right = min(data_max, float(upper_limit))
    if not np.isfinite(left) or not np.isfinite(right) or left >= right:
        left, right = data_min, data_max
    padding = 0.035 * max(right - left, 1.0)
    ax.set_xlim(max(data_min, left - padding), min(data_max, right + padding))


def add_vertical_family_region(ax, values, color, label):
    """Draw a centre-dark region for a family of vertical limits."""
    values = np.sort(np.asarray(values, dtype=float))
    for inward_index in range(len(values) // 2):
        ax.axvspan(
            values[inward_index], values[-1 - inward_index],
            color=color, alpha=0.13, linewidth=0.0, zorder=1,
        )
    ax.axvline(values[0], color=color, linewidth=1.25, alpha=0.9, zorder=2)
    ax.axvline(values[-1], color=color, linewidth=1.25, alpha=0.9, zorder=2)
    return Patch(
        facecolor=color, edgecolor=color, linewidth=1.0, alpha=0.42,
        label=label,
    )


def add_design_point(ax, x, y, label, annotation, offset=(-22, -62)):
    point = ax.scatter(
        x, y, marker="*", s=165, color="#007f5f", edgecolor="white",
        linewidth=1.2, label=label, zorder=9,
    )
    ax.annotate(
        annotation, xy=(x, y), xytext=offset, textcoords="offset points",
        ha="right" if offset[0] < 0 else "left", fontsize=9.5,
        arrowprops=dict(arrowstyle="-", color="#4d4d4d", linewidth=1.0),
        bbox=dict(boxstyle="round,pad=0.35", facecolor="white",
                  edgecolor="#b3b3b3", alpha=0.96),
        zorder=10,
    )
    return point


# ============================================================
# Load constraint data
# ============================================================
metadata_path = os.path.join(output_dir, "analysis_metadata.csv")
if os.path.exists(metadata_path):
    metadata = pd.read_csv(metadata_path)
    case_id = str(metadata.iloc[0]["case_id"])
    if selected_case_id and case_id != selected_case_id:
        raise RuntimeError(
            f"Selected case '{selected_case_id}' does not match metadata "
            f"case '{case_id}' in {metadata_path}."
        )
    propeller_mode = metadata.iloc[0]["propulsion_type"] == "propeller"
else:
    propeller_mode = os.path.exists(
        os.path.join(output_dir, "propeller_takeoff_constraint.csv")
    )
    case_id = "PROPELLER_UNICADO_BASELINE" if propeller_mode else "JET_CASE"

case_labels = {
    "JET_V2527A5_BASELINE": "Jet — V2527-A5 Test Engine: Baseline Case",
    "JET_PW1127GJM_BASELINE": "Jet — PW1127G-JM Real Engine: Baseline Case",
    "JET_PW1127GJM_SHORT_FIELD": "Jet — PW1127G-JM Real Engine: Short-Field Case",
    "JET_PW1127GJM_LONG_FIELD": "Jet — PW1127G-JM Real Engine: Long-Field Case",
    "PROPELLER_UNICADO_BASELINE": "Propeller — UNICADO: Baseline Case",
}
analysis_label = case_labels.get(
    case_id,
    f"{'Propeller' if propeller_mode else 'Jet'} — {case_id}",
)

propeller_climb_coverage_note = None
if propeller_mode:
    coverage_path = os.path.join(
        output_dir, "propeller_climb_mission_coverage.csv"
    )
    if os.path.exists(coverage_path):
        coverage = pd.read_csv(coverage_path).iloc[0]
        if coverage["coverage_status"] == "partial_mission_coverage":
            propeller_climb_coverage_note = (
                "Climb constraint uses partial mission coverage: "
                f"{int(coverage['valid_deck_points'])}/"
                f"{int(coverage['total_mission_points'])} points "
                "inside supplied propeller deck"
            )

# Keep the deliverable aligned with the lecture workflow.  The existing
# matching chart and active-constraint figure remain part of the output; the
# added work is limited to sensitivity plots and classical carpet studies.
if propeller_mode:
    plot_allowlist = {
        "01_matching_chart_professional",
        "01_matching_chart_with_tolerance_bands",
        "02_active_constraint_regions",
        "03_constraint_utilization_dashboard",
        "04_governing_constraint_gap_map",
        "03_propeller_performance_map",
        "06_classical_carpet_plot",
    }
else:
    plot_allowlist = {
        "01_matching_chart_professional",
        "01_matching_chart_with_tolerance_bands",
        "02_active_constraint_regions",
        "03_constraint_utilization_dashboard",
        "04_governing_constraint_gap_map",
        "05_cd0_k_design_map",
        "06_mission_runway_design_map",
    }
for existing_plot in os.listdir(save_dir):
    if not existing_plot.endswith(".png"):
        continue
    if os.path.splitext(existing_plot)[0] not in plot_allowlist:
        os.remove(os.path.join(save_dir, existing_plot))


def analysis_title(title):
    return f"{analysis_label}: {title}"


def readable_constraint_name(raw_name):
    return str(raw_name).replace("jet_", "").replace(
        "_constraint", ""
    ).replace("_", " ").title()


def draw_design_map(
        ax, carpet, parameter_a, parameter_b,
        parameter_a_label, parameter_b_label):
    """Draw an engineering design map without connecting optimum points."""
    values_a = np.sort(carpet[parameter_a].unique())
    values_b = np.sort(carpet[parameter_b].unique())
    ws_grid = carpet.pivot(
        index=parameter_b, columns=parameter_a,
        values="best_wing_loading",
    ).loc[values_b, values_a].to_numpy()
    tw_grid = carpet.pivot(
        index=parameter_b, columns=parameter_a,
        values="best_thrust_to_weight",
    ).loc[values_b, values_a].to_numpy()

    active_names = sorted(carpet["active_constraint_name"].astype(str).unique())
    active_index = {name: index for index, name in enumerate(active_names)}
    active_grid = carpet.assign(
        _active_index=carpet["active_constraint_name"].astype(str).map(
            active_index
        )
    ).pivot(
        index=parameter_b, columns=parameter_a, values="_active_index"
    ).loc[values_b, values_a].to_numpy()
    active_colors = [
        color_map.get(readable_constraint_name(name), "#94a3b8")
        for name in active_names
    ]
    ax.pcolormesh(
        values_a, values_b, active_grid,
        shading="nearest",
        cmap=mpl.colors.ListedColormap(active_colors),
        vmin=-0.5, vmax=max(len(active_names) - 0.5, 0.5),
        alpha=0.16, zorder=0,
    )

    ws_contours = ax.contour(
        values_a, values_b, ws_grid, levels=6,
        colors="#2563eb", linewidths=1.8, zorder=3,
    )
    tw_contours = ax.contour(
        values_a, values_b, tw_grid, levels=6,
        colors="#dc2626", linewidths=1.7, linestyles="--", zorder=3,
    )
    ax.clabel(ws_contours, inline=True, fontsize=8.0, fmt="W/S %.0f")
    ax.clabel(tw_contours, inline=True, fontsize=8.0, fmt="T/W %.3f")

    baseline = carpet[carpet["is_baseline"] == 1]
    if len(baseline) != 1:
        raise RuntimeError("Design map must contain exactly one nominal point.")
    baseline_row = baseline.iloc[0]
    ax.scatter(
        baseline_row[parameter_a], baseline_row[parameter_b],
        marker="*", s=190, color="#fbbf24", edgecolor="#0f172a",
        linewidth=0.95, zorder=8,
    )
    ax.annotate(
        "Nominal case\n"
        f"W/S = {float(baseline_row['best_wing_loading']):.0f} N/m²\n"
        f"T/W = {float(baseline_row['best_thrust_to_weight']):.3f}\n"
        f"Active: {readable_constraint_name(baseline_row['active_constraint_name'])}",
        (baseline_row[parameter_a], baseline_row[parameter_b]),
        xytext=(12, 15), textcoords="offset points", fontsize=8.3,
        color="#334155",
        bbox=dict(boxstyle="round,pad=0.28", facecolor="white",
                  edgecolor="#cbd5e1", alpha=0.95),
        arrowprops=dict(arrowstyle="->", color="#64748b", linewidth=1.0),
    )

    handles = [
        Line2D([0], [0], color="#2563eb", linewidth=1.8,
               label="Optimum W/S contours"),
        Line2D([0], [0], color="#dc2626", linewidth=1.7, linestyle="--",
               label="Required T/W contours"),
        Line2D([0], [0], marker="*", color="none",
               markerfacecolor="#fbbf24", markeredgecolor="#0f172a",
               markersize=11, label="Nominal case"),
    ]
    handles.extend(
        Patch(
            facecolor=color, edgecolor="none", alpha=0.22,
            label=f"Active region: {readable_constraint_name(name)}",
        )
        for name, color in zip(active_names, active_colors)
    )
    ax.set_xlabel(parameter_a_label)
    ax.set_ylabel(parameter_b_label)
    if parameter_a.endswith("_scale"):
        ax.xaxis.set_major_formatter(mpl.ticker.PercentFormatter(xmax=1.0))
    if parameter_b.endswith("_scale"):
        ax.yaxis.set_major_formatter(mpl.ticker.PercentFormatter(xmax=1.0))
    clean_axes(ax)
    return handles


def plot_two_parameter_classical_carpet(
        carpet, parameter_a, parameter_b, parameter_a_label,
        parameter_b_label, parameter_a_formatter, parameter_b_formatter,
        title, plot_name):
    required = {
        parameter_a, parameter_b, "best_wing_loading",
        "best_thrust_to_weight", "is_baseline",
        "active_constraint_name", "second_constraint_name",
        "constraint_margin",
    }
    if not required.issubset(carpet.columns):
        raise RuntimeError(
            f"{plot_name} CSV schema is incomplete; rerun the C++ application."
        )
    values_a = np.sort(carpet[parameter_a].unique())
    values_b = np.sort(carpet[parameter_b].unique())
    if len(values_a) != 9 or len(values_b) != 9 or len(carpet) != 81:
        raise RuntimeError(f"{plot_name} must contain a complete 9x9 grid.")

    fig, ax = plt.subplots(figsize=(10.0, 7.0))
    color_a = "#10b981"
    color_b = "#ef4444"
    label_indices = {0, 4, 8}

    # First family: parameter A is constant while parameter B varies.
    for index, value_a in enumerate(values_a):
        family = carpet[np.isclose(
            carpet[parameter_a], value_a,
            rtol=1.0e-9, atol=1.0e-12,
        )].sort_values(parameter_b)
        ax.plot(
            family["best_wing_loading"],
            family["best_thrust_to_weight"],
            color=color_a,
            linewidth=2.4 if index in label_indices else 1.25,
            alpha=1.0 if index in label_indices else 0.42,
        )
        if index in label_indices:
            row = family.iloc[index]
            ax.annotate(
                parameter_a_formatter(value_a),
                (row["best_wing_loading"], row["best_thrust_to_weight"]),
                xytext=(7, 5), textcoords="offset points",
                fontsize=8.2, color="#047857",
            )

    # Second family: parameter B is constant while parameter A varies.
    for index, value_b in enumerate(values_b):
        family = carpet[np.isclose(
            carpet[parameter_b], value_b,
            rtol=1.0e-9, atol=1.0e-12,
        )].sort_values(parameter_a)
        ax.plot(
            family["best_wing_loading"],
            family["best_thrust_to_weight"],
            color=color_b,
            linewidth=2.2 if index in label_indices else 1.2,
            alpha=1.0 if index in label_indices else 0.38,
            marker="o", markersize=4.0,
            markerfacecolor="#0f172a", markeredgecolor="#0f172a",
        )
        if index in label_indices:
            row = family.iloc[index]
            ax.annotate(
                parameter_b_formatter(value_b),
                (row["best_wing_loading"], row["best_thrust_to_weight"]),
                xytext=(7, -7), textcoords="offset points",
                fontsize=8.2, color="#b91c1c",
            )

    baseline = carpet[carpet["is_baseline"] == 1]
    if len(baseline) != 1:
        raise RuntimeError(f"{plot_name} must contain one nominal point.")
    baseline_row = baseline.iloc[0]

    def readable_constraint_name(raw_name):
        return str(raw_name).replace("jet_", "").replace(
            "_constraint", ""
        ).replace("_", " ").title()

    active_names = sorted(carpet["active_constraint_name"].astype(str).unique())
    active_handles = []
    for active_name in active_names:
        active_label = active_name.replace("jet_", "").replace(
            "_constraint", ""
        ).replace("_", " ").title()
        active_points = carpet[
            carpet["active_constraint_name"].astype(str) == active_name
        ]
        active_color = color_map.get(active_label, "#475569")
        marker = ax.scatter(
            active_points["best_wing_loading"],
            active_points["best_thrust_to_weight"],
            s=34, color=active_color, edgecolor="white", linewidth=0.45,
            alpha=0.92, zorder=7,
            label=f"Active: {active_label}",
        )
        active_handles.append(marker)

    ax.scatter(
        baseline_row["best_wing_loading"],
        baseline_row["best_thrust_to_weight"],
        marker="*", s=180, color="#fbbf24", edgecolor="#0f172a",
        linewidth=0.9, zorder=9,
    )
    ax.annotate(
        "Nominal inputs\n"
        f"Control: {readable_constraint_name(baseline_row['active_constraint_name'])} / "
        f"{readable_constraint_name(baseline_row['second_constraint_name'])}\n"
        f"Margin: {float(baseline_row['constraint_margin']):.3g}",
        (baseline_row["best_wing_loading"],
         baseline_row["best_thrust_to_weight"]),
        xytext=(0.98, 0.13), textcoords="axes fraction",
        ha="right", va="bottom", fontsize=8.3,
        color="#334155",
        arrowprops=dict(arrowstyle="->", color="#64748b", linewidth=1.0),
        bbox=dict(boxstyle="round,pad=0.25", facecolor="white",
                  edgecolor="#cbd5e1", alpha=0.92),
    )

    handles = [
        Line2D([0], [0], color=color_a, linewidth=2.4,
               label=f"Constant {parameter_a_label}"),
        Line2D([0], [0], color=color_b, linewidth=2.2, marker="o",
               markerfacecolor="#0f172a", markeredgecolor="#0f172a",
               label=f"Constant {parameter_b_label}"),
        Line2D([0], [0], color="#fbbf24", marker="*",
               markeredgecolor="#0f172a", linestyle="None",
               markersize=11, label="Nominal inputs"),
    ]
    ax.clear()
    handles = draw_design_map(
        ax, carpet, parameter_a, parameter_b,
        parameter_a_label, parameter_b_label,
    )
    ax.set_title(analysis_title(title))
    ax.legend(handles=handles, frameon=False, loc="best")
    ax.text(
        0.01, 0.02,
        "Pastel areas show the governing constraint; solid blue contours "
        "show optimum W/S and dashed red contours show required T/W.",
        transform=ax.transAxes, fontsize=8.8, color="#4d4d4d",
        va="bottom",
    )
    save_plot(plot_name)

if propeller_mode:
    constraint_files = {
        "Acceleration": "propeller_acceleration_constraint.csv",
        "Takeoff": "propeller_takeoff_constraint.csv",
        "Turn": "propeller_turn_constraint.csv",
    }
    y_axis_label = "Required Shaft Power Loading, P/W [W/N]"
    y_symbol = "P/W"
    design_value_column = "shaft_power_to_weight"
    for stale_name in (
        "03_cd0_carpet_plot.png",
        "04_optimum_tw_vs_cd0.png",
        "05_optimum_ws_vs_cd0.png",
        "06_range_fuel_fraction_and_ld.png",
        "07_constraint_envelope_carpet_plot.png",
    ):
        stale_path = os.path.join(save_dir, stale_name)
        if os.path.exists(stale_path):
            os.remove(stale_path)
else:
    constraint_files = {
        "Acceleration": "jet_acceleration_constraint.csv",
        "Max Mach": "jet_max_mach_constraint.csv",
        "Takeoff": "jet_takeoff_constraint.csv",
        "Turn": "jet_turn_constraint.csv",
    }
    y_axis_label = "Required Thrust-to-Weight Ratio, T/W [-]"
    y_symbol = "T/W"
    design_value_column = "thrust_to_weight"
    stale_duplicate = os.path.join(
        save_dir, "07_constraint_envelope_carpet_plot.png"
    )
    if os.path.exists(stale_duplicate):
        os.remove(stale_duplicate)

propulsion_prefix = "propeller" if propeller_mode else "jet"
for regime in ("subsonic", "transonic", "supersonic"):
    for segment in ("climb", "cruise"):
        filename = (
            f"{propulsion_prefix}_{regime}_{segment}_constraint.csv"
        )
        if os.path.exists(os.path.join(output_dir, filename)):
            label = f"{regime.title()} {segment.title()}"
            constraint_files[label] = filename

for required_segment in ("Climb", "Cruise"):
    if not any(name.endswith(required_segment) for name in constraint_files):
        raise RuntimeError(
            f"No mission-supported {required_segment.lower()} constraint "
            "curve was generated. Inspect mission_mach_regime_coverage.csv."
        )

for name in constraint_files:
    if name.endswith("Climb") or name.endswith("Cruise"):
        constraint_tolerances.setdefault(name, (0.10, 0.10))

constraints = {}
missing_constraint_files = []

for name, filename in constraint_files.items():
    df = load_xy_csv(filename)
    if df is not None:
        constraints[name] = df
    else:
        missing_constraint_files.append(filename)

if missing_constraint_files:
    missing_list = ", ".join(missing_constraint_files)
    raise RuntimeError(
        "The current analysis did not produce its constraint CSV files. "
        "Do not plot stale results; inspect the C++ application exit code. "
        f"Missing: {missing_list}"
    )

envelope = load_xy_csv("constraint_envelope.csv")

if envelope is None:
    raise FileNotFoundError("constraint_envelope.csv not found.")


# Read both the existing aircraft point and the minimum feasible design point.
# Older output folders with a single row remain supported.
design_point_path = os.path.join(output_dir, "design_point.csv")

if os.path.exists(design_point_path):
    design_point_df = pd.read_csv(design_point_path)
    if design_value_column not in design_point_df.columns:
        raise RuntimeError(
            f"{design_point_path} belongs to a different propulsion mode: "
            f"missing column '{design_value_column}'. Clean output and rerun "
            "the C++ application before plotting."
        )

    if "method" in design_point_df.columns:
        aircraft_rows = design_point_df[
            design_point_df["method"] == "aerodynamics_reference_area"
        ]
        best_rows = design_point_df[
            design_point_df["method"] == "best_design_point"
        ]
    else:
        aircraft_rows = design_point_df.iloc[[0]]
        best_rows = pd.DataFrame()

    if aircraft_rows.empty:
        aircraft_rows = design_point_df.iloc[[0]]

    aircraft_ws = float(aircraft_rows.iloc[0]["wing_loading"])
    aircraft_tw = float(aircraft_rows.iloc[0][design_value_column])
    aircraft_power_MW = None
    if (propeller_mode and
            "required_total_shaft_power_W" in aircraft_rows.columns):
        aircraft_power_MW = float(
            aircraft_rows.iloc[0]["required_total_shaft_power_W"]
        ) / 1.0e6

    if not best_rows.empty:
        best_ws = float(best_rows.iloc[0]["wing_loading"])
        best_tw = float(best_rows.iloc[0][design_value_column])
        best_area_m2 = None
        if "wing_area_m2" in best_rows.columns:
            best_area_m2 = float(best_rows.iloc[0]["wing_area_m2"])
        best_power_MW = None
        if (propeller_mode and
                "required_total_shaft_power_W" in best_rows.columns):
            best_power_MW = float(
                best_rows.iloc[0]["required_total_shaft_power_W"]
            ) / 1.0e6
    else:
        idx = envelope["thrust_to_weight"].idxmin()
        best_ws = float(envelope.loc[idx, "wing_loading"])
        best_tw = float(envelope.loc[idx, "thrust_to_weight"])
        best_power_MW = None
        best_area_m2 = None
else:
    idx = envelope["thrust_to_weight"].idxmin()
    best_ws = float(envelope.loc[idx, "wing_loading"])
    best_tw = float(envelope.loc[idx, "thrust_to_weight"])
    aircraft_ws = best_ws
    aircraft_tw = best_tw
    aircraft_power_MW = None
    best_power_MW = None
    best_area_m2 = None

takeoff_weight_N = None
if best_power_MW is not None and best_tw > 0.0:
    takeoff_weight_N = best_power_MW * 1.0e6 / best_tw

aircraft_annotation = (
    f"Aircraft\nW/S = {aircraft_ws:.0f} N/m²\n{y_symbol} = {aircraft_tw:.3f}"
)
best_annotation = (
    f"Best design point\nW/S = {best_ws:.0f} N/m²\n{y_symbol} = {best_tw:.3f}"
)
if aircraft_power_MW is not None:
    aircraft_annotation += f"\nP required = {aircraft_power_MW:.2f} MW"
if best_power_MW is not None:
    best_annotation += f"\nP required = {best_power_MW:.2f} MW"


def read_vertical_limit(filename):
    path = os.path.join(output_dir, filename)

    if not os.path.exists(path):
        return None

    df = pd.read_csv(path)

    if "wing_loading" in df.columns:
        return float(df["wing_loading"].iloc[0])
    if "W/S_limit" in df.columns:
        return float(df["W/S_limit"].iloc[0])
    if "x" in df.columns:
        return float(df["x"].iloc[0])

    return float(df.iloc[0, 0])


# Vertical limits
vertical_prefix = "propeller" if propeller_mode else "jet"
landing_ws_limit = read_vertical_limit(f"{vertical_prefix}_landing_limit.csv")
stall_ws_limit = read_vertical_limit(f"{vertical_prefix}_stall_speed_limit.csv")
gust_ws_limit = read_vertical_limit(f"{vertical_prefix}_gust_limit.csv")


# Axis limits. Build the visible domain from every finite curve point, every
# vertical constraint, and both marked design points. This is deliberately a
# union: a matching chart must never hide valid analysis data merely to obtain
# a visually tighter crop.
vertical_limits = [
    value for value in (landing_ws_limit, stall_ws_limit, gust_ws_limit)
    if value is not None and np.isfinite(value)
]
curve_x_arrays = [
    envelope["wing_loading"].to_numpy(dtype=float),
    *[
        df["wing_loading"].to_numpy(dtype=float)
        for df in constraints.values()
    ],
]
curve_x_values = np.concatenate(curve_x_arrays)
curve_x_values = curve_x_values[np.isfinite(curve_x_values)]
if curve_x_values.size == 0:
    raise RuntimeError("Matching chart contains no finite wing-loading data.")

nominal_extent_values = np.concatenate((
    curve_x_values,
    np.asarray(vertical_limits, dtype=float),
    np.asarray([aircraft_ws, best_ws], dtype=float),
))
nominal_extent_values = nominal_extent_values[
    np.isfinite(nominal_extent_values)
]
nominal_extent_min = float(np.min(nominal_extent_values))
nominal_extent_max = float(np.max(nominal_extent_values))
nominal_span = max(nominal_extent_max - nominal_extent_min, 1.0)
nominal_padding = 0.04 * nominal_span
x_min = max(0.0, nominal_extent_min - nominal_padding)
x_max = nominal_extent_max + nominal_padding

tolerance_vertical_limits = []
for name, value in (
    ("Landing", landing_ws_limit),
    ("Stall speed", stall_ws_limit),
    ("Gust", gust_ws_limit),
):
    if value is None or not np.isfinite(value):
        continue
    lower_fraction, upper_fraction = constraint_tolerances[name]
    tolerance_vertical_limits.extend((
        value * (1.0 - lower_fraction),
        value * (1.0 + upper_fraction),
    ))

tolerance_extent_values = np.concatenate((
    nominal_extent_values,
    np.asarray(tolerance_vertical_limits, dtype=float),
))
tolerance_extent_min = float(np.min(tolerance_extent_values))
tolerance_extent_max = float(np.max(tolerance_extent_values))
tolerance_span = max(tolerance_extent_max - tolerance_extent_min, 1.0)
tolerance_padding = 0.04 * tolerance_span
tolerance_x_min = max(0.0, tolerance_extent_min - tolerance_padding)
tolerance_x_max = tolerance_extent_max + tolerance_padding

y_min = 0.0
y_max = max(
    envelope["thrust_to_weight"].max(),
    max(df["thrust_to_weight"].max() for df in constraints.values())
)
y_max *= 1.15

tolerance_y_max = max(
    envelope["thrust_to_weight"].max(),
    max(
        df["thrust_to_weight"].max()
        * (1.0 + constraint_tolerances[name][1])
        for name, df in constraints.items()
    )
)
tolerance_y_max *= 1.15


# ============================================================
# Plot 1: Professional constraint envelope
# ============================================================
fig, ax = plt.subplots(figsize=(13.5, 6.8))

color_map = {
    "Acceleration": "#1f77b4",
    "Takeoff": "#d62728",
    "Landing": "#9467bd",
    "Stall speed": "#bcbd22",
    "Gust": "#7f7f7f",
    "Climb": "#2ca02c",
    "Cruise": "#ff7f0e",
    "Max Mach": "#8c564b",
    "Supercruise": "#17becf",
    "Turn": "#e377c2",
    "Range": "#7f7f7f",
}
color_map.update({
    "Subsonic Climb": "#2ca02c",
    "Subsonic Cruise": "#ff7f0e",
    "Transonic Climb": "#9467bd",
    "Transonic Cruise": "#17becf",
    "Supersonic Climb": "#006d2c",
    "Supersonic Cruise": "#d95f02",
})

for name, df in constraints.items():
    ax.plot(
        df["wing_loading"],
        df["thrust_to_weight"],
        label=name,
        color=color_map.get(name, None),
        alpha=0.68,
        linewidth=1.5,
        zorder=2,
    )

# Wing loading must remain between the lower and upper vertical limits.
# For a required-power chart, only the area above the envelope is feasible.
upper_ws_limits = [
    value for value in (landing_ws_limit, stall_ws_limit)
    if value is not None
]
feasible_ws_min = gust_ws_limit if gust_ws_limit is not None else x_min
feasible_ws_max = min(upper_ws_limits) if upper_ws_limits else x_max


def centered_feasible_xlim(required_left, required_right):
    """Show all required data while centering feasibility when possible."""
    if not np.isfinite(feasible_ws_min) or not np.isfinite(feasible_ws_max):
        return required_left, required_right
    if feasible_ws_min >= feasible_ws_max:
        return required_left, required_right

    center = 0.5 * (feasible_ws_min + feasible_ws_max)
    half_width = max(
        center - required_left,
        required_right - center,
        0.60 * (feasible_ws_max - feasible_ws_min),
    )
    centered_left = center - half_width
    centered_right = center + half_width
    if centered_left < 0.0:
        # Negative wing loading is non-physical. Once the left edge reaches
        # zero, retaining every right-side datum takes priority over perfect
        # centering.
        centered_left = 0.0
        centered_right = max(centered_right, required_right)
    return centered_left, centered_right


x_plot_min, x_plot_max = centered_feasible_xlim(x_min, x_max)
tolerance_plot_min, tolerance_plot_max = centered_feasible_xlim(
    tolerance_x_min, tolerance_x_max
)

def shade_feasible_design_region(ax, upper_y, label=True):
    if feasible_ws_min >= feasible_ws_max:
        return
    region_x = np.linspace(feasible_ws_min, feasible_ws_max, 500)
    region_envelope = np.interp(
        region_x,
        envelope["wing_loading"],
        envelope["thrust_to_weight"],
    )
    ax.fill_between(
        region_x,
        region_envelope,
        upper_y,
        color="#2ca02c",
        alpha=0.075,
        label="Feasible design region" if label else None,
        zorder=0,
    )


shade_feasible_design_region(ax, y_max)

ax.scatter(
    aircraft_ws,
    aircraft_tw,
    s=85,
    color="#d62728",
    edgecolor="white",
    linewidth=1.4,
    zorder=6,
    label="Aircraft point from aero Sref",
)

ax.annotate(
    aircraft_annotation,
    xy=(aircraft_ws, aircraft_tw),
    xytext=(18, 28),
    textcoords="offset points",
    fontsize=10,
    arrowprops=dict(
        arrowstyle="-",
        color="#4d4d4d",
        linewidth=1.0,
        shrinkA=4,
        shrinkB=5,
    ),
    bbox=dict(
        boxstyle="round,pad=0.35",
        facecolor="white",
        edgecolor="#b3b3b3",
        alpha=0.96,
    ),
    zorder=7,
)

ax.scatter(
    best_ws,
    best_tw,
    s=115,
    marker="*",
    color="#2ca02c",
    edgecolor="white",
    linewidth=1.2,
    zorder=7,
    label="Best design point",
)

ax.annotate(
    best_annotation,
    xy=(best_ws, best_tw),
    xytext=(-20, -48),
    textcoords="offset points",
    ha="right",
    fontsize=10,
    arrowprops=dict(
        arrowstyle="-",
        color="#4d4d4d",
        linewidth=1.0,
        shrinkA=4,
        shrinkB=5,
    ),
    bbox=dict(
        boxstyle="round,pad=0.35",
        facecolor="white",
        edgecolor="#b3b3b3",
        alpha=0.96,
    ),
    zorder=8,
)

if landing_ws_limit is not None:
    ax.axvline(
        landing_ws_limit,
        color="#7b3294",
        linestyle="--",
        linewidth=1.7,
        alpha=0.9,
        label=f"Landing max ({landing_ws_limit:.0f})",
        zorder=3,
    )

if stall_ws_limit is not None:
    ax.axvline(
        stall_ws_limit,
        color=color_map["Stall speed"],
        linestyle=":",
        linewidth=2.0,
        alpha=0.9,
        label=f"Stall max ({stall_ws_limit:.0f})",
        zorder=3,
    )

if gust_ws_limit is not None:
    ax.axvline(
        gust_ws_limit,
        color=color_map["Gust"],
        linestyle="-.",
        linewidth=1.7,
        alpha=0.9,
        label=f"Gust min ({gust_ws_limit:.0f})",
        zorder=3,
    )

ax.set_title(
    analysis_title("Constraint Analysis Matching Chart"),
    pad=12,
    fontweight="semibold",
)
ax.set_xlabel("Wing Loading, W/S [N/m²]")
ax.set_ylabel(y_axis_label)

if propeller_climb_coverage_note:
    ax.text(
        0.01,
        0.02,
        propeller_climb_coverage_note,
        transform=ax.transAxes,
        fontsize=8.8,
        color="#7c2d12",
        va="bottom",
        bbox=dict(
            boxstyle="round,pad=0.3",
            facecolor="#fff7ed",
            edgecolor="#fdba74",
            alpha=0.94,
        ),
        zorder=10,
    )

ax.set_xlim(x_plot_min, x_plot_max)
ax.set_ylim(y_min, y_max)

ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
ax.grid(axis="x", alpha=0.18)
ax.grid(axis="y", alpha=0.25)

ax.legend(
    loc="upper left",
    bbox_to_anchor=(1.015, 1.0),
    borderaxespad=0.0,
    frameon=False,
    handlelength=2.4,
    labelspacing=0.7,
)

fig.subplots_adjust(right=0.79)
save_plot("01_matching_chart_professional")


# ============================================================
# Plot 1b: Matching chart with gradient tolerance bands
# ============================================================
# This is intentionally a design-detail view. The nominal matching chart
# already provides the complete analysis domain; the tolerance chart focuses
# on the neighbourhood that contains both marked design points.
point_x_low = min(best_ws, aircraft_ws)
point_x_high = max(best_ws, aircraft_ws)
point_x_span = max(point_x_high - point_x_low, 1.0)
point_x_center = 0.5 * (point_x_low + point_x_high)
zoom_x_padding = max(
    0.28 * point_x_span,
    0.08 * max(point_x_center, 1.0),
    250.0,
)
tolerance_zoom_x_min = max(
    tolerance_plot_min, point_x_low - zoom_x_padding,
)
tolerance_zoom_x_max = min(
    tolerance_plot_max, point_x_high + zoom_x_padding,
)

zoom_samples = np.linspace(
    tolerance_zoom_x_min, tolerance_zoom_x_max, 400,
)
zoom_band_values = []
for name, df in constraints.items():
    nominal_zoom = np.interp(
        zoom_samples,
        df["wing_loading"].to_numpy(dtype=float),
        df["thrust_to_weight"].to_numpy(dtype=float),
    )
    lower_fraction, upper_fraction = constraint_tolerances[name]
    zoom_band_values.extend(nominal_zoom * (1.0 - lower_fraction))
    zoom_band_values.extend(nominal_zoom * (1.0 + upper_fraction))
zoom_band_values.extend((aircraft_tw, best_tw))
zoom_y_low = float(np.nanmin(zoom_band_values))
zoom_y_high = float(np.nanmax(zoom_band_values))
zoom_y_span = max(zoom_y_high - zoom_y_low, 1.0e-3)
tolerance_zoom_y_min = max(0.0, zoom_y_low - 0.10 * zoom_y_span)
tolerance_zoom_y_max = zoom_y_high + 0.12 * zoom_y_span

fig, ax = plt.subplots(figsize=(13.5, 6.8))

tolerance_constraint_handles = []
for name, df in constraints.items():
    color = color_map.get(name, "#7f7f7f")
    lower_fraction, upper_fraction = constraint_tolerances[name]
    add_gradient_curve_band(
        ax,
        df["wing_loading"],
        df["thrust_to_weight"],
        color,
        lower_fraction,
        upper_fraction,
        draw_edges=False,
    )
    tolerance_constraint_handles.append(Patch(
        facecolor=color, edgecolor="none", alpha=0.35,
        label=f"{name} relaxed region",
    ))

shade_feasible_design_region(ax, tolerance_y_max)

ax.scatter(
    aircraft_ws,
    aircraft_tw,
    s=85,
    color="#d62728",
    edgecolor="white",
    linewidth=1.4,
    zorder=6,
    label="Aircraft point from aero Sref",
)
ax.scatter(
    best_ws,
    best_tw,
    s=115,
    marker="*",
    color="#2ca02c",
    edgecolor="white",
    linewidth=1.2,
    zorder=7,
    label="Best nominal design point",
)

vertical_band_specs = (
    ("Landing", landing_ws_limit, "--", 1.7, "Landing max"),
    ("Stall speed", stall_ws_limit, ":", 2.0, "Stall max"),
    ("Gust", gust_ws_limit, "-.", 1.7, "Gust min"),
)
for name, limit, linestyle, linewidth, label in vertical_band_specs:
    if limit is None:
        continue
    lower_fraction, upper_fraction = constraint_tolerances[name]
    band_low = limit * (1.0 - lower_fraction)
    band_high = limit * (1.0 + upper_fraction)
    if (band_high < tolerance_zoom_x_min or
            band_low > tolerance_zoom_x_max):
        continue
    color = color_map[name]
    add_gradient_vertical_band(
        ax,
        limit,
        color,
        lower_fraction,
        upper_fraction,
        draw_edges=False,
    )
    tolerance_constraint_handles.append(Patch(
        facecolor=color, edgecolor="none", alpha=0.35,
        label=f"{label} relaxed region ({limit:.0f})",
    ))

ax.set_title(
    analysis_title("Constraint Tolerance Bands — Design-Point Detail"),
    pad=12,
    fontweight="semibold",
)
ax.set_xlabel("Wing Loading, W/S [N/m²]")
ax.set_ylabel(y_axis_label)
if propeller_climb_coverage_note:
    ax.text(
        0.01,
        0.02,
        propeller_climb_coverage_note,
        transform=ax.transAxes,
        fontsize=8.8,
        color="#7c2d12",
        va="bottom",
        bbox=dict(
            boxstyle="round,pad=0.3",
            facecolor="#fff7ed",
            edgecolor="#fdba74",
            alpha=0.94,
        ),
        zorder=10,
    )
ax.set_xlim(tolerance_zoom_x_min, tolerance_zoom_x_max)
ax.set_ylim(tolerance_zoom_y_min, tolerance_zoom_y_max)
ax.spines["top"].set_visible(False)
ax.spines["right"].set_visible(False)
ax.grid(axis="x", alpha=0.18)
ax.grid(axis="y", alpha=0.25)

tolerance_handles, tolerance_labels = ax.get_legend_handles_labels()
tolerance_handles = tolerance_constraint_handles + tolerance_handles
tolerance_handles.append(Patch(
    facecolor="#7f7f7f",
    edgecolor="none",
    alpha=0.42,
    label="Region width: default ±10%",
))
ax.legend(
    handles=tolerance_handles,
    loc="upper left",
    bbox_to_anchor=(1.015, 1.0),
    borderaxespad=0.0,
    frameon=False,
    handlelength=2.4,
    labelspacing=0.7,
)

fig.subplots_adjust(right=0.79)
save_plot("01_matching_chart_with_tolerance_bands")


# ============================================================
# Plot 2: Active constraint map
# ============================================================
x_env = envelope["wing_loading"].values
y_env = envelope["thrust_to_weight"].values

interp_values = {}

for name, df in constraints.items():
    interp_values[name] = np.interp(
        x_env,
        df["wing_loading"].values,
        df["thrust_to_weight"].values,
    )

active = []

for i in range(len(x_env)):
    values_here = {name: values[i] for name, values in interp_values.items()}
    active.append(max(values_here, key=values_here.get))


fig, ax = plt.subplots(figsize=(13.5, 6.8))

for name, df in constraints.items():
    ax.plot(
        df["wing_loading"],
        df["thrust_to_weight"],
        color=color_map.get(name, None),
        alpha=0.25,
        linewidth=1.2,
    )

performance_legend_handles = [
    Line2D(
        [0],
        [0],
        color=color_map.get(name, "black"),
        linewidth=2.2,
        label=name,
    )
    for name in constraints
]

# Continuous envelope background to avoid visual gaps
ax.plot(
    x_env,
    y_env,
    color="black",
    linewidth=2.0,
    alpha=0.35,
    label="Envelope"
)
envelope_legend_handle = Line2D(
    [0], [0], color="black", linewidth=3.0, label="Constraint envelope"
)

start = 0

for i in range(1, len(x_env) + 1):
    if i == len(x_env) or active[i] != active[start]:
        name = active[start]

        plot_start = max(start - 1, 0)
        plot_end = i

        ax.plot(
            x_env[plot_start:plot_end],
            y_env[plot_start:plot_end],
            linewidth=4.0,
            color=color_map.get(name, "black"),
            label=name,
        )

        mid = (start + i - 1) // 2

        ax.text(
            x_env[mid],
            y_env[mid] + 0.035 * y_max,
            name,
            ha="center",
            va="bottom",
            fontsize=10,
            weight="bold",
            bbox=dict(facecolor="white", edgecolor="none", alpha=0.75),
        )

        start = i

aircraft_point_handle = ax.scatter(
    aircraft_ws,
    aircraft_tw,
    s=85,
    color="#d62728",
    edgecolor="white",
    linewidth=1.2,
    zorder=6,
    label="Aircraft point from aero Sref",
)

best_point_handle = ax.scatter(
    best_ws,
    best_tw,
    s=115,
    marker="*",
    color="#2ca02c",
    edgecolor="white",
    linewidth=1.2,
    zorder=7,
    label="Best design point",
)

reference_legend_handles = [aircraft_point_handle, best_point_handle]

if landing_ws_limit is not None:
    landing_handle = ax.axvline(
        landing_ws_limit,
        color="purple",
        linestyle="--",
        linewidth=2.0,
        label=f"Landing W/S max ({landing_ws_limit:.0f})",
    )
    reference_legend_handles.append(landing_handle)

if stall_ws_limit is not None:
    stall_handle = ax.axvline(
        stall_ws_limit,
        color=color_map["Stall speed"],
        linestyle=":",
        linewidth=2.4,
        label=f"Stall W/S max ({stall_ws_limit:.0f})",
    )
    reference_legend_handles.append(stall_handle)

if gust_ws_limit is not None:
    gust_handle = ax.axvline(
        gust_ws_limit,
        color=color_map["Gust"],
        linestyle="-.",
        linewidth=2.0,
        label=f"Gust W/S min ({gust_ws_limit:.0f})",
    )
    reference_legend_handles.append(gust_handle)

ax.set_title(analysis_title("Active Constraint Regions"))
ax.set_xlabel("Wing Loading, W/S [N/m²]")
ax.set_ylabel(y_axis_label)
ax.set_xlim(x_plot_min, x_plot_max)
ax.set_ylim(y_min, y_max)

performance_legend = ax.legend(
    handles=[envelope_legend_handle, *performance_legend_handles],
    title="Performance constraints",
    loc="upper left",
    bbox_to_anchor=(1.01, 1.0),
    ncol=1,
    frameon=False,
)
ax.add_artist(performance_legend)

ax.legend(
    handles=reference_legend_handles,
    title="Design points and wing-loading limits",
    loc="lower left",
    bbox_to_anchor=(1.01, 0.0),
    frameon=False,
)

clean_axes(ax)
fig.subplots_adjust(right=0.76)
save_plot("02_active_constraint_regions")


# ============================================================
# Plot 3: Constraint utilization at the two decision points
# ============================================================
def constraint_requirements_at(wing_loading):
    return {
        name: float(np.interp(
            wing_loading,
            curve["wing_loading"].to_numpy(dtype=float),
            curve["thrust_to_weight"].to_numpy(dtype=float),
        ))
        for name, curve in constraints.items()
    }


def utilization_color(value):
    if value >= 99.5:
        return "#dc2626"
    if value >= 90.0:
        return "#f59e0b"
    return "#2563eb"


decision_points = [
    ("Best feasible design", best_ws, best_tw),
    ("Aircraft reference-area point", aircraft_ws, aircraft_tw),
]
fig, utilization_axes = plt.subplots(
    1, 2, figsize=(13.2, 6.4), sharex=True, sharey=True,
)
all_utilizations = []
for utilization_ax, (point_name, point_ws, point_y) in zip(
        utilization_axes, decision_points):
    requirements = constraint_requirements_at(point_ws)
    ordered = sorted(
        requirements.items(), key=lambda item: item[1], reverse=True,
    )
    names = [item[0] for item in ordered][::-1]
    utilization = np.asarray([
        100.0 * item[1] / point_y if point_y > 0.0 else np.nan
        for item in ordered
    ])[::-1]
    all_utilizations.extend(utilization[np.isfinite(utilization)])
    bars = utilization_ax.barh(
        names, utilization,
        color=[utilization_color(value) for value in utilization],
        alpha=0.88, height=0.66,
    )
    utilization_ax.axvline(
        100.0, color="#111827", linewidth=1.4, linestyle="--",
    )
    for bar, value in zip(bars, utilization):
        utilization_ax.text(
            value + 1.0,
            bar.get_y() + 0.5 * bar.get_height(),
            f"{value:.1f}%",
            va="center", fontsize=8.7, color="#334155",
        )
    utilization_ax.set_title(
        f"{point_name}\nW/S = {point_ws:.0f} N/m², {y_symbol} = {point_y:.3f}",
        fontsize=12,
    )
    utilization_ax.set_xlabel("Constraint utilization [%]")
    clean_axes(utilization_ax)

utilization_max = max(all_utilizations, default=100.0)
for utilization_ax in utilization_axes:
    utilization_ax.set_xlim(0.0, max(112.0, 1.10 * utilization_max))

fig.suptitle(analysis_title("Constraint Utilization Dashboard"))
fig.text(
    0.5, 0.015,
    "100% identifies the governing boundary; values above 100% are infeasible.",
    ha="center", fontsize=9.2, color="#475569",
)
fig.tight_layout(rect=(0.0, 0.04, 1.0, 0.94))
save_plot("03_constraint_utilization_dashboard")


# ============================================================
# Plot 4: Governing constraint and runner-up separation
# ============================================================
feasible_mask = (
    (x_env >= feasible_ws_min) & (x_env <= feasible_ws_max)
)
if not np.any(feasible_mask):
    feasible_mask = np.ones_like(x_env, dtype=bool)

gap_x = x_env[feasible_mask]
constraint_matrix = np.vstack([
    interp_values[name][feasible_mask] for name in constraints
])
constraint_names = list(constraints)
ranked_indices = np.argsort(constraint_matrix, axis=0)
governing_indices = ranked_indices[-1]
runner_up_indices = ranked_indices[-2]
governing_values = np.take_along_axis(
    constraint_matrix, governing_indices[np.newaxis, :], axis=0,
).ravel()
runner_up_values = np.take_along_axis(
    constraint_matrix, runner_up_indices[np.newaxis, :], axis=0,
).ravel()
separation_percent = np.where(
    governing_values > 0.0,
    100.0 * (governing_values - runner_up_values) / governing_values,
    np.nan,
)

fig, (governing_ax, gap_ax) = plt.subplots(
    2, 1, figsize=(13.0, 7.4), sharex=True,
    gridspec_kw={"height_ratios": [2.0, 1.0]},
)
governing_ax.plot(
    gap_x, governing_values, color="#111827", linewidth=1.0, alpha=0.35,
)

segment_start = 0
transition_points = []
for index in range(1, len(gap_x) + 1):
    if (index == len(gap_x) or
            governing_indices[index] != governing_indices[segment_start]):
        segment_end = index
        constraint_name = constraint_names[governing_indices[segment_start]]
        plot_start = max(segment_start - 1, 0)
        governing_ax.plot(
            gap_x[plot_start:segment_end],
            governing_values[plot_start:segment_end],
            color=color_map.get(constraint_name, "#111827"),
            linewidth=5.0, solid_capstyle="round",
        )
        middle = (segment_start + segment_end - 1) // 2
        governing_ax.annotate(
            constraint_name,
            (gap_x[middle], governing_values[middle]),
            xytext=(0, 11), textcoords="offset points",
            ha="center", fontsize=9.3, weight="bold",
            color=color_map.get(constraint_name, "#111827"),
            bbox=dict(
                boxstyle="round,pad=0.2", facecolor="white",
                edgecolor="none", alpha=0.84,
            ),
        )
        if segment_end < len(gap_x):
            transition_points.append(segment_end)
        segment_start = index

for transition_index in transition_points:
    transition_ws = gap_x[transition_index]
    for target_ax in (governing_ax, gap_ax):
        target_ax.axvline(
            transition_ws, color="#64748b", linestyle=":",
            linewidth=1.0, alpha=0.8,
        )

governing_ax.scatter(
    [best_ws, aircraft_ws], [best_tw, aircraft_tw],
    marker="*", s=[130, 110], color=["#16a34a", "#dc2626"],
    edgecolor="white", linewidth=0.9, zorder=5,
)
governing_ax.set_ylabel(y_axis_label)
governing_ax.set_title("Governing requirement across the feasible W/S corridor")
clean_axes(governing_ax)

gap_ax.fill_between(
    gap_x, 0.0, separation_percent,
    color="#7c3aed", alpha=0.34, linewidth=0.0,
)
gap_ax.plot(gap_x, separation_percent, color="#6d28d9", linewidth=1.4)
gap_ax.axhline(5.0, color="#f59e0b", linestyle="--", linewidth=1.0)
gap_ax.text(
    0.99, 5.0, "5% close competition", transform=gap_ax.get_yaxis_transform(),
    ha="right", va="bottom", fontsize=8.5, color="#b45309",
)
gap_ax.set_xlabel("Wing Loading, W/S [N/m²]")
gap_ax.set_ylabel("Lead over runner-up [%]")
gap_ax.set_ylim(bottom=0.0)
clean_axes(gap_ax)

fig.suptitle(analysis_title("Governing Constraint Gap Map"))
fig.tight_layout(rect=(0.0, 0.0, 1.0, 0.95))
save_plot("04_governing_constraint_gap_map")

if propeller_mode:
    # ============================================================
    # Plot 3: Integrated takeoff history
    # ============================================================
    takeoff_profile_path = os.path.join(
        output_dir, "propeller_takeoff_profile.csv"
    )
    if os.path.exists(takeoff_profile_path):
        takeoff = pd.read_csv(takeoff_profile_path)
        fig, ax1 = plt.subplots(figsize=(10.5, 6.0))
        ax1.plot(
            takeoff["speed_ms"],
            takeoff["distance_m"],
            color="#1f77b4",
            linewidth=2.4,
            label="Integrated ground roll",
        )
        ax1.set_xlabel("Ground Speed, V [m/s]")
        ax1.set_ylabel("Ground-Roll Distance [m]", color="#1f77b4")
        ax1.tick_params(axis="y", labelcolor="#1f77b4")
        ax2 = ax1.twinx()
        ax2.plot(
            takeoff["speed_ms"],
            takeoff["acceleration_ms2"],
            color="#d62728",
            linewidth=2.0,
            linestyle="--",
            label="Acceleration",
        )
        ax2.set_ylabel("Acceleration [m/s²]", color="#d62728")
        ax2.tick_params(axis="y", labelcolor="#d62728")
        ax1.set_title(analysis_title("Integrated Takeoff Ground Roll"))
        lines1, labels1 = ax1.get_legend_handles_labels()
        lines2, labels2 = ax2.get_legend_handles_labels()
        ax1.legend(lines1 + lines2, labels1 + labels2, loc="upper left")
        save_plot("03_propeller_takeoff_profile")

    # ============================================================
    # Plot 4: Required power versus both availability definitions
    # ============================================================
    capacity_path = os.path.join(
        output_dir, "propeller_capacity_check.csv"
    )
    if os.path.exists(capacity_path):
        capacity = pd.read_csv(capacity_path)
        x = np.arange(len(capacity))
        width = 0.36
        fig, ax = plt.subplots(figsize=(11.0, 6.0))
        ax.bar(
            x - width / 2,
            capacity["required_total_shaft_power_W"] / 1.0e6,
            width,
            label="Required",
            color="#d62728",
        )
        ax.bar(
            x + width / 2,
            capacity["deck_total_shaft_power_W"] / 1.0e6,
            width,
            label="Supplied propeller-deck output",
            color="#1f77b4",
        )
        ax.set_xticks(x, capacity["case"].str.title())
        ax.set_ylabel("Total Shaft Power [MW]")
        ax.set_title(
            analysis_title("Required Power and Supplied Propeller-Map Reference")
        )
        ax.legend(frameon=False)
        save_plot("04_propeller_capacity_check")

    # ============================================================
    # Plot 5: Propeller power-loading constraint carpet
    # ============================================================
    propeller_carpet_path = os.path.join(
        output_dir, "propeller_constraint_carpet.csv"
    )
    if os.path.exists(propeller_carpet_path):
        carpet = pd.read_csv(propeller_carpet_path)
        fig, ax = plt.subplots(figsize=(13.0, 7.0))
        carpet_colors = {
            "Acceleration": "#0072B2",
            "Climb": "#009E73",
            "Cruise": "#E69F00",
            "Subsonic Climb": "#009E73",
            "Subsonic Cruise": "#E69F00",
            "Transonic Climb": "#9467BD",
            "Transonic Cruise": "#17BECF",
            "Supersonic Climb": "#006D2C",
            "Supersonic Cruise": "#D95F02",
            "Takeoff": "#D55E00",
            "Turn": "#CC79A7",
        }
        for constraint_name, group in carpet.groupby("constraint"):
            group = group.sort_values("wing_loading_N_m2")
            display_name = constraint_name.replace("propeller_", "").replace(
                "_constraint", ""
            ).replace("_", " ").title()
            ax.plot(
                group["wing_loading_N_m2"],
                group["required_shaft_power_to_weight_W_N"],
                linewidth=1.8,
                color=carpet_colors.get(display_name),
                alpha=0.80,
                label=display_name,
            )

        ax.plot(
            envelope["wing_loading"],
            envelope["thrust_to_weight"],
            color="black",
            linewidth=3.0,
            label="Constraint envelope",
            zorder=5,
        )
        optimum_label = (
            f"W/S = {best_ws:.0f} N/m²\nP/W = {best_tw:.3f} W/N"
        )
        if best_power_MW is not None:
            optimum_label += f"\nP = {best_power_MW:.2f} MW"
        if best_area_m2 is not None:
            optimum_label += f"\nS = {best_area_m2:.1f} m²"
        add_design_point(
            ax, best_ws, best_tw, "Best feasible design point",
            optimum_label, offset=(-24, -72),
        )
        if gust_ws_limit is not None:
            ax.axvline(
                gust_ws_limit,
                color="#7f7f7f",
                linestyle="-.",
                linewidth=1.8,
                label=f"Gust min ({gust_ws_limit:.0f})",
            )
        if landing_ws_limit is not None:
            ax.axvline(
                landing_ws_limit,
                color="#9467bd",
                linestyle="--",
                linewidth=1.8,
                label=f"Landing max ({landing_ws_limit:.0f})",
            )
        if stall_ws_limit is not None:
            ax.axvline(
                stall_ws_limit,
                color="#bcbd22",
                linestyle=":",
                linewidth=1.8,
                label=f"Stall max ({stall_ws_limit:.0f})",
            )

        carpet_y_max = max(
            carpet["required_shaft_power_to_weight_W_N"].max(),
            envelope["thrust_to_weight"].max(),
        ) * 1.08
        shade_feasible_design_region(ax, carpet_y_max)

        if takeoff_weight_N is not None:
            secondary = ax.secondary_yaxis(
                "right",
                functions=(
                    lambda power_loading: power_loading * takeoff_weight_N / 1.0e6,
                    lambda power_MW: power_MW * 1.0e6 / takeoff_weight_N,
                ),
            )
            secondary.set_ylabel("Required Total Shaft Power [MW]")

        ax.set_title(analysis_title("Power-Loading Constraint Family"))
        ax.text(
            0.01, 0.02,
            "Feasible designs lie above the black envelope and inside the wing-loading limits.",
            transform=ax.transAxes, fontsize=9, color="#4d4d4d", va="bottom",
        )
        ax.set_xlabel("Wing Loading, W/S [N/m²]")
        ax.set_ylabel("Shaft Power Loading, P/W [W/N]")
        ax.set_xlim(x_plot_min, x_plot_max)
        ax.set_ylim(0.0, carpet_y_max)
        clean_axes(ax)
        ax.legend(loc="upper left", bbox_to_anchor=(1.01, 1.0),
                  frameon=False, title="Constraints and limits")
        fig.subplots_adjust(right=0.79)
        save_plot("05_propeller_constraint_carpet")
    # ============================================================
    # Propeller-deck evidence and aircraft-level aerodynamic studies
    # ============================================================
    performance_map_path = os.path.join(
        output_dir, "propeller_performance_map.csv"
    )
    if os.path.exists(performance_map_path):
        prop_map = pd.read_csv(performance_map_path).dropna()
        valid_map = prop_map[
            (prop_map["efficiency"] >= 0.0) &
            (prop_map["efficiency"] <= 1.0)
        ].copy()
        if len(valid_map) < 3:
            raise RuntimeError(
                "Propeller performance map has too few physically valid points."
            )

        pitch_values = np.sort(valid_map["pitch_deg"].unique())
        best_map_row = valid_map.loc[valid_map["efficiency"].idxmax()]

        fig, ax = plt.subplots(figsize=(10.0, 6.0))
        pitch_colors = mpl.colormaps["viridis"](
            np.linspace(0.18, 0.82, len(pitch_values))
        )
        for color, pitch in zip(pitch_colors, pitch_values):
            pitch_slice = valid_map[
                valid_map["pitch_deg"] == pitch
            ].sort_values("advance_ratio")
            ax.plot(
                pitch_slice["advance_ratio"], pitch_slice["efficiency"],
                color=color, marker="o", markersize=4.0, linewidth=2.2,
                label=f"Constant pitch = {pitch:g}°",
            )
        ax.scatter(
            best_map_row["advance_ratio"], best_map_row["efficiency"],
            marker="*", s=165, color="#fbbf24", edgecolor="#0f172a",
            linewidth=0.9, label="Best supplied map point", zorder=8,
        )
        ax.annotate(
            f"η = {best_map_row['efficiency']:.3f}\n"
            f"J = {best_map_row['advance_ratio']:.3f}, "
            f"pitch = {best_map_row['pitch_deg']:g}°",
            (best_map_row["advance_ratio"], best_map_row["efficiency"]),
            xytext=(10, 12), textcoords="offset points", fontsize=8.5,
            color="#334155",
            bbox=dict(boxstyle="round,pad=0.25", facecolor="white",
                      edgecolor="#cbd5e1", alpha=0.92),
        )
        ax.set_title(analysis_title("Propeller Performance Map"))
        ax.set_xlabel("Advance Ratio, J [-]")
        ax.set_ylabel("Propeller Efficiency, η [-]")
        ax.set_ylim(0.0, 1.0)
        ax.text(
            0.01, 0.02,
            "Lines connect physically valid points supplied by the propeller "
            "deck; no pitch interpolation is shown.",
            transform=ax.transAxes, fontsize=8.8, color="#4d4d4d",
            va="bottom",
        )
        clean_axes(ax)
        ax.legend(frameon=False)
        save_plot("03_propeller_performance_map")

    prop_aero_carpet_path = os.path.join(
        output_dir, "propeller_cd0_k_carpet.csv"
    )
    prop_cd0_path = os.path.join(
        output_dir, "propeller_cd0_sensitivity_curves.csv"
    )
    prop_k_path = os.path.join(
        output_dir, "propeller_k_sensitivity_curves.csv"
    )
    missing_study_paths = [
        required_path for required_path in (
            prop_aero_carpet_path, prop_cd0_path, prop_k_path
        )
        if not os.path.exists(required_path)
    ]
    if missing_study_paths:
        print()
        print(
            "Optional propeller parameter-study CSV files are absent; "
            "main-result plots are complete and study plots were skipped."
        )
        print(
            "Run the C++ application with --with-studies, then rerun this "
            "script to generate sensitivity and carpet plots."
        )
        print()
        print("Plot generation completed.")
        print(
            f"Aircraft point: W/S = {aircraft_ws:.0f} N/m², "
            f"{y_symbol} = {aircraft_tw:.4f}"
        )
        print(
            f"Best design point: W/S = {best_ws:.0f} N/m², "
            f"{y_symbol} = {best_tw:.4f}"
        )
        print(f"Plots saved to: {save_dir}")
        raise SystemExit(0)

    prop_aero_carpet = pd.read_csv(prop_aero_carpet_path).dropna()
    prop_cd0_sensitivity = pd.read_csv(prop_cd0_path).dropna()
    prop_k_sensitivity = pd.read_csv(prop_k_path).dropna()
    required_diagnostic_columns = {
        "active_constraint_value", "second_constraint_name",
        "second_constraint_value", "constraint_margin",
    }
    if not required_diagnostic_columns.issubset(prop_aero_carpet.columns):
        raise RuntimeError(
            "Rerun the C++ application: carpet diagnostic schema is outdated."
        )

    cd0_values = np.sort(prop_aero_carpet["cd_0"].unique())
    k_values = np.sort(
        prop_aero_carpet["induced_drag_factor"].unique()
    )
    if (len(cd0_values) != 9 or len(k_values) != 9 or
            len(prop_aero_carpet) != 81):
        raise RuntimeError(
            "Propeller aerodynamic carpet must be a complete 9x9 grid."
        )

    prop_baseline = prop_aero_carpet[
        prop_aero_carpet["is_baseline"] == 1
    ]
    if len(prop_baseline) != 1:
        raise RuntimeError(
            "Propeller aerodynamic carpet must contain one nominal point."
        )
    prop_baseline_row = prop_baseline.iloc[0]
    nominal_cd0 = float(prop_baseline_row["cd_0"])
    nominal_k = float(prop_baseline_row["induced_drag_factor"])
    label_indices = {0, 2, 4, 6, 8}

    # CD0 sensitivity: the acceleration P/W family at fixed nominal k.
    acceleration_cd0 = prop_cd0_sensitivity[
        prop_cd0_sensitivity["constraint_name"].str.contains(
            "acceleration", case=False, na=False
        )
    ].copy()
    if len(acceleration_cd0["cd_0"].unique()) != 9:
        raise RuntimeError(
            "Propeller CD0 sensitivity must contain nine acceleration curves."
        )

    fig, ax = plt.subplots(figsize=(10.5, 6.2))
    cd0_norm = mpl.colors.Normalize(
        vmin=cd0_values.min(), vmax=cd0_values.max()
    )
    cd0_cmap = mpl.colormaps["Greens"]
    for cd0_index, cd0 in enumerate(cd0_values):
        curve = acceleration_cd0[np.isclose(
            acceleration_cd0["cd_0"], cd0,
            rtol=1.0e-9, atol=1.0e-12,
        )].sort_values("wing_loading")
        is_nominal = np.isclose(
            cd0, nominal_cd0, rtol=1.0e-9, atol=1.0e-12
        )
        ax.plot(
            curve["wing_loading"], curve["thrust_to_weight"],
            color="#064e3b" if is_nominal else cd0_cmap(cd0_norm(cd0)),
            linewidth=3.0 if is_nominal else 1.45,
            alpha=1.0 if is_nominal else 0.72,
            label=(f"Nominal CD₀ = {cd0:.5f}" if is_nominal else None),
            zorder=4 if is_nominal else 2,
        )
        if cd0_index in label_indices:
            label_row = curve.iloc[-1]
            ax.annotate(
                f"{cd0 / nominal_cd0:.0%}",
                (label_row["wing_loading"], label_row["thrust_to_weight"]),
                xytext=(5, 0), textcoords="offset points",
                fontsize=8.5, color="#166534", va="center",
            )

    ax.clear()
    cd0_region_handle = add_curve_family_region(
        ax, acceleration_cd0, "cd_0", cd0_values,
        "#15803d", "Acceleration P/W region (CD₀: 80–120%)",
    )
    set_feasible_sensitivity_xlim(
        ax, acceleration_cd0, feasible_ws_min, feasible_ws_max
    )
    ax.set_title(analysis_title("CD₀ Sensitivity Region"))
    ax.set_xlabel("Wing Loading, W/S [N/m²]")
    ax.set_ylabel("Required Shaft Power Loading, P/W [W/N]")
    ax.text(
        0.01, 0.02,
        "Only CD₀ varies (80–120%); k, propeller deck, mission and all "
        "other aircraft parameters remain constant.",
        transform=ax.transAxes, fontsize=8.8, color="#4d4d4d",
        va="bottom",
    )
    clean_axes(ax)
    ax.legend(handles=[cd0_region_handle], loc="upper right", frameon=False)
    save_plot("04_cd0_parameter_sensitivity")

    # k sensitivity: acceleration P/W curves and the associated gust limits.
    required_k_columns = {
        "induced_drag_factor", "constraint_name", "wing_loading",
        "thrust_to_weight", "gust_wing_loading_limit",
    }
    if not required_k_columns.issubset(prop_k_sensitivity.columns):
        raise RuntimeError("Propeller k sensitivity CSV schema is outdated.")
    sensitivity_k_values = np.sort(
        prop_k_sensitivity["induced_drag_factor"].unique()
    )
    if len(sensitivity_k_values) != 9:
        raise RuntimeError(
            "Propeller k sensitivity must contain nine parameter levels."
        )

    fig, ax = plt.subplots(figsize=(10.5, 6.2))
    k_norm = mpl.colors.Normalize(
        vmin=sensitivity_k_values.min(), vmax=sensitivity_k_values.max()
    )
    k_cmap = mpl.colormaps["Blues"]
    gust_cmap = mpl.colormaps["Oranges"]
    for k_index, induced_drag_factor in enumerate(sensitivity_k_values):
        curve = prop_k_sensitivity[np.isclose(
            prop_k_sensitivity["induced_drag_factor"],
            induced_drag_factor, rtol=1.0e-9, atol=1.0e-12,
        )].sort_values("wing_loading")
        is_nominal = np.isclose(
            induced_drag_factor, nominal_k,
            rtol=1.0e-9, atol=1.0e-12,
        )
        ax.plot(
            curve["wing_loading"], curve["thrust_to_weight"],
            color="#1e3a8a" if is_nominal
            else k_cmap(k_norm(induced_drag_factor)),
            linewidth=3.0 if is_nominal else 1.45,
            alpha=1.0 if is_nominal else 0.72,
            label=(f"Nominal k = {induced_drag_factor:.5f}"
                   if is_nominal else None),
            zorder=4 if is_nominal else 2,
        )
        gust_limit = float(curve["gust_wing_loading_limit"].iloc[0])
        ax.axvline(
            gust_limit,
            color="#9a3412" if is_nominal
            else gust_cmap(k_norm(induced_drag_factor)),
            linewidth=2.2 if is_nominal else 0.9,
            alpha=0.95 if is_nominal else 0.42,
            linestyle="--" if is_nominal else "-",
            label=(f"Nominal gust limit = {gust_limit:.0f} N/m²"
                   if is_nominal else None),
            zorder=3,
        )
        if k_index in label_indices:
            label_row = curve.iloc[-1]
            ax.annotate(
                f"{induced_drag_factor / nominal_k:.0%}",
                (label_row["wing_loading"], label_row["thrust_to_weight"]),
                xytext=(5, 0), textcoords="offset points",
                fontsize=8.5, color="#1d4ed8", va="center",
            )

    ax.clear()
    k_region_handle = add_curve_family_region(
        ax, prop_k_sensitivity, "induced_drag_factor",
        sensitivity_k_values, "#2563eb",
        "Acceleration P/W region (k: 80–120%)",
    )
    set_feasible_sensitivity_xlim(
        ax, prop_k_sensitivity, feasible_ws_min, feasible_ws_max
    )
    prop_gust_limits = prop_k_sensitivity.groupby(
        "induced_drag_factor"
    )["gust_wing_loading_limit"].first().to_numpy()
    gust_region_handle = add_vertical_family_region(
        ax, prop_gust_limits, "#ea580c", "Gust-limit region (k: 80–120%)",
    )
    ax.set_title(analysis_title(
        f"Acceleration and Gust-Limit P/W Sensitivity to k "
        f"at Nominal CD₀ = {nominal_cd0:.5f}"
    ))
    ax.set_xlabel("Wing Loading, W/S [N/m²]")
    ax.set_ylabel("Required Shaft Power Loading, P/W [W/N]")
    ax.text(
        0.01, 0.02,
        "Only k varies (80–120%); CD₀, propeller deck, mission and all "
        "other aircraft parameters remain constant.",
        transform=ax.transAxes, fontsize=8.8, color="#4d4d4d",
        va="bottom",
    )
    clean_axes(ax)
    ax.legend(handles=[k_region_handle, gust_region_handle],
              loc="upper right", frameon=False)
    save_plot("05_k_parameter_sensitivity")

    # Classical aircraft-level carpet: optimum W/S and P/W responses.
    fig, ax = plt.subplots(figsize=(10.0, 7.0))
    cd0_color = "#10b981"
    k_color = "#ef4444"
    for cd0_index, cd0 in enumerate(cd0_values):
        family = prop_aero_carpet[np.isclose(
            prop_aero_carpet["cd_0"], cd0,
            rtol=1.0e-9, atol=1.0e-12,
        )].sort_values("induced_drag_factor")
        ax.plot(
            family["best_wing_loading"],
            family["best_thrust_to_weight"],
            color=cd0_color,
            linewidth=2.4 if cd0_index in label_indices else 1.25,
            alpha=1.0 if cd0_index in label_indices else 0.42,
        )
        if cd0_index in label_indices:
            label_row = family.iloc[-1]
            ax.annotate(
                f"CD₀ = {cd0:.4f}",
                (label_row["best_wing_loading"],
                 label_row["best_thrust_to_weight"]),
                xytext=(7, 5), textcoords="offset points",
                fontsize=8.5, color="#047857",
            )

    for k_index, induced_drag_factor in enumerate(k_values):
        family = prop_aero_carpet[np.isclose(
            prop_aero_carpet["induced_drag_factor"],
            induced_drag_factor, rtol=1.0e-9, atol=1.0e-12,
        )].sort_values("cd_0")
        ax.plot(
            family["best_wing_loading"],
            family["best_thrust_to_weight"],
            color=k_color,
            linewidth=2.2 if k_index in label_indices else 1.2,
            alpha=1.0 if k_index in label_indices else 0.38,
            marker="o", markersize=4.2,
            markerfacecolor="#0f172a", markeredgecolor="#0f172a",
        )
        if k_index in label_indices:
            label_row = family.iloc[-1]
            ax.annotate(
                f"k = {induced_drag_factor:.4f}",
                (label_row["best_wing_loading"],
                 label_row["best_thrust_to_weight"]),
                xytext=(7, -7), textcoords="offset points",
                fontsize=8.5, color="#b91c1c",
            )

    baseline_active = str(
        prop_baseline_row["active_constraint_name"]
    )
    ax.scatter(
        prop_baseline_row["best_wing_loading"],
        prop_baseline_row["best_thrust_to_weight"],
        marker="*", s=180, color="#fbbf24", edgecolor="#0f172a",
        linewidth=0.9, zorder=9,
    )
    ax.annotate(
        f"Nominal polar\nActive: {baseline_active}",
        (prop_baseline_row["best_wing_loading"],
         prop_baseline_row["best_thrust_to_weight"]),
        xytext=(12, 14), textcoords="offset points", fontsize=8.5,
        color="#334155",
        bbox=dict(boxstyle="round,pad=0.25", facecolor="white",
                  edgecolor="#cbd5e1", alpha=0.92),
    )

    family_handles = [
        Line2D([0], [0], color=cd0_color, linewidth=2.4,
               label="Constant CD₀"),
        Line2D([0], [0], color=k_color, linewidth=2.2,
               marker="o", markerfacecolor="#0f172a",
               markeredgecolor="#0f172a", label="Constant k"),
        Line2D([0], [0], color="#fbbf24", marker="*",
               markeredgecolor="#0f172a", linestyle="None",
               markersize=11, label="Imported nominal polar"),
    ]
    ax.set_title(analysis_title(
        "Classical Aerodynamic Power-Loading Carpet Plot"
    ))
    ax.set_xlabel("Optimum Wing Loading, W/S [N/m²]")
    ax.set_ylabel("Optimum Required P/W [W/N]")
    clean_axes(ax)
    ax.legend(handles=family_handles, frameon=False)
    ax.text(
        0.01, 0.02,
        "81 propeller constraint solutions; labels show every other CD₀ "
        "and k level (80–120% of the imported polar).",
        transform=ax.transAxes, fontsize=8.8, color="#4d4d4d",
        va="bottom",
    )
    save_plot("06_classical_carpet_plot")

# ============================================================
# Jet CD₀ sensitivity plots
# ============================================================
study_path = os.path.join(output_dir, "carpet_plot_study.csv")

if not propeller_mode and os.path.exists(study_path):

    study = pd.read_csv(study_path)

    # ========================================================
    # Lecture-style one-parameter constraint-family sensitivity
    # ========================================================
    aerodynamic_carpet_path = os.path.join(
        output_dir, "jet_cd0_k_carpet.csv"
    )
    if not os.path.exists(aerodynamic_carpet_path):
        raise FileNotFoundError(
            "jet_cd0_k_carpet.csv not found; rerun the C++ application."
        )
    sensitivity_carpet = pd.read_csv(aerodynamic_carpet_path).dropna()
    required_sensitivity_columns = {
        "cd_0", "induced_drag_factor", "best_thrust_to_weight",
        "is_baseline", "active_constraint_name",
    }
    if not required_sensitivity_columns.issubset(
            sensitivity_carpet.columns):
        raise RuntimeError(
            "Rerun the C++ application: carpet CSV schema is outdated."
        )

    baseline_rows = sensitivity_carpet[
        sensitivity_carpet["is_baseline"] == 1
    ]
    if len(baseline_rows) != 1:
        raise RuntimeError(
            "Jet carpet must contain exactly one nominal polar point."
        )
    baseline_row = baseline_rows.iloc[0]
    nominal_k = float(baseline_row["induced_drag_factor"])
    sensitivity_cd0_values = np.sort(
        sensitivity_carpet["cd_0"].unique()
    )
    if len(sensitivity_cd0_values) != 9:
        raise RuntimeError(
            "CD0 sensitivity must contain nine parameter levels."
        )

    curve_family_path = os.path.join(
        output_dir, "true_carpet_constraints.csv"
    )
    if not os.path.exists(curve_family_path):
        raise FileNotFoundError(
            "true_carpet_constraints.csv not found; rerun the C++ application."
        )
    curve_family = pd.read_csv(curve_family_path).dropna()
    required_curve_columns = {
        "cd_0", "constraint_name", "wing_loading", "thrust_to_weight"
    }
    if not required_curve_columns.issubset(curve_family.columns):
        raise RuntimeError("Constraint-family CSV schema is outdated.")

    acceleration_family = curve_family[
        curve_family["constraint_name"].str.contains(
            "acceleration", case=False, na=False
        )
    ].copy()
    if len(acceleration_family["cd_0"].unique()) != 9:
        raise RuntimeError(
            "Acceleration sensitivity must contain nine CD0 curves."
        )

    fig, ax = plt.subplots(figsize=(10.5, 6.2))
    normalization = mpl.colors.Normalize(
        vmin=sensitivity_cd0_values.min(),
        vmax=sensitivity_cd0_values.max(),
    )
    sensitivity_cmap = mpl.colormaps["Greens"]
    label_indices = {0, 2, 4, 6, 8}
    nominal_cd0 = float(baseline_row["cd_0"])

    for cd0_index, cd0 in enumerate(sensitivity_cd0_values):
        curve = acceleration_family[np.isclose(
            acceleration_family["cd_0"], cd0,
            rtol=1.0e-9, atol=1.0e-12,
        )].sort_values("wing_loading")
        is_nominal = np.isclose(
            cd0, nominal_cd0, rtol=1.0e-9, atol=1.0e-12
        )
        ax.plot(
            curve["wing_loading"], curve["thrust_to_weight"],
            color="#064e3b" if is_nominal
            else sensitivity_cmap(normalization(cd0)),
            linewidth=3.0 if is_nominal else 1.45,
            alpha=1.0 if is_nominal else 0.72,
            label=(f"Nominal CD₀ = {cd0:.5f}"
                   if is_nominal else None),
            zorder=4 if is_nominal else 2,
        )
        if cd0_index in label_indices:
            label_row = curve.iloc[-1]
            ax.annotate(
                f"{cd0 / nominal_cd0:.0%}",
                (label_row["wing_loading"],
                 label_row["thrust_to_weight"]),
                xytext=(5, 0), textcoords="offset points",
                fontsize=8.5, color="#166534", va="center",
            )

    middle_ws = float(acceleration_family["wing_loading"].median())
    low_curve = acceleration_family[np.isclose(
        acceleration_family["cd_0"], sensitivity_cd0_values[0],
        rtol=1.0e-9, atol=1.0e-12,
    )].sort_values("wing_loading")
    high_curve = acceleration_family[np.isclose(
        acceleration_family["cd_0"], sensitivity_cd0_values[-1],
        rtol=1.0e-9, atol=1.0e-12,
    )].sort_values("wing_loading")
    low_y = np.interp(
        middle_ws, low_curve["wing_loading"], low_curve["thrust_to_weight"]
    )
    high_y = np.interp(
        middle_ws, high_curve["wing_loading"], high_curve["thrust_to_weight"]
    )
    ax.annotate(
        "Increasing CD₀",
        xy=(middle_ws, high_y), xytext=(middle_ws, low_y),
        ha="center", va="bottom", fontsize=9.5, color="#334155",
        arrowprops=dict(arrowstyle="->", color="#334155", linewidth=1.4),
    )

    ax.clear()
    cd0_region_handle = add_curve_family_region(
        ax, acceleration_family, "cd_0", sensitivity_cd0_values,
        "#15803d", "Acceleration T/W region (CD₀: 80–120%)",
    )
    set_feasible_sensitivity_xlim(
        ax, acceleration_family, feasible_ws_min, feasible_ws_max
    )
    ax.set_title(analysis_title("CD₀ Sensitivity Region"))
    ax.set_xlabel("Wing Loading, W/S [N/m²]")
    ax.set_ylabel("Required Thrust-to-Weight Ratio, T/W [-]")
    ax.text(
        0.01, 0.02,
        "Only CD₀ varies (80–120%); k and the vertical wing-loading "
        "limits remain constant.",
        transform=ax.transAxes, fontsize=8.8, color="#4d4d4d",
        va="bottom",
    )

    clean_axes(ax)
    ax.legend(handles=[cd0_region_handle], loc="upper right", frameon=False)
    save_plot("03_cd0_parameter_sensitivity")

    # ========================================================
    # k sensitivity with a moving gust-limit family
    # ========================================================
    k_sensitivity_path = os.path.join(
        output_dir, "jet_k_sensitivity_curves.csv"
    )
    if not os.path.exists(k_sensitivity_path):
        raise FileNotFoundError(
            "jet_k_sensitivity_curves.csv not found; rerun the C++ application."
        )
    k_sensitivity = pd.read_csv(k_sensitivity_path).dropna()
    required_k_columns = {
        "induced_drag_factor", "constraint_name", "wing_loading",
        "thrust_to_weight", "gust_wing_loading_limit",
    }
    if not required_k_columns.issubset(k_sensitivity.columns):
        raise RuntimeError("k sensitivity CSV schema is outdated.")

    k_values = np.sort(k_sensitivity["induced_drag_factor"].unique())
    if len(k_values) != 9:
        raise RuntimeError("k sensitivity must contain nine parameter levels.")

    fig, ax = plt.subplots(figsize=(10.5, 6.2))
    k_normalization = mpl.colors.Normalize(
        vmin=k_values.min(), vmax=k_values.max()
    )
    k_curve_cmap = mpl.colormaps["Blues"]
    gust_cmap = mpl.colormaps["Oranges"]
    gust_limit_family = []

    for k_index, induced_drag_factor in enumerate(k_values):
        curve = k_sensitivity[np.isclose(
            k_sensitivity["induced_drag_factor"], induced_drag_factor,
            rtol=1.0e-9, atol=1.0e-12,
        )].sort_values("wing_loading")
        is_nominal = np.isclose(
            induced_drag_factor, nominal_k,
            rtol=1.0e-9, atol=1.0e-12,
        )
        curve_color = (
            "#1e3a8a" if is_nominal
            else k_curve_cmap(k_normalization(induced_drag_factor))
        )
        ax.plot(
            curve["wing_loading"], curve["thrust_to_weight"],
            color=curve_color, linewidth=3.0 if is_nominal else 1.45,
            alpha=1.0 if is_nominal else 0.72,
            label=(f"Nominal k = {induced_drag_factor:.5f}"
                   if is_nominal else None),
            zorder=4 if is_nominal else 2,
        )

        gust_limit = float(curve["gust_wing_loading_limit"].iloc[0])
        gust_limit_family.append((
            k_index, induced_drag_factor, gust_limit, is_nominal
        ))
        ax.axvline(
            gust_limit,
            color="#9a3412" if is_nominal
            else gust_cmap(k_normalization(induced_drag_factor)),
            linewidth=2.2 if is_nominal else 0.85,
            alpha=0.95 if is_nominal else 0.34,
            linestyle="--" if is_nominal else "-",
            label=(f"Nominal gust limit = {gust_limit:.0f} N/m²"
                   if is_nominal else None),
            zorder=3,
        )

        if k_index in label_indices:
            label_row = curve.iloc[-1]
            ax.annotate(
                f"{induced_drag_factor / nominal_k:.0%}",
                (label_row["wing_loading"],
                 label_row["thrust_to_weight"]),
                xytext=(5, 0), textcoords="offset points",
                fontsize=8.5, color="#1d4ed8", va="center",
            )

    gust_limits = np.array([
        item[2] for item in gust_limit_family
    ], dtype=float)
    gust_span = float(gust_limits.max() - gust_limits.min())
    gust_padding = max(0.18 * gust_span, 2.0)
    gust_inset = ax.inset_axes([0.57, 0.57, 0.29, 0.29])
    for k_index, induced_drag_factor, gust_limit, is_nominal in (
            gust_limit_family):
        gust_inset.axvline(
            gust_limit,
            color="#9a3412" if is_nominal
            else gust_cmap(k_normalization(induced_drag_factor)),
            linewidth=2.5 if is_nominal else 1.25,
            linestyle="--" if is_nominal else "-",
            alpha=1.0 if is_nominal else 0.78,
        )
    selected_gust_items = [
        item for item in gust_limit_family if item[0] in label_indices
    ]
    gust_inset.set_xlim(
        gust_limits.min() - gust_padding,
        gust_limits.max() + gust_padding,
    )
    gust_inset.set_ylim(0.0, 1.0)
    gust_inset.set_yticks([])
    gust_inset.set_xticks([item[2] for item in selected_gust_items])
    gust_inset.set_xticklabels(
        [f"{item[1] / nominal_k:.0%}" for item in selected_gust_items],
        rotation=40, ha="right", fontsize=7.5,
    )
    gust_inset.set_title("Gust-limit shift", fontsize=9)
    gust_inset.set_xlabel("W/S [N/m²]", fontsize=8)
    gust_inset.grid(axis="x", alpha=0.18)
    gust_inset.spines["top"].set_visible(False)
    gust_inset.spines["right"].set_visible(False)

    low_k_curve = k_sensitivity[np.isclose(
        k_sensitivity["induced_drag_factor"], k_values[0],
        rtol=1.0e-9, atol=1.0e-12,
    )].sort_values("wing_loading")
    high_k_curve = k_sensitivity[np.isclose(
        k_sensitivity["induced_drag_factor"], k_values[-1],
        rtol=1.0e-9, atol=1.0e-12,
    )].sort_values("wing_loading")
    k_middle_ws = float(k_sensitivity["wing_loading"].median())
    low_k_y = np.interp(
        k_middle_ws, low_k_curve["wing_loading"],
        low_k_curve["thrust_to_weight"],
    )
    high_k_y = np.interp(
        k_middle_ws, high_k_curve["wing_loading"],
        high_k_curve["thrust_to_weight"],
    )
    ax.annotate(
        "Increasing k",
        xy=(k_middle_ws, high_k_y),
        xytext=(k_middle_ws, low_k_y),
        ha="center", va="bottom", fontsize=9.5, color="#334155",
        arrowprops=dict(arrowstyle="->", color="#334155", linewidth=1.4),
    )

    gust_inset.remove()
    plt.close(fig)
    fig, (ax, gust_ax) = plt.subplots(
        1, 2, figsize=(13.2, 5.8),
        gridspec_kw={"width_ratios": [1.65, 1.0]},
    )
    k_region_handle = add_curve_family_region(
        ax, k_sensitivity, "induced_drag_factor", k_values,
        "#2563eb", "Acceleration T/W region (k: 80–120%)",
    )
    set_feasible_sensitivity_xlim(
        ax, k_sensitivity, feasible_ws_min, feasible_ws_max
    )
    ax.set_title("Acceleration T/W region")
    ax.set_xlabel("Wing Loading, W/S [N/m²]")
    ax.set_ylabel("Required Thrust-to-Weight Ratio, T/W [-]")
    clean_axes(ax)
    ax.legend(handles=[k_region_handle], loc="upper right", frameon=False)

    k_percent = 100.0 * k_values / nominal_k
    nominal_index = int(np.argmin(np.abs(k_values - nominal_k)))
    nominal_gust = float(gust_limits[nominal_index])
    gust_delta = gust_limits - nominal_gust
    for scale in np.linspace(1.0, 0.125, 8):
        gust_ax.fill_between(
            k_percent, nominal_gust, nominal_gust + scale * gust_delta,
            color="#ea580c", alpha=0.12, linewidth=0.0,
        )
    gust_ax.plot(k_percent, gust_limits, color="#c2410c", linewidth=1.35)
    gust_ax.axhline(
        nominal_gust, color="#9a3412", linestyle="--", linewidth=1.0,
    )
    gust_ax.scatter(
        [100.0], [nominal_gust], marker="*", s=120,
        facecolor="#facc15", edgecolor="#111827", zorder=4,
    )
    gust_span = float(np.ptp(gust_limits))
    gust_padding = max(0.18 * gust_span, 0.005 * abs(nominal_gust), 1.0)
    gust_ax.set_ylim(
        float(gust_limits.min()) - gust_padding,
        float(gust_limits.max()) + gust_padding,
    )
    gust_ax.set_xlabel("k / nominal k [%]")
    gust_ax.xaxis.set_major_formatter(mpl.ticker.PercentFormatter(xmax=100.0))
    gust_ax.set_ylabel("Gust W/S limit [N/m²]")
    gust_ax.set_title("Gust-limit response")
    clean_axes(gust_ax)
    gust_ax.legend(
        handles=[Patch(
            facecolor="#ea580c", edgecolor="#c2410c", alpha=0.42,
            label="Gust W/S region (k: 80–120%)",
        )],
        loc="best", frameon=False,
    )

    fig.suptitle(analysis_title("k Sensitivity: Acceleration and Gust"))
    fig.text(
        0.5, 0.015,
        "Only k varies (80–120%); CD₀, mission, propulsion and all "
        "other aircraft parameters remain constant.",
        ha="center", fontsize=8.8, color="#4d4d4d",
    )
    fig.tight_layout(rect=(0.0, 0.04, 1.0, 0.94))
    save_plot("04_k_parameter_sensitivity")


    # ========================================================
    # Plot 5: Best W/S vs CD0
    # ========================================================
    fig, ax = plt.subplots(figsize=(9, 5.5))

    ax.plot(
        study["cd_0"],
        study["best_wing_loading"],
        marker="o",
        color="black",
        linewidth=2.2,
    )

    ax.set_title(analysis_title("Sensitivity of Optimum W/S to CD₀"))
    ax.set_xlabel("Zero-Lift Drag Coefficient, CD₀ [-]")
    ax.set_ylabel("Optimum Wing Loading, W/S [N/m²]")

    for _, row in study.iterrows():
        ax.annotate(
            f"{row['best_wing_loading']:.0f}",
            xy=(row["cd_0"], row["best_wing_loading"]),
            xytext=(0, 8),
            textcoords="offset points",
            ha="center",
            fontsize=9,
        )

    save_plot("05_optimum_ws_vs_cd0")

    # ============================================================
    # Plot 6: Range fuel fraction and L/D
    # ============================================================
    range_path = os.path.join(output_dir, "jet_range_fuel_fraction_constraint.csv")

    if os.path.exists(range_path):

        range_df = pd.read_csv(range_path)

        fig, ax1 = plt.subplots(figsize=(10, 6))

        ax1.plot(
            range_df["wing_loading"],
            range_df["required_fuel_fraction"],
            marker="o",
            linewidth=2.2,
            color="black",
            label="Required fuel fraction"
        )

        ax1.set_xlabel("Wing Loading, W/S [N/m²]")
        ax1.set_ylabel("Required Fuel Fraction [-]")

        ax1.grid(True, alpha=0.35)

        ax2 = ax1.twinx()

        ax2.plot(
            range_df["wing_loading"],
            range_df["lift_to_drag"],
            marker="s",
            linewidth=2.0,
            linestyle="--",
            color="gray",
            label="L/D"
        )

        ax2.set_ylabel("Lift-to-Drag Ratio, L/D [-]")

        plt.title(analysis_title("Range Constraint: Fuel Fraction and L/D"))

        lines_1, labels_1 = ax1.get_legend_handles_labels()
        lines_2, labels_2 = ax2.get_legend_handles_labels()

        ax1.legend(
            lines_1 + lines_2,
            labels_1 + labels_2,
            loc="center right",
            frameon=True
        )

        save_plot("06_range_fuel_fraction_and_ld")

    # ============================================================
    # Lecture-style two-family CD0-k aerodynamic carpet
    # ============================================================
    aerodynamic_carpet_path = os.path.join(
        output_dir, "jet_cd0_k_carpet.csv"
    )
    if os.path.exists(aerodynamic_carpet_path):
        aero_carpet = pd.read_csv(aerodynamic_carpet_path).dropna()
        cd0_grid_values = np.sort(aero_carpet["cd_0"].unique())
        k_grid_values = np.sort(
            aero_carpet["induced_drag_factor"].unique()
        )

        expected_points = len(cd0_grid_values) * len(k_grid_values)
        if (len(cd0_grid_values) != 9 or len(k_grid_values) != 9 or
                len(aero_carpet) != expected_points):
            raise RuntimeError(
                "Jet CD0-k carpet must be a complete 9x9 grid."
            )
        if (np.ptp(aero_carpet["best_wing_loading"].to_numpy()) < 1.0e-9 and
                np.ptp(aero_carpet["best_thrust_to_weight"].to_numpy()) <
                1.0e-12):
            raise RuntimeError(
                "Jet CD0-k carpet contains no aerodynamic response. Rerun "
                "the updated C++ application with --with-studies; the CSV "
                "was generated before operating-polar scaling was fixed."
            )

        fig, ax = plt.subplots(figsize=(10.0, 7.0))
        cd0_color = "#10b981"
        k_color = "#ef4444"

        # Green family: CD0 remains constant while k varies.
        label_indices = {0, 4, 8}
        for cd0_index, cd0 in enumerate(cd0_grid_values):
            family = aero_carpet[
                aero_carpet["cd_0"] == cd0
            ].sort_values("induced_drag_factor")
            ax.plot(
                family["best_wing_loading"],
                family["best_thrust_to_weight"],
                color=cd0_color,
                linewidth=2.4 if cd0_index in label_indices else 1.25,
                alpha=1.0 if cd0_index in label_indices else 0.42,
            )
            if cd0_index in label_indices:
                label_row = family.iloc[-1]
                ax.annotate(
                    f"CD₀ = {cd0:.4f}",
                    (label_row["best_wing_loading"],
                     label_row["best_thrust_to_weight"]),
                    xytext=(7, 5), textcoords="offset points",
                    fontsize=8.5, color="#047857",
                )

        # Red family: k remains constant while CD0 varies.
        for k_index, induced_drag_factor in enumerate(k_grid_values):
            family = aero_carpet[
                aero_carpet["induced_drag_factor"] == induced_drag_factor
            ].sort_values("cd_0")
            ax.plot(
                family["best_wing_loading"],
                family["best_thrust_to_weight"],
                color=k_color,
                linewidth=2.2 if k_index in label_indices else 1.2,
                alpha=1.0 if k_index in label_indices else 0.38,
                marker="o", markersize=4.2,
                markerfacecolor="#0f172a", markeredgecolor="#0f172a",
            )
            if k_index in label_indices:
                label_row = family.iloc[-1]
                ax.annotate(
                    f"k = {induced_drag_factor:.4f}",
                    (label_row["best_wing_loading"],
                     label_row["best_thrust_to_weight"]),
                    xytext=(7, -7), textcoords="offset points",
                    fontsize=8.5, color="#b91c1c",
                )

        required_columns = {
            "is_baseline", "active_constraint_name",
            "active_constraint_value", "second_constraint_name",
            "second_constraint_value", "constraint_margin",
        }
        if not required_columns.issubset(aero_carpet.columns):
            raise RuntimeError(
                "Rerun the C++ application: carpet CSV schema is outdated."
            )
        baseline = aero_carpet[aero_carpet["is_baseline"] == 1]
        if len(baseline) != 1:
            raise RuntimeError(
                "Jet CD0-k carpet must contain one nominal polar point."
            )
        active_handles = []
        active_names = sorted(
            aero_carpet["active_constraint_name"].astype(str).unique()
        )
        for active_name in active_names:
            active_label = active_name.replace("jet_", "").replace(
                "_constraint", ""
            ).replace("_", " ").title()
            active_points = aero_carpet[
                aero_carpet["active_constraint_name"].astype(str) ==
                active_name
            ]
            active_marker = ax.scatter(
                active_points["best_wing_loading"],
                active_points["best_thrust_to_weight"],
                s=34, color=color_map.get(active_label, "#475569"),
                edgecolor="white", linewidth=0.45, alpha=0.92, zorder=7,
                label=f"Active: {active_label}",
            )
            active_handles.append(active_marker)

        baseline_row = baseline.iloc[0]
        baseline_active = str(baseline_row["active_constraint_name"]).replace(
            "jet_", ""
        ).replace("_constraint", "").replace("_", " ").title()
        ax.scatter(
            baseline_row["best_wing_loading"],
            baseline_row["best_thrust_to_weight"],
            marker="*", s=180, color="#fbbf24", edgecolor="#0f172a",
            linewidth=0.9, label="Imported nominal polar", zorder=9,
        )
        ax.annotate(
            f"Nominal polar\nActive: {baseline_active}",
            (baseline_row["best_wing_loading"],
             baseline_row["best_thrust_to_weight"]),
            xytext=(0.98, 0.13), textcoords="axes fraction",
            ha="right", va="bottom", fontsize=8.5,
            color="#334155",
            arrowprops=dict(
                arrowstyle="->", color="#64748b", linewidth=1.0
            ),
            bbox=dict(boxstyle="round,pad=0.25", facecolor="white",
                      edgecolor="#cbd5e1", alpha=0.92),
        )

        infeasible = aero_carpet[aero_carpet["range_feasible"] == 0]
        if not infeasible.empty:
            ax.scatter(
                infeasible["best_wing_loading"],
                infeasible["best_thrust_to_weight"],
                marker="x", s=65, linewidth=1.8, color="#D55E00",
                label="Range infeasible", zorder=8,
            )

        family_handles = [
            Line2D([0], [0], color=cd0_color, linewidth=2.4,
                   label="Constant CD₀"),
            Line2D([0], [0], color=k_color, linewidth=2.2,
                   marker="o", markerfacecolor="#0f172a",
                   markeredgecolor="#0f172a", label="Constant k"),
            Line2D([0], [0], color="#fbbf24", marker="*",
                   markeredgecolor="#0f172a", linestyle="None",
                   markersize=11, label="Imported nominal polar"),
        ]
        if not infeasible.empty:
            family_handles.append(Line2D(
                [0], [0], color="#D55E00", marker="x", linestyle="None",
                markersize=7, label="Range infeasible",
            ))

        ax.clear()
        family_handles = draw_design_map(
            ax, aero_carpet, "cd_0", "induced_drag_factor",
            "Zero-lift drag coefficient, CD₀ [-]",
            "Induced drag factor, k [-]",
        )
        ax.set_title(analysis_title("CD₀–k Aerodynamic Design Map"))
        ax.legend(handles=family_handles, frameon=False, loc="best")
        ax.text(
            0.01, 0.02,
            "Pastel areas show the governing constraint; solid blue "
            "contours show optimum W/S and dashed red contours show T/W.",
            transform=ax.transAxes, fontsize=8.8, color="#4d4d4d",
            va="bottom",
        )
        save_plot("05_cd0_k_design_map")

    # ============================================================
    # Additional case-specific, two-parameter classical carpets
    # ============================================================
    additional_carpet_studies = [
        (
            "jet_acceleration_takeoff_distance_carpet.csv",
            "acceleration_severity_scale", "takeoff_distance_m",
            "Mission acceleration demand scale [-]",
            "Take-off distance [m]",
            lambda value: f"Mission demand = {value:.0%}",
            lambda value: f"s_TO = {value:.0f} m",
            "Mission–Runway Design Map",
            "06_mission_runway_design_map",
        ),
    ]
    for (
            csv_name, parameter_a, parameter_b, parameter_a_label,
            parameter_b_label, parameter_a_formatter,
            parameter_b_formatter, title, plot_name,
    ) in additional_carpet_studies:
        carpet_path = os.path.join(output_dir, csv_name)
        if not os.path.exists(carpet_path):
            raise FileNotFoundError(
                f"{csv_name} not found; rerun the C++ application."
            )
        plot_two_parameter_classical_carpet(
            pd.read_csv(carpet_path).dropna(),
            parameter_a, parameter_b, parameter_a_label,
            parameter_b_label, parameter_a_formatter,
            parameter_b_formatter, title, plot_name,
        )

print()
print("Plot generation completed.")
print(
    f"Aircraft point: W/S = {aircraft_ws:.0f} N/m², "
    f"{y_symbol} = {aircraft_tw:.4f}"
)
print(
    f"Best design point: W/S = {best_ws:.0f} N/m², "
    f"{y_symbol} = {best_tw:.4f}"
)
print(f"Plots saved to: {save_dir}")
