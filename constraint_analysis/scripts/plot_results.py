import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


# ============================================================
# Paths
# ============================================================
output_dir = "output"
save_dir = "plots"
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
    "grid.alpha": 0.28,
    "grid.linestyle": "--",
    "lines.linewidth": 1.8,
    "savefig.dpi": 300,
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


def save_plot(name):

    png_path = os.path.join(save_dir, f"{name}.png")

    plt.tight_layout()

    plt.savefig(
        png_path,
        dpi=300,
        bbox_inches="tight"
    )

    plt.close()

    print(f"Saved: {png_path}")


# ============================================================
# Load constraint data
# ============================================================
constraint_files = {
    "Acceleration": "jet_acceleration_constraint.csv",
    "Climb": "jet_climb_constraint.csv",
    "Cruise": "jet_cruise_constraint.csv",
    "Max Mach": "jet_max_mach_constraint.csv",
    "Supercruise": "jet_supercruise_constraint.csv",
    "Takeoff": "jet_takeoff_constraint.csv",
    "Turn": "jet_turn_constraint.csv",
}

constraints = {}

for name, filename in constraint_files.items():
    df = load_xy_csv(filename)
    if df is not None:
        constraints[name] = df


envelope = load_xy_csv("constraint_envelope.csv")

if envelope is None:
    raise FileNotFoundError("constraint_envelope.csv not found.")


# Prefer the C++ interpolated design point. Fall back to the sampled
# envelope minimum for compatibility with older output folders.
design_point_path = os.path.join(output_dir, "design_point.csv")

if os.path.exists(design_point_path):
    design_point_df = pd.read_csv(design_point_path)
    design_ws = float(design_point_df.loc[0, "wing_loading"])
    design_tw = float(design_point_df.loc[0, "thrust_to_weight"])
else:
    idx = envelope["thrust_to_weight"].idxmin()
    design_ws = float(envelope.loc[idx, "wing_loading"])
    design_tw = float(envelope.loc[idx, "thrust_to_weight"])


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
landing_ws_limit = read_vertical_limit("jet_landing_limit.csv")
stall_ws_limit = read_vertical_limit("jet_stall_speed_limit.csv")
gust_ws_limit = read_vertical_limit("jet_gust_limit.csv")


# Axis limits
x_data_min = envelope["wing_loading"].min()
x_data_max = envelope["wing_loading"].max()

# Frame the meaningful wing-loading interval between the gust minimum and
# the rightmost upper limit. Add the same absolute padding beyond both
# vertical boundary lines so their distances to the plot edges are equal.
upper_physical_limits = [
    value for value in (landing_ws_limit, stall_ws_limit)
    if value is not None
]
x_core_min = gust_ws_limit if gust_ws_limit is not None else x_data_min
x_core_max = max(upper_physical_limits) if upper_physical_limits else x_data_max
x_padding = 0.03 * (x_core_max - x_core_min)
x_min = max(0.0, x_core_min - x_padding)
x_max = x_core_max + x_padding

y_min = 0.0
y_max = max(
    envelope["thrust_to_weight"].max(),
    max(df["thrust_to_weight"].max() for df in constraints.values())
) * 1.15


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

# Lightly mark the wing-loading interval allowed by all vertical limits.
# The thrust constraints still determine the required T/W inside this band.
upper_ws_limits = [
    value for value in (landing_ws_limit, stall_ws_limit)
    if value is not None
]
feasible_ws_min = gust_ws_limit if gust_ws_limit is not None else x_min
feasible_ws_max = min(upper_ws_limits) if upper_ws_limits else x_max

if feasible_ws_min < feasible_ws_max:
    ax.axvspan(
        feasible_ws_min,
        feasible_ws_max,
        color="#2ca02c",
        alpha=0.055,
        zorder=0,
    )

ax.plot(
    envelope["wing_loading"],
    envelope["thrust_to_weight"],
    color="black",
    linewidth=3.2,
    label="Envelope",
    zorder=5,
)

ax.scatter(
    design_ws,
    design_tw,
    s=85,
    color="#d62728",
    edgecolor="white",
    linewidth=1.4,
    zorder=6,
    label="Selected design point",
)

ax.annotate(
    f"W/S = {design_ws:.0f} N/m²\nT/W = {design_tw:.3f}",
    xy=(design_ws, design_tw),
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

ax.set_title("Constraint Analysis Matching Chart", pad=12, fontweight="semibold")
ax.set_xlabel("Wing Loading, W/S [N/m²]")
ax.set_ylabel("Required Thrust-to-Weight Ratio, T/W [-]")

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


fig, ax = plt.subplots()

for name, df in constraints.items():
    ax.plot(
        df["wing_loading"],
        df["thrust_to_weight"],
        color=color_map.get(name, None),
        alpha=0.25,
        linewidth=1.2,
    )

# Continuous envelope background to avoid visual gaps
ax.plot(
    x_env,
    y_env,
    color="black",
    linewidth=2.0,
    alpha=0.35,
    label="Envelope"
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
            y_env[mid] + 0.04,
            name,
            ha="center",
            va="bottom",
            fontsize=10,
            weight="bold",
            bbox=dict(facecolor="white", edgecolor="none", alpha=0.75),
        )

        start = i

ax.scatter(
    design_ws,
    design_tw,
    s=85,
    color="black",
    edgecolor="white",
    linewidth=1.2,
    zorder=6,
)

if landing_ws_limit is not None:
    ax.axvline(
        landing_ws_limit,
        color="purple",
        linestyle="--",
        linewidth=2.0,
    )

if stall_ws_limit is not None:
    ax.axvline(
        stall_ws_limit,
        color=color_map["Stall speed"],
        linestyle=":",
        linewidth=2.4,
    )

if gust_ws_limit is not None:
    ax.axvline(
        gust_ws_limit,
        color=color_map["Gust"],
        linestyle="-.",
        linewidth=2.0,
    )

ax.set_title("Active Constraint Regions")
ax.set_xlabel("Wing Loading, W/S [N/m²]")
ax.set_ylabel("Required Thrust-to-Weight Ratio, T/W [-]")
ax.set_xlim(x_min, x_max)
ax.set_ylim(y_min, y_max)

# Tekrarlı legend temizliği
handles, labels = ax.get_legend_handles_labels()
unique = dict(zip(labels, handles))
ax.legend(unique.values(), unique.keys(), loc="upper left", frameon=True)

save_plot("02_active_constraint_regions")


# ============================================================
# Plot 3: CD0 carpet plot
# ============================================================
carpet_path = os.path.join(output_dir, "carpet_plot_full.csv")
study_path = os.path.join(output_dir, "carpet_plot_study.csv")

if os.path.exists(carpet_path) and os.path.exists(study_path):

    carpet = pd.read_csv(carpet_path)
    study = pd.read_csv(study_path)

    fig, ax = plt.subplots()

    cd0_values = sorted(carpet["cd_0"].unique())

    for cd0 in cd0_values:
        group = carpet[carpet["cd_0"] == cd0].sort_values("wing_loading")

        ax.plot(
            group["wing_loading"],
            group["thrust_to_weight"],
            linewidth=2.0,
            label=f"CD₀ = {cd0:.3f}",
        )

    ax.scatter(
        study["best_wing_loading"],
        study["best_thrust_to_weight"],
        color="black",
        edgecolor="white",
        s=70,
        zorder=5,
        label="Optimum points",
    )

    ax.set_title("Carpet Plot: Effect of Zero-Lift Drag Coefficient")
    ax.set_xlabel("Wing Loading, W/S [N/m²]")
    ax.set_ylabel("Required Thrust-to-Weight Ratio, T/W [-]")
    ax.legend(loc="upper left", frameon=True, ncol=2)

    save_plot("03_cd0_carpet_plot")


    # ========================================================
    # Plot 4: Best T/W vs CD0
    # ========================================================
    fig, ax = plt.subplots(figsize=(9, 5.5))

    ax.plot(
        study["cd_0"],
        study["best_thrust_to_weight"],
        marker="o",
        color="black",
        linewidth=2.2,
    )

    ax.set_title("Sensitivity of Optimum T/W to CD₀")
    ax.set_xlabel("Zero-Lift Drag Coefficient, CD₀ [-]")
    ax.set_ylabel("Optimum Required T/W [-]")

    for _, row in study.iterrows():
        ax.annotate(
            f"{row['best_thrust_to_weight']:.3f}",
            xy=(row["cd_0"], row["best_thrust_to_weight"]),
            xytext=(0, 8),
            textcoords="offset points",
            ha="center",
            fontsize=9,
        )

    save_plot("04_optimum_tw_vs_cd0")


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

    ax.set_title("Sensitivity of Optimum W/S to CD₀")
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

        plt.title("Range Constraint: Fuel Fraction and L/D")

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
    # Plot 7: Constraint envelope carpet plot
    # ============================================================

    carpet_path = os.path.join(output_dir, "carpet_plot_full.csv")
    study_path = os.path.join(output_dir, "carpet_plot_study.csv")

    if os.path.exists(carpet_path):

        carpet = pd.read_csv(carpet_path)

        fig, ax = plt.subplots(figsize=(11, 6.5))

        cd0_values = sorted(carpet["cd_0"].unique())

        for cd0 in cd0_values:
            group = carpet[carpet["cd_0"] == cd0].sort_values("wing_loading")

            ax.plot(
                group["wing_loading"],
                group["thrust_to_weight"],
                linewidth=2.4,
                label=f"CD₀ = {cd0:.3f}"
            )

        if os.path.exists(study_path):
            study = pd.read_csv(study_path)

            feasible = study[study["range_feasible"] == 1]
            infeasible = study[study["range_feasible"] == 0]

            ax.scatter(
                feasible["best_wing_loading"],
                feasible["best_thrust_to_weight"],
                s=80,
                color="black",
                edgecolor="white",
                zorder=5,
                label="Feasible optimum"
            )

            if len(infeasible) > 0:
                ax.scatter(
                    infeasible["best_wing_loading"],
                    infeasible["best_thrust_to_weight"],
                    s=95,
                    marker="x",
                    color="red",
                    linewidth=2.2,
                    zorder=6,
                    label="Range infeasible optimum"
                )

        if landing_ws_limit is not None:
            ax.axvline(
                landing_ws_limit,
                color="purple",
                linestyle="--",
                linewidth=1.8,
                label=f"Landing W/S limit = {landing_ws_limit:.0f}"
            )

        if stall_ws_limit is not None:
            ax.axvline(
                stall_ws_limit,
                color=color_map["Stall speed"],
                linestyle=":",
                linewidth=2.0,
                label=f"Stall speed W/S limit = {stall_ws_limit:.0f}"
            )

        if gust_ws_limit is not None:
            ax.axvline(
                gust_ws_limit,
                color=color_map["Gust"],
                linestyle="-.",
                linewidth=1.8,
                label=f"Gust W/S min = {gust_ws_limit:.0f}"
            )

        ax.set_title("Constraint Envelope Carpet Plot")
        ax.set_xlabel("Wing Loading, W/S [N/m²]")
        ax.set_ylabel("Required Thrust-to-Weight Ratio, T/W [-]")

        ax.grid(True, linestyle="--", alpha=0.3)
        ax.legend(loc="upper left", frameon=True)

        save_plot("07_constraint_envelope_carpet_plot")


print()
print("Plot generation completed.")
print(f"Design point: W/S = {design_ws:.0f} N/m², T/W = {design_tw:.4f}")
print(f"Plots saved to: {save_dir}")
