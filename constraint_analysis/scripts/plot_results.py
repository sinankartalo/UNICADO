import os
import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
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

if not selected_case_id:
    raise FileNotFoundError(
        "No case selected and output/latest_case.txt is missing. "
        "Run the C++ application or pass CASE_ID explicitly."
    )

output_dir = os.path.join(output_root, selected_case_id)
save_dir = os.path.join(plots_root, selected_case_id)

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
        layers=120, draw_edges=True, edge_alpha=0.08,
        center_alpha=0.48):
    """Shade a curve band with one continuous centre-dark alpha gradient."""
    x = np.asarray(x, dtype=float)
    nominal_y = np.asarray(nominal_y, dtype=float)

    # Adjacent, non-overlapping slices avoid the visible nested bands produced
    # by repeatedly painting smaller regions on top of one another. Every
    # slice uses the same RGB colour; only opacity changes continuously.
    normalized_edges = np.linspace(-1.0, 1.0, layers + 1)
    for normalized_low, normalized_high in zip(
            normalized_edges[:-1], normalized_edges[1:]):
        def fractional_offset(normalized):
            return (
                normalized * lower_fraction if normalized < 0.0
                else normalized * upper_fraction
            )

        lower_y = nominal_y * (
            1.0 + fractional_offset(normalized_low)
        )
        upper_y = nominal_y * (
            1.0 + fractional_offset(normalized_high)
        )
        normalized_mid = 0.5 * (normalized_low + normalized_high)
        alpha = edge_alpha + (
            center_alpha - edge_alpha
        ) * (1.0 - abs(normalized_mid))
        ax.fill_between(
            x,
            lower_y,
            upper_y,
            color=color,
            alpha=alpha,
            linewidth=0.0,
            antialiased=False,
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
        ax, nominal_x, color, lower_fraction, upper_fraction, layers=120,
        draw_edges=True, edge_alpha=0.08, center_alpha=0.48):
    """Shade a vertical band with one continuous centre-dark gradient."""
    normalized_edges = np.linspace(-1.0, 1.0, layers + 1)
    for normalized_low, normalized_high in zip(
            normalized_edges[:-1], normalized_edges[1:]):
        def fractional_offset(normalized):
            return (
                normalized * lower_fraction if normalized < 0.0
                else normalized * upper_fraction
            )

        lower_x = nominal_x * (
            1.0 + fractional_offset(normalized_low)
        )
        upper_x = nominal_x * (
            1.0 + fractional_offset(normalized_high)
        )
        normalized_mid = 0.5 * (normalized_low + normalized_high)
        alpha = edge_alpha + (
            center_alpha - edge_alpha
        ) * (1.0 - abs(normalized_mid))
        ax.axvspan(
            lower_x,
            upper_x,
            color=color,
            alpha=alpha,
            linewidth=0.0,
            antialiased=False,
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


# ============================================================
# Load constraint data
# ============================================================
metadata_path = os.path.join(output_dir, "analysis_metadata.csv")
metadata_row = None
if os.path.exists(metadata_path):
    metadata = pd.read_csv(metadata_path)
    metadata_row = metadata.iloc[0]
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
    "JET_PW1127GJM_BASELINE": "Jet — PW1127G-JM Real Engine: Baseline Case",
    "PROPELLER_UNICADO_BASELINE": "Propeller — UNICADO: Baseline Case",
}
analysis_label = case_labels.get(
    case_id,
    f"{'Propeller' if propeller_mode else 'Jet'} — {case_id}",
)


def constraint_is_active(name):
    """Read optional C++ activation metadata; old outputs default to active."""
    column = f"{name}_active"
    if metadata_row is None or column not in metadata_row.index:
        return True
    value = metadata_row[column]
    if isinstance(value, str):
        return value.strip().lower() in ("true", "1", "yes")
    return bool(value)


def selected_input_summary(row):
    """Format the exact C++ analysis conditions for the chart sidebar."""
    required = {
        "condition_source", "takeoff_altitude_m", "takeoff_runway_m",
        "landing_altitude_m", "landing_runway_m", "stall_speed_limit_ms",
        "max_mach_altitude_m", "max_mach", "acceleration_altitude_m",
        "acceleration_speed_ms", "acceleration_ms2", "cruise_altitude_m",
        "cruise_speed_ms", "climb_altitude_m", "climb_speed_ms",
        "climb_roc_ms", "gust_altitude_m", "gust_speed_ms",
        "turn_altitude_m", "turn_speed_ms", "turn_load_factor",
        "takeoff_beta", "landing_beta", "max_mach_beta",
        "acceleration_beta", "cruise_beta", "climb_beta", "gust_beta",
        "turn_beta",
    }
    if row is None or not required.issubset(row.index):
        return None

    def km(value):
        return float(value) / 1000.0

    lines = [f"Mode: {str(row['condition_source']).title()}"]
    if constraint_is_active("takeoff"):
        lines.append(
            f"Takeoff: {row['takeoff_runway_m']:.0f} m runway, "
            f"h={km(row['takeoff_altitude_m']):.1f} km")
    if constraint_is_active("landing"):
        lines.append(
            f"Landing: {row['landing_runway_m']:.0f} m roll, "
            f"h={km(row['landing_altitude_m']):.1f} km")
    if constraint_is_active("stall"):
        lines.append(f"Stall: V≤{row['stall_speed_limit_ms']:.0f} m/s")
    if constraint_is_active("max_mach"):
        lines.append(
            f"Max Mach: M={row['max_mach']:.2f}, "
            f"h={km(row['max_mach_altitude_m']):.1f} km")
    if constraint_is_active("acceleration"):
        lines.append(
            f"Acceleration: h={km(row['acceleration_altitude_m']):.1f} km, "
            f"V={row['acceleration_speed_ms']:.0f} m/s, "
            f"a={row['acceleration_ms2']:.2f} m/s²")
    if constraint_is_active("cruise"):
        lines.append(
            f"Cruise: h={km(row['cruise_altitude_m']):.1f} km, "
            f"V={row['cruise_speed_ms']:.0f} m/s")
    if constraint_is_active("climb"):
        lines.append(
            f"Climb: h={km(row['climb_altitude_m']):.1f} km, "
            f"V={row['climb_speed_ms']:.0f} m/s, "
            f"ROC={row['climb_roc_ms']:.1f} m/s")
    if constraint_is_active("gust"):
        lines.append(
            f"Gust: h={km(row['gust_altitude_m']):.1f} km, "
            f"V={row['gust_speed_ms']:.0f} m/s")
    if constraint_is_active("turn"):
        lines.append(
            f"Turn: h={km(row['turn_altitude_m']):.1f} km, "
            f"V={row['turn_speed_ms']:.0f} m/s, "
            f"n={row['turn_load_factor']:.1f}")
    return "\n".join(lines)


selected_inputs_text = selected_input_summary(metadata_row)


def add_selected_inputs_sidebar(fig):
    if not selected_inputs_text:
        return
    fig.text(
        0.795, 0.43,
        "SELECTED ANALYSIS INPUTS\n" + selected_inputs_text,
        ha="left", va="top", fontsize=8.3, linespacing=1.35,
        color="#263238",
        bbox=dict(
            boxstyle="round,pad=0.55",
            facecolor="#f7f9fb",
            edgecolor="#c8d1da",
            linewidth=0.9,
        ),
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

# Keep only the compact review package. Removed plots are deliberately absent
# from this allowlist, so stale PNGs are deleted on the next plotting run.
plot_allowlist = {
    "01_design_matching_chart",
    "02_governing_constraint_envelope",
    "03_design_point_margins",
    "04_requirement_tradeoff",
    "05_tolerance_robustness",
}
for existing_plot in os.listdir(save_dir):
    if not existing_plot.endswith(".png"):
        continue
    if os.path.splitext(existing_plot)[0] not in plot_allowlist:
        os.remove(os.path.join(save_dir, existing_plot))


def analysis_title(title):
    return f"{analysis_label}: {title}"


def plot_requirement_tradeoff(csv_path):
    study = pd.read_csv(csv_path).dropna()
    secondary_parameter = (
        "climb_rate_ms" if propeller_mode else "acceleration_ms2"
    )
    required = {
        secondary_parameter, "takeoff_distance_m", "best_wing_loading",
        "best_required_loading", "is_baseline", "active_constraint_name",
    }
    if not required.issubset(study.columns):
        missing = sorted(required.difference(study.columns))
        raise RuntimeError(
            "Requirement trade-off CSV is incomplete. Missing: "
            + ", ".join(missing)
        )

    baseline = study[
        pd.to_numeric(study["is_baseline"], errors="coerce").fillna(0) == 1
    ]
    if len(baseline) != 1:
        raise RuntimeError(
            "Requirement trade-off study must contain one nominal point."
        )
    baseline = baseline.iloc[0]

    available_levels = np.sort(study[secondary_parameter].unique())
    nominal_value = float(baseline[secondary_parameter])
    nominal_level = available_levels[
        np.argmin(np.abs(available_levels - nominal_value))
    ]
    requested_levels = [available_levels[0], nominal_level, available_levels[-1]]
    level_names = ["Low", "Nominal", "High"]
    colors = ["#16a34a", "#2563eb", "#dc2626"]

    fig, (ax_loading, ax_ws) = plt.subplots(
        1, 2, figsize=(12.2, 5.8), sharex=True,
    )
    for level, level_name, color in zip(
            requested_levels, level_names, colors):
        points = study[np.isclose(study[secondary_parameter], level)].sort_values(
            "takeoff_distance_m"
        )
        if points.empty:
            continue
        if propeller_mode:
            requirement = f"ROC = {level:.1f} m/s"
        else:
            requirement = f"a = {level:.2f} m/s²"
        label = f"{level_name}: {requirement}"
        width = 2.8 if level_name == "Nominal" else 1.8
        ax_loading.plot(
            points["takeoff_distance_m"], points["best_required_loading"],
            color=color, linewidth=width, marker="o", markersize=3.8,
            label=label,
        )
        ax_ws.plot(
            points["takeoff_distance_m"], points["best_wing_loading"],
            color=color, linewidth=width, marker="o", markersize=3.8,
            label=label,
        )

    nominal_x = float(baseline["takeoff_distance_m"])
    nominal_loading = float(baseline["best_required_loading"])
    nominal_ws = float(baseline["best_wing_loading"])
    for ax, nominal_y in (
            (ax_loading, nominal_loading), (ax_ws, nominal_ws)):
        ax.scatter(
            nominal_x, nominal_y, marker="*", s=190, color="#fbbf24",
            edgecolor="#111827", linewidth=0.9, zorder=6,
        )

    governing = str(baseline["active_constraint_name"]).replace(
        "propeller_", ""
    ).replace("jet_", "").replace("_constraint", "").replace("_", " ")
    ax_loading.annotate(
        f"Nominal design\n{y_symbol} = {nominal_loading:.3f}\n"
        f"Control: {governing.title()}",
        (nominal_x, nominal_loading), xytext=(14, 14),
        textcoords="offset points", fontsize=8.8,
        arrowprops=dict(arrowstyle="->", color="#64748b"),
        bbox=dict(boxstyle="round,pad=0.3", facecolor="white",
                  edgecolor="#cbd5e1", alpha=0.96),
    )
    ax_ws.annotate(
        f"Nominal W/S = {nominal_ws:.0f} N/m²",
        (nominal_x, nominal_ws), xytext=(14, 14),
        textcoords="offset points", fontsize=8.8,
        arrowprops=dict(arrowstyle="->", color="#64748b"),
        bbox=dict(boxstyle="round,pad=0.3", facecolor="white",
                  edgecolor="#cbd5e1", alpha=0.96),
    )

    ax_loading.set_title("Required propulsion loading")
    ax_loading.set_ylabel(y_axis_label)
    ax_ws.set_title("Selected wing loading")
    ax_ws.set_ylabel("Selected Wing Loading, W/S [N/m²]")
    for ax in (ax_loading, ax_ws):
        ax.set_xlabel("Take-off ground-roll requirement [m]")
        clean_axes(ax)
    ax_loading.legend(frameon=False, fontsize=8.7)
    fig.suptitle(analysis_title("Requirement Trade-Off"), fontsize=15)
    fig.text(
        0.5, 0.015,
        "Shorter runway and higher climb/acceleration requirements are more "
        "demanding. The star marks the nominal design requirement.",
        ha="center", fontsize=8.8, color="#475569",
    )
    fig.tight_layout(rect=(0.0, 0.055, 1.0, 0.94))
    save_plot("04_requirement_tradeoff", tight=False)


if propeller_mode:
    constraint_files = {
        "Acceleration": "propeller_acceleration_constraint.csv",
        "Takeoff": "propeller_takeoff_constraint.csv",
        "Turn": "propeller_turn_constraint.csv",
    }
    y_axis_label = "Required Shaft Power Loading, P/W [W/N]"
    y_symbol = "P/W"
    design_value_column = "shaft_power_to_weight"
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

activation_by_label = {
    "Acceleration": "acceleration",
    "Max Mach": "max_mach",
    "Takeoff": "takeoff",
    "Turn": "turn",
}
constraint_files = {
    label: filename for label, filename in constraint_files.items()
    if constraint_is_active(activation_by_label[label])
}

propulsion_prefix = "propeller" if propeller_mode else "jet"
for regime in ("subsonic", "transonic", "supersonic"):
    for segment in ("climb", "cruise"):
        if not constraint_is_active(segment):
            continue
        filename = (
            f"{propulsion_prefix}_{regime}_{segment}_constraint.csv"
        )
        if os.path.exists(os.path.join(output_dir, filename)):
            label = f"{regime.title()} {segment.title()}"
            constraint_files[label] = filename

for required_segment in ("Climb", "Cruise"):
    if not constraint_is_active(required_segment.lower()):
        continue
    if not any(name.endswith(required_segment) for name in constraint_files):
        raise RuntimeError(
            f"No mission-supported {required_segment.lower()} constraint "
            "curve was generated. Inspect mission_mach_regime_coverage.csv."
        )

for name in constraint_files:
    if name.endswith("Climb") or name.endswith("Cruise"):
        constraint_tolerances.setdefault(name, (0.10, 0.10))

constraints = {}

for name, filename in constraint_files.items():
    df = load_xy_csv(filename)
    if df is not None:
        constraints[name] = df

if not constraints:
    raise RuntimeError(
        "The current analysis did not produce any active constraint CSV files. "
        "Enable at least one curve constraint in the XML and rerun the C++ application."
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

best_constraint_values = {
    name: float(np.interp(
        best_ws,
        curve["wing_loading"].to_numpy(dtype=float),
        curve["thrust_to_weight"].to_numpy(dtype=float),
    ))
    for name, curve in constraints.items()
}
best_constraint_ranking = sorted(
    best_constraint_values, key=best_constraint_values.get, reverse=True,
)
best_governing_name = best_constraint_ranking[0]
best_runner_up_name = (
    best_constraint_ranking[1]
    if len(best_constraint_ranking) > 1 else "None"
)
best_annotation += (
    f"\nPower control: {best_governing_name}"
    f"\nRunner-up: {best_runner_up_name}"
)


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

wing_control_candidates = []
for control_name, limit in (
        ("Landing", landing_ws_limit), ("Stall", stall_ws_limit),
        ("Gust", gust_ws_limit)):
    if limit is not None and limit > 0.0:
        wing_control_candidates.append(
            (abs(best_ws - limit) / limit, control_name)
        )
if wing_control_candidates:
    best_annotation += (
        f"\nWing-loading control: {min(wing_control_candidates)[1]}"
    )


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
        color=color_map.get(name, "#475569"),
        alpha=0.88,
        linewidth=1.8,
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
    analysis_title("Design Matching Chart"),
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
    labelspacing=0.48,
    fontsize=8.7,
)

add_selected_inputs_sidebar(fig)
fig.subplots_adjust(right=0.77)
save_plot("01_design_matching_chart")


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
    0.42 * point_x_span,
    0.12 * max(point_x_center, 1.0),
    375.0,
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
tolerance_zoom_y_min = max(0.0, zoom_y_low - 0.18 * zoom_y_span)
tolerance_zoom_y_max = zoom_y_high + 0.20 * zoom_y_span

# Conservative design point: upper performance demand bands together with
# tightened vertical limits. This converts the tolerance picture into a clear
# robustness decision rather than showing uncertainty bands alone.
robust_ws_min = (
    gust_ws_limit * (1.0 + constraint_tolerances["Gust"][1])
    if gust_ws_limit is not None else feasible_ws_min
)
robust_upper_limits = []
if landing_ws_limit is not None:
    robust_upper_limits.append(
        landing_ws_limit * (1.0 - constraint_tolerances["Landing"][0])
    )
if stall_ws_limit is not None:
    robust_upper_limits.append(
        stall_ws_limit * (1.0 - constraint_tolerances["Stall speed"][0])
    )
robust_ws_max = (
    min(robust_upper_limits) if robust_upper_limits else feasible_ws_max
)
robust_grid = envelope["wing_loading"].to_numpy(dtype=float)
robust_curve_values = np.vstack([
    np.interp(
        robust_grid,
        curve["wing_loading"].to_numpy(dtype=float),
        curve["thrust_to_weight"].to_numpy(dtype=float),
    ) * (1.0 + constraint_tolerances[name][1])
    for name, curve in constraints.items()
])
robust_envelope = np.max(robust_curve_values, axis=0)
robust_mask = (
    (robust_grid >= robust_ws_min) & (robust_grid <= robust_ws_max)
)
robust_design_available = np.any(robust_mask)
if robust_design_available:
    robust_indices = np.flatnonzero(robust_mask)
    robust_index = robust_indices[np.argmin(robust_envelope[robust_mask])]
    robust_best_ws = float(robust_grid[robust_index])
    robust_best_y = float(robust_envelope[robust_index])
    robust_governing_index = int(np.argmax(
        robust_curve_values[:, robust_index]
    ))
    robust_governing_name = list(constraints)[robust_governing_index]
    robust_loading_shift_percent = 100.0 * (robust_best_y / best_tw - 1.0)
    tolerance_zoom_x_min = max(
        tolerance_plot_min,
        min(tolerance_zoom_x_min, robust_best_ws - 0.08 * point_x_center),
    )
    tolerance_zoom_x_max = min(
        tolerance_plot_max,
        max(tolerance_zoom_x_max, robust_best_ws + 0.08 * point_x_center),
    )
    tolerance_zoom_y_max = max(
        tolerance_zoom_y_max, 1.12 * robust_best_y,
    )

# Always show the complete stall tolerance region. The previous point-centred
# zoom could crop one edge of this upper W/S boundary.
if stall_ws_limit is not None:
    stall_lower_fraction, stall_upper_fraction = constraint_tolerances[
        "Stall speed"
    ]
    stall_band_low = stall_ws_limit * (1.0 - stall_lower_fraction)
    stall_band_high = stall_ws_limit * (1.0 + stall_upper_fraction)
    stall_band_padding = max(
        0.25 * (stall_band_high - stall_band_low),
        0.025 * stall_ws_limit,
        150.0,
    )
    tolerance_zoom_x_min = max(
        tolerance_plot_min,
        min(tolerance_zoom_x_min, stall_band_low - stall_band_padding),
    )
    tolerance_zoom_x_max = min(
        tolerance_plot_max,
        max(tolerance_zoom_x_max, stall_band_high + stall_band_padding),
    )

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
        layers=120,
        draw_edges=False,
        edge_alpha=0.08,
        center_alpha=0.48,
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
if robust_design_available:
    ax.scatter(
        robust_best_ws, robust_best_y,
        s=92, marker="D", color="#7c3aed", edgecolor="white",
        linewidth=1.1, zorder=8, label="Robust design point",
    )
    ax.annotate(
        f"Robust design\nW/S = {robust_best_ws:.0f} N/m²\n"
        f"{y_symbol} = {robust_best_y:.3f}\n"
        f"Control: {robust_governing_name}\n"
        f"Loading increase: {robust_loading_shift_percent:+.1f}%",
        (robust_best_ws, robust_best_y),
        xytext=(12, 16), textcoords="offset points", fontsize=8.8,
        color="#4c1d95",
        arrowprops=dict(arrowstyle="->", color="#7c3aed", linewidth=1.0),
        bbox=dict(boxstyle="round,pad=0.3", facecolor="white",
                  edgecolor="#c4b5fd", alpha=0.96), zorder=9,
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
        layers=120,
        draw_edges=False,
        edge_alpha=0.08,
        center_alpha=0.48,
    )
    tolerance_constraint_handles.append(Patch(
        facecolor=color, edgecolor="none", alpha=0.35,
        label=f"{label} relaxed region ({limit:.0f})",
    ))

ax.set_title(
    analysis_title("Tolerance and Robust Design"),
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
    labelspacing=0.48,
    fontsize=8.7,
)

add_selected_inputs_sidebar(fig)
fig.subplots_adjust(right=0.77)
save_plot("05_tolerance_robustness")


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

ax.set_title(analysis_title("Governing Constraint Envelope"))
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
save_plot("02_governing_constraint_envelope")


# ============================================================
# Plot 3: Design-point constraint margins
# ============================================================
requirements = best_constraint_values
ordered = sorted(requirements.items(), key=lambda item: item[1])
names = [item[0] for item in ordered]
margin_percent = np.asarray([
    100.0 * (best_tw - required) / best_tw
    if best_tw > 0.0 else np.nan
    for _, required in ordered
])


def margin_color(value):
    if value < -0.05:
        return "#991b1b"
    if value <= 5.0:
        return "#dc2626"
    if value <= 15.0:
        return "#f59e0b"
    return "#16a34a"


fig, ax = plt.subplots(figsize=(10.5, 6.4))
bars = ax.barh(
    names, margin_percent,
    color=[margin_color(value) for value in margin_percent],
    alpha=0.9, height=0.66,
)
ax.axvline(0.0, color="#111827", linewidth=1.4)
for bar, value in zip(bars, margin_percent):
    ax.text(
        value + (0.7 if value >= 0.0 else -0.7),
        bar.get_y() + 0.5 * bar.get_height(),
        f"{value:+.1f}%",
        va="center", ha="left" if value >= 0.0 else "right",
        fontsize=9.0, color="#334155",
    )
ax.set_title(analysis_title(
    f"Design-Point Margins — Governing: {best_governing_name}"
))
ax.set_xlabel("Available margin at selected design [%]")
ax.set_ylabel("Performance requirement")
finite_margins = margin_percent[np.isfinite(margin_percent)]
margin_left = min(-5.0, float(np.min(finite_margins)) - 4.0)
margin_right = max(20.0, float(np.max(finite_margins)) + 8.0)
ax.set_xlim(margin_left, margin_right)
clean_axes(ax)
ax.text(
    0.99, 0.02,
    "0% = governing boundary | negative = infeasible | larger = more reserve",
    transform=ax.transAxes, ha="right", va="bottom",
    fontsize=8.8, color="#475569",
)
fig.tight_layout()
save_plot("03_design_point_margins")


requirement_study_path = os.path.join(
    output_dir,
    "propeller_performance_carpet.csv"
    if propeller_mode else "jet_performance_carpet.csv",
)
if os.path.exists(requirement_study_path):
    plot_requirement_tradeoff(requirement_study_path)


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
print(f"Governing performance constraint: {best_governing_name}")
print(f"Decision plots saved to: {save_dir}")
