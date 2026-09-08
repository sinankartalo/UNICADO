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
generate_jet_design_maps = False


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

    return "\n".join((
        f"Mode: {str(row['condition_source']).title()}",
        (f"Takeoff: {row['takeoff_runway_m']:.0f} m runway, "
         f"h={km(row['takeoff_altitude_m']):.1f} km"),
        (f"Landing: {row['landing_runway_m']:.0f} m roll, "
         f"h={km(row['landing_altitude_m']):.1f} km"),
        f"Stall: V≤{row['stall_speed_limit_ms']:.0f} m/s",
        (f"Max Mach: M={row['max_mach']:.2f}, "
         f"h={km(row['max_mach_altitude_m']):.1f} km"),
        (f"Acceleration: h={km(row['acceleration_altitude_m']):.1f} km, "
         f"V={row['acceleration_speed_ms']:.0f} m/s, "
         f"a={row['acceleration_ms2']:.2f} m/s²"),
        (f"Cruise: h={km(row['cruise_altitude_m']):.1f} km, "
         f"V={row['cruise_speed_ms']:.0f} m/s"),
        (f"Climb: h={km(row['climb_altitude_m']):.1f} km, "
         f"V={row['climb_speed_ms']:.0f} m/s, "
         f"ROC={row['climb_roc_ms']:.1f} m/s"),
        (f"Gust: h={km(row['gust_altitude_m']):.1f} km, "
         f"V={row['gust_speed_ms']:.0f} m/s"),
        (f"Turn: h={km(row['turn_altitude_m']):.1f} km, "
         f"V={row['turn_speed_ms']:.0f} m/s, "
         f"n={row['turn_load_factor']:.1f}"),
        (f"β: TO={row['takeoff_beta']:.2f}, LDG={row['landing_beta']:.2f}, "
         f"ACC={row['acceleration_beta']:.2f}, CR={row['cruise_beta']:.2f}"),
        (f"β: CL={row['climb_beta']:.2f}, Gust={row['gust_beta']:.2f}, "
         f"Turn={row['turn_beta']:.2f}, MMO={row['max_mach_beta']:.2f}"),
    ))


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

# The review package contains five core plots plus the optional requirement
# design map generated by --with-studies. Each answers a distinct question.
plot_allowlist = {
    "01_design_matching_chart",
    "02_governing_constraint_envelope",
    "03_design_point_margins",
    "04_performance_requirement_design_map",
    "05_tolerance_robustness",
    "06_mission_verification",
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
        "propeller_", ""
    ).replace(
        "_constraint", ""
    ).replace("_", " ").title()


def plot_performance_carpet(
        carpet, parameter_a, parameter_b, parameter_a_label,
        parameter_b_label, parameter_a_formatter, parameter_b_formatter):
    """Map user requirements directly to the selected aircraft design."""
    required = {
        parameter_a, parameter_b, "best_wing_loading",
        "best_required_loading", "is_baseline",
        "active_constraint_name", "second_constraint_name",
        "constraint_margin",
    }
    if not required.issubset(carpet.columns):
        missing = sorted(required.difference(carpet.columns))
        raise RuntimeError(
            "Performance carpet CSV is outdated; rerun C++ with "
            f"--with-studies. Missing: {', '.join(missing)}"
        )
    values_a = np.sort(carpet[parameter_a].unique())
    values_b = np.sort(carpet[parameter_b].unique())
    if len(values_a) != 9 or len(values_b) != 9 or len(carpet) != 81:
        raise RuntimeError("Performance carpet must contain a complete 9x9 grid.")
    def grid(column):
        return carpet.pivot(
            index=parameter_a, columns=parameter_b, values=column,
        ).reindex(index=values_a, columns=values_b).to_numpy()

    loading_grid = grid("best_required_loading").astype(float)
    ws_grid = grid("best_wing_loading").astype(float)
    active_grid_raw = grid("active_constraint_name").astype(str)
    x_grid, y_grid = np.meshgrid(values_b, values_a)

    fig, ax = plt.subplots(figsize=(11.2, 7.2))
    heatmap = ax.pcolormesh(
        x_grid, y_grid, loading_grid, shading="nearest", cmap="viridis",
    )
    colorbar = fig.colorbar(heatmap, ax=ax, pad=0.025)
    colorbar.set_label(y_axis_label.replace("Required ", "Selected "))

    if np.ptp(ws_grid) > 1.0:
        ws_levels = np.unique(np.round(
            np.linspace(float(np.min(ws_grid)), float(np.max(ws_grid)), 6),
            -1,
        ))
        if len(ws_levels) >= 2:
            ws_contours = ax.contour(
                x_grid, y_grid, ws_grid, levels=ws_levels,
                colors="white", linewidths=1.15, alpha=0.9,
            )
            ax.clabel(
                ws_contours, inline=True, fontsize=8,
                fmt=lambda value: f"W/S {value:.0f}",
            )

    active_names = sorted(np.unique(active_grid_raw))
    active_codes = np.zeros(active_grid_raw.shape, dtype=float)
    for code, active_name in enumerate(active_names):
        active_codes[active_grid_raw == active_name] = code
        positions = np.argwhere(active_grid_raw == active_name)
        center = positions[len(positions) // 2]
        ax.text(
            values_b[center[1]], values_a[center[0]],
            readable_constraint_name(active_name), ha="center", va="center",
            fontsize=8.2, color="white", fontweight="bold",
            bbox=dict(boxstyle="round,pad=0.22", facecolor="#0f172a",
                      edgecolor="white", alpha=0.72), zorder=6,
        )
    if len(active_names) > 1:
        ax.contour(
            x_grid, y_grid, active_codes,
            levels=np.arange(len(active_names) - 1) + 0.5,
            colors="#f8fafc", linewidths=2.4, alpha=0.95,
        )

    baseline = carpet[carpet["is_baseline"] == 1]
    if len(baseline) != 1:
        raise RuntimeError("Performance carpet must contain one nominal point.")
    row = baseline.iloc[0]
    ax.scatter(
        row[parameter_b], row[parameter_a],
        marker="*", s=190, color="#fbbf24", edgecolor="#0f172a",
        linewidth=0.9, zorder=8,
    )
    ax.annotate(
        "Nominal requirement\n"
        f"W/S = {float(row['best_wing_loading']):.0f} N/m²\n"
        f"{y_symbol} = {float(row['best_required_loading']):.3f}\n"
        f"Control: {readable_constraint_name(row['active_constraint_name'])}",
        (row[parameter_b], row[parameter_a]),
        xytext=(14, 15), textcoords="offset points", fontsize=8.7,
        arrowprops=dict(arrowstyle="->", color="#64748b", linewidth=1.0),
        bbox=dict(boxstyle="round,pad=0.3", facecolor="white",
                  edgecolor="#cbd5e1", alpha=0.96), zorder=9,
    )

    ax.set_title(analysis_title("Performance-Requirement Design Map"))
    ax.set_xlabel(parameter_b_label)
    ax.set_ylabel(parameter_a_label)
    clean_axes(ax)
    fig.text(
        0.5, 0.025,
        "Colour = selected engine/propulsion loading; white contours = "
        "selected W/S; labelled regions = governing requirement.",
        fontsize=8.8, color="#475569", ha="center", va="bottom",
    )
    fig.tight_layout(rect=(0.0, 0.07, 1.0, 1.0))
    save_plot("04_performance_requirement_design_map", tight=False)

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


# ============================================================
# Plot 4: User-performance requirement design map (--with-studies)
# ============================================================
if propeller_mode:
    performance_carpet_filename = "propeller_performance_carpet.csv"
    performance_carpet_parameters = (
        "climb_rate_ms", "takeoff_distance_m",
        "climb rate, ROC [m/s]", "take-off distance [m]",
        lambda value: f"ROC = {value:.1f} m/s",
        lambda value: f"s_TO = {value:.0f} m",
    )
else:
    performance_carpet_filename = "jet_performance_carpet.csv"
    performance_carpet_parameters = (
        "acceleration_ms2", "takeoff_distance_m",
        "required acceleration [m/s²]", "take-off distance [m]",
        lambda value: f"a = {value:.2f} m/s²",
        lambda value: f"s_TO = {value:.0f} m",
    )

performance_carpet_path = os.path.join(
    output_dir, performance_carpet_filename,
)
if os.path.exists(performance_carpet_path):
    plot_performance_carpet(
        pd.read_csv(performance_carpet_path).dropna(),
        *performance_carpet_parameters,
    )
else:
    stale_design_map = os.path.join(
        save_dir, "04_performance_requirement_design_map.png",
    )
    if os.path.exists(stale_design_map):
        os.remove(stale_design_map)
    print(
        f"Warning: {performance_carpet_filename} is missing. Run the C++ "
        "application with --with-studies to create the requirement design map."
    )

legacy_carpet_plot = os.path.join(save_dir, "04_performance_carpet_plot.png")
if os.path.exists(legacy_carpet_plot):
    os.remove(legacy_carpet_plot)


# ============================================================
# Plot 6: Independent mission verification
# ============================================================
mission_verification_path = os.path.join(
    output_dir, "mission_verification.csv",
)
if os.path.exists(mission_verification_path):
    verification = pd.read_csv(mission_verification_path)
    evaluated = verification[
        verification["status"].isin(["PASS", "FAIL"])
    ].copy().sort_values("time_s")
    if not evaluated.empty:
        use_range = np.ptp(evaluated["range_m"].to_numpy(dtype=float)) > 1.0
        if use_range:
            evaluated["mission_progress"] = evaluated["range_m"] / 1000.0
            progress_label = "Mission distance [km]"
        else:
            evaluated["mission_progress"] = evaluated["time_s"] / 60.0
            progress_label = "Mission time [min]"

        fig, (ax_profile, ax) = plt.subplots(
            2, 1, figsize=(11.4, 7.4), sharex=True,
            gridspec_kw={"height_ratios": [1.0, 2.1], "hspace": 0.08},
        )
        segment_styles = {
            "acceleration": ("#dc2626", "Acceleration"),
            "climb": ("#2563eb", "Climb"),
            "cruise": ("#16a34a", "Cruise"),
        }
        ax_profile.plot(
            evaluated["mission_progress"], evaluated["altitude_m"] / 1000.0,
            color="#64748b", linewidth=1.5, zorder=1,
        )
        ax.plot(
            evaluated["mission_progress"], evaluated["utilization_percent"],
            color="#94a3b8", linewidth=1.0, alpha=0.65, zorder=1,
        )
        for segment, (color, label) in segment_styles.items():
            points = evaluated[evaluated["segment"] == segment]
            if points.empty:
                continue
            ax_profile.scatter(
                points["mission_progress"], points["altitude_m"] / 1000.0,
                s=12, color=color, alpha=0.8, zorder=2,
            )
            ax.scatter(
                points["mission_progress"], points["utilization_percent"],
                s=20, color=color, edgecolor="white", linewidth=0.25,
                alpha=0.92, label=label, zorder=3,
            )

        critical = evaluated.loc[evaluated["utilization_percent"].idxmax()]
        critical_label = str(critical["segment"]).title()
        ax.scatter(
            [critical["mission_progress"]],
            [critical["utilization_percent"]], marker="*", s=190,
            color="#fbbf24", edgecolor="#111827", linewidth=0.8,
            zorder=6, label="Critical mission point",
        )
        ax.annotate(
            f"Critical: {critical_label}\n"
            f"h = {float(critical['altitude_m']):.0f} m, "
            f"V = {float(critical['speed_ms']):.1f} m/s\n"
            f"Utilization = {float(critical['utilization_percent']):.1f}%",
            (critical["mission_progress"], critical["utilization_percent"]),
            xytext=(14, 14), textcoords="offset points", fontsize=8.8,
            arrowprops=dict(arrowstyle="->", color="#64748b"),
            bbox=dict(boxstyle="round,pad=0.3", facecolor="white",
                      edgecolor="#cbd5e1", alpha=0.96),
        )
        ax.axhline(
            100.0, color="#b91c1c", linewidth=1.8,
            label="Available design capacity (100%)",
        )
        utilization_top = max(
            110.0, float(evaluated["utilization_percent"].max()) * 1.08,
        )
        utilization_bottom = min(
            0.0, float(evaluated["utilization_percent"].min()) * 0.95,
        )
        ax.axhspan(100.0, utilization_top, color="#fee2e2", alpha=0.42)
        ax.set_ylim(utilization_bottom, utilization_top)
        ax_profile.set_title(analysis_title(
            "Mission Verification of the Performance-Sized Design"
        ))
        ax_profile.set_ylabel("Altitude [km]")
        ax.set_xlabel(progress_label)
        ax.set_ylabel("Required / available capacity [%]")
        clean_axes(ax_profile)
        clean_axes(ax)
        ax.legend(frameon=False, loc="upper left", ncol=2, fontsize=8.3)
        outside_count = int(
            (verification["status"] == "OUTSIDE_MODEL_DOMAIN").sum()
        )
        fig.text(
            0.5, 0.025,
            "Below 100% = mission point satisfied; above 100% = failed. "
            f"Points outside the model/deck domain: {outside_count}.",
            ha="center", fontsize=8.8, color="#475569",
        )
        fig.tight_layout(rect=(0.0, 0.06, 1.0, 1.0))
        save_plot("06_mission_verification", tight=False)
else:
    stale_verification_plot = os.path.join(
        save_dir, "06_mission_verification.png",
    )
    if os.path.exists(stale_verification_plot):
        os.remove(stale_verification_plot)

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
