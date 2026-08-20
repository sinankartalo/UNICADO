import os
import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
from matplotlib.lines import Line2D


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
    case_id = "UNICADO_PROPELLER" if propeller_mode else "JET_CASE"

case_labels = {
    "UNICADO_TEST_ENGINE": "Jet — V2527-A5 Test Engine",
    "UNICADO_REAL_ENGINE": "Jet — PW1127G-JM Real Engine",
    "SHORT_FIELD_DEMO": "Jet — Short-Field Demo",
    "UNICADO_PROPELLER": "Propeller — UNICADO",
}
analysis_label = case_labels.get(
    case_id,
    f"{'Propeller' if propeller_mode else 'Jet'} — {case_id}",
)

# Keep the deliverable aligned with the lecture workflow.  The existing
# matching chart and active-constraint figure remain part of the output; the
# new work is limited to one sensitivity plot and one classical carpet.
plot_allowlist = {
    "01_matching_chart_professional",
    "02_active_constraint_regions",
    "03_parameter_sensitivity",
    "04_classical_carpet_plot",
}
for existing_plot in os.listdir(save_dir):
    if not existing_plot.endswith(".png"):
        continue
    if os.path.splitext(existing_plot)[0] not in plot_allowlist:
        os.remove(os.path.join(save_dir, existing_plot))


def analysis_title(title):
    return f"{analysis_label}: {title}"

if propeller_mode:
    constraint_files = {
        "Acceleration": "propeller_acceleration_constraint.csv",
        "Climb": "propeller_climb_constraint.csv",
        "Cruise": "propeller_cruise_constraint.csv",
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
        "Climb": "jet_climb_constraint.csv",
        "Cruise": "jet_cruise_constraint.csv",
        "Max Mach": "jet_max_mach_constraint.csv",
        "Supercruise": "jet_supercruise_constraint.csv",
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


# Axis limits
x_min = envelope["wing_loading"].min()

# The landing and stall limits can lie outside the sampled W/S range.
# Expand the plotting range so those vertical constraint lines remain visible.
vertical_limits = [
    value for value in (landing_ws_limit, stall_ws_limit, gust_ws_limit)
    if value is not None
]

x_data_max = envelope["wing_loading"].max()
x_limit_max = max(vertical_limits, default=x_data_max)
x_max = max(x_data_max, x_limit_max) * 1.05

y_min = 0.0
y_max = max(
    envelope["thrust_to_weight"].max(),
    max(df["thrust_to_weight"].max() for df in constraints.values())
)
y_max *= 1.15


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

ax.plot(
    envelope["wing_loading"],
    envelope["thrust_to_weight"],
    color="black",
    linewidth=3.2,
    label="Envelope",
    zorder=5,
)

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

ax.set_xlim(x_min, x_max)
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
ax.set_xlim(x_min, x_max)
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
        ax.set_xlim(x_min, x_max)
        ax.set_ylim(0.0, carpet_y_max)
        clean_axes(ax)
        ax.legend(loc="upper left", bbox_to_anchor=(1.01, 1.0),
                  frameon=False, title="Constraints and limits")
        fig.subplots_adjust(right=0.79)
        save_plot("05_propeller_constraint_carpet")

    # ============================================================
    # Lecture-style propeller sensitivity and classical carpet
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

        # One-parameter sensitivity: vary J on each supplied pitch slice.
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
                label=f"Pitch = {pitch:g}°",
            )
        ax.set_title(analysis_title("Variation of Advance Ratio"))
        ax.set_xlabel("Advance Ratio, J [-]")
        ax.set_ylabel("Propeller Efficiency, η [-]")
        ax.set_ylim(0.0, 1.0)
        clean_axes(ax)
        ax.legend(frameon=False)
        save_plot("03_parameter_sensitivity")

        # Classical carpet: green lines are constant pitch, red lines are
        # constant J.  The plot uses CT and CP as the two configuration outputs.
        fig, ax = plt.subplots(figsize=(10.0, 7.0))
        pitch_color = "#10b981"
        advance_ratio_color = "#ef4444"
        for pitch in pitch_values:
            pitch_slice = valid_map[
                valid_map["pitch_deg"] == pitch
            ].sort_values("advance_ratio")
            ax.plot(
                pitch_slice["thrust_coefficient"],
                pitch_slice["power_coefficient"],
                color=pitch_color, linewidth=2.4,
            )
            label_row = pitch_slice.iloc[len(pitch_slice) // 2]
            ax.annotate(
                f"Pitch {pitch:g}°",
                (label_row["thrust_coefficient"],
                 label_row["power_coefficient"]),
                xytext=(5, 5), textcoords="offset points",
                fontsize=9, color="#047857",
            )

        minimum_common_j = max(
            valid_map[valid_map["pitch_deg"] == pitch]["advance_ratio"].min()
            for pitch in pitch_values
        )
        maximum_common_j = min(
            valid_map[valid_map["pitch_deg"] == pitch]["advance_ratio"].max()
            for pitch in pitch_values
        )
        common_j_values = np.linspace(
            minimum_common_j, maximum_common_j, 5
        )
        for advance_ratio in common_j_values:
            ct_values = []
            cp_values = []
            for pitch in pitch_values:
                pitch_slice = valid_map[
                    valid_map["pitch_deg"] == pitch
                ].sort_values("advance_ratio")
                ct_values.append(np.interp(
                    advance_ratio,
                    pitch_slice["advance_ratio"],
                    pitch_slice["thrust_coefficient"],
                ))
                cp_values.append(np.interp(
                    advance_ratio,
                    pitch_slice["advance_ratio"],
                    pitch_slice["power_coefficient"],
                ))
            ax.plot(
                ct_values, cp_values, color=advance_ratio_color,
                linewidth=2.2, marker="o", markersize=5.0,
                markerfacecolor="#0f172a", markeredgecolor="#0f172a",
            )
            ax.annotate(
                f"J = {advance_ratio:.2f}",
                (ct_values[-1], cp_values[-1]),
                xytext=(5, 0), textcoords="offset points",
                fontsize=9, color="#b91c1c", va="center",
            )

        family_handles = [
            Line2D([0], [0], color=pitch_color, linewidth=2.4,
                   label="Constant pitch"),
            Line2D([0], [0], color=advance_ratio_color, linewidth=2.2,
                   label="Constant advance ratio J"),
        ]
        ax.set_title(analysis_title("Classical Propeller Carpet Plot"))
        ax.set_xlabel("Thrust Coefficient, Cₜ [-]")
        ax.set_ylabel("Power Coefficient, Cₚ [-]")
        clean_axes(ax)
        ax.legend(handles=family_handles, frameon=False)
        save_plot("04_classical_carpet_plot")


# ============================================================
# Plot 3: CD0 carpet plot
# ============================================================
carpet_path = os.path.join(output_dir, "carpet_plot_full.csv")
study_path = os.path.join(output_dir, "carpet_plot_study.csv")

if (not propeller_mode and os.path.exists(carpet_path)
        and os.path.exists(study_path)):

    carpet = pd.read_csv(carpet_path)
    study = pd.read_csv(study_path)

    # ========================================================
    # Lecture-style one-parameter sensitivity at nominal k
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
    sensitivity = sensitivity_carpet[np.isclose(
        sensitivity_carpet["induced_drag_factor"], nominal_k,
        rtol=1.0e-9, atol=1.0e-12,
    )].sort_values("cd_0")
    if len(sensitivity) != 9:
        raise RuntimeError(
            "CD0 sensitivity at nominal k must contain nine points."
        )

    fig, ax = plt.subplots(figsize=(9, 5.5))

    ax.plot(
        sensitivity["cd_0"],
        sensitivity["best_thrust_to_weight"],
        color="#334155",
        linewidth=2.2,
        zorder=2,
    )

    active_names = list(sensitivity["active_constraint_name"].unique())
    active_palette = mpl.colormaps["tab10"](
        np.linspace(0.0, 0.8, max(len(active_names), 1))
    )
    for active_name, active_color in zip(active_names, active_palette):
        active_rows = sensitivity[
            sensitivity["active_constraint_name"] == active_name
        ]
        display_name = str(active_name).replace(
            "jet_", ""
        ).replace("_constraint", "").replace("_", " ").title()
        ax.scatter(
            active_rows["cd_0"],
            active_rows["best_thrust_to_weight"],
            s=52, color=active_color, edgecolor="white", linewidth=0.7,
            label=f"Active: {display_name}", zorder=4,
        )

    ax.scatter(
        baseline_row["cd_0"], baseline_row["best_thrust_to_weight"],
        marker="*", s=170, color="#fbbf24", edgecolor="#0f172a",
        linewidth=0.9, label="Imported nominal polar", zorder=6,
    )

    ax.set_title(analysis_title(
        f"Optimum T/W Sensitivity to CD₀ at Nominal k = {nominal_k:.4f}"
    ))
    ax.set_xlabel("Zero-Lift Drag Coefficient, CD₀ [-]")
    ax.set_ylabel("Optimum Required T/W [-]")

    for row_index, (_, row) in enumerate(sensitivity.iterrows()):
        if row_index % 2 != 0:
            continue
        ax.annotate(
            f"{row['best_thrust_to_weight']:.3f}",
            xy=(row["cd_0"], row["best_thrust_to_weight"]),
            xytext=(0, 8),
            textcoords="offset points",
            ha="center",
            fontsize=9,
        )

    clean_axes(ax)
    ax.legend(frameon=False)
    save_plot("03_parameter_sensitivity")


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

        fig, ax = plt.subplots(figsize=(10.0, 7.0))
        cd0_color = "#10b981"
        k_color = "#ef4444"

        # Green family: CD0 remains constant while k varies.
        label_indices = {0, 2, 4, 6, 8}
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

        required_columns = {"is_baseline", "active_constraint_name"}
        if not required_columns.issubset(aero_carpet.columns):
            raise RuntimeError(
                "Rerun the C++ application: carpet CSV schema is outdated."
            )
        baseline = aero_carpet[aero_carpet["is_baseline"] == 1]
        if len(baseline) != 1:
            raise RuntimeError(
                "Jet CD0-k carpet must contain one nominal polar point."
            )
        baseline_row = baseline.iloc[0]
        baseline_active = str(baseline_row["active_constraint_name"])
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
            xytext=(12, 14), textcoords="offset points", fontsize=8.5,
            color="#334155",
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

        ax.set_title(analysis_title("Classical Aerodynamic Carpet Plot"))
        ax.set_xlabel("Optimum Wing Loading, W/S [N/m²]")
        ax.set_ylabel("Optimum Required T/W [-]")
        clean_axes(ax)
        ax.legend(handles=family_handles, frameon=False)
        ax.text(
            0.01, 0.02,
            "81 interpolated feasible optima; labels show every other "
            "CD₀ and k level (80–120% of the imported polar).",
            transform=ax.transAxes, fontsize=8.8, color="#4d4d4d",
            va="bottom",
        )
        save_plot("04_classical_carpet_plot")

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
