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
        df = df.rename(columns={"x": "wing_loading", "y": "power_to_weight_W_per_N"})

    required = ["wing_loading", "power_to_weight_W_per_N"]

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
    "Acceleration": "propeller_acceleration_constraint.csv",
    "Max speed": "propeller_max_speed_constraint.csv",
    "High-speed cruise": "propeller_high_speed_cruise_constraint.csv",
    "Takeoff": "propeller_takeoff_constraint.csv",
    "Turn": "propeller_turn_constraint.csv",
}

for regime in ("subsonic", "transonic", "supersonic"):
    for segment in ("climb", "cruise"):
        filename = f"propeller_{regime}_{segment}_constraint.csv"
        if os.path.exists(os.path.join(output_dir, filename)):
            constraint_files[
                f"{regime.title()} {segment.title()}"
            ] = filename

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

    if "power_to_weight_W_per_N" not in design_point_df.columns:
        raise RuntimeError(
            "output/design_point.csv belongs to an older jet run. "
            "Run constraint_analysis_app.exe UNICADO_PROPELLER successfully "
            "before running this plotting script."
        )

    design_ws = float(design_point_df.loc[0, "wing_loading"])
    design_tw = float(design_point_df.loc[0, "power_to_weight_W_per_N"])
else:
    idx = envelope["power_to_weight_W_per_N"].idxmin()
    design_ws = float(envelope.loc[idx, "wing_loading"])
    design_tw = float(envelope.loc[idx, "power_to_weight_W_per_N"])


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
landing_ws_limit = read_vertical_limit("propeller_landing_limit.csv")
stall_ws_limit = read_vertical_limit("propeller_stall_speed_limit.csv")
gust_ws_limit = read_vertical_limit("propeller_gust_limit.csv")


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
    envelope["power_to_weight_W_per_N"].max(),
    max(df["power_to_weight_W_per_N"].max() for df in constraints.values())
) * 1.15


# ============================================================
# Plot 1: Professional constraint envelope
# ============================================================
fig, ax = plt.subplots()

color_map = {
    "Acceleration": "#1f77b4",
    "Takeoff": "#d62728",
    "Landing": "#9467bd",
    "Stall speed": "#bcbd22",
    "Gust": "#7f7f7f",
    "Climb": "#2ca02c",
    "Cruise": "#ff7f0e",
    "Max speed": "#8c564b",
    "High-speed cruise": "#17becf",
    "Turn": "#e377c2",
    "Range": "#7f7f7f",
    "Subsonic Climb": "#2ca02c",
    "Subsonic Cruise": "#ff7f0e",
    "Transonic Climb": "#9467bd",
    "Transonic Cruise": "#17becf",
    "Supersonic Climb": "#006d2c",
    "Supersonic Cruise": "#d95f02",
}

for name, df in constraints.items():
    ax.plot(
        df["wing_loading"],
        df["power_to_weight_W_per_N"],
        label=name,
        color=color_map.get(name, None),
        alpha=0.78,
        linewidth=1.7,
    )

ax.plot(
    envelope["wing_loading"],
    envelope["power_to_weight_W_per_N"],
    color="black",
    linewidth=3.4,
    label="Envelope",
    zorder=5,
)

ax.scatter(
    design_ws,
    design_tw,
    s=85,
    color="black",
    edgecolor="white",
    linewidth=1.2,
    zorder=6,
)

ax.annotate(
    f"Design point\nW/S = {design_ws:.0f} N/m²\nP/W = {design_tw:.2f} W/N",
    xy=(design_ws, design_tw),
    xytext=(design_ws + 450, design_tw + 0.12),
    arrowprops=dict(arrowstyle="->", linewidth=1.2),
    bbox=dict(boxstyle="round,pad=0.35", facecolor="white", edgecolor="gray", alpha=0.95),
)

if landing_ws_limit is not None:
    ax.axvline(
        landing_ws_limit,
        color="purple",
        linestyle="--",
        linewidth=2.0,
        label=f"Landing W/S limit = {landing_ws_limit:.0f}",
    )

if stall_ws_limit is not None:
    ax.axvline(
        stall_ws_limit,
        color=color_map["Stall speed"],
        linestyle=":",
        linewidth=2.4,
        label=f"Stall speed W/S limit = {stall_ws_limit:.0f}",
    )

if gust_ws_limit is not None:
    ax.axvline(
        gust_ws_limit,
        color=color_map["Gust"],
        linestyle="-.",
        linewidth=2.0,
        label=f"Gust W/S min = {gust_ws_limit:.0f}",
    )

ax.set_title("Constraint Analysis Matching Chart")
ax.set_xlabel("Wing Loading, W/S [N/m²]")
ax.set_ylabel("Required Shaft Power-to-Weight, P/W [W/N]")

ax.set_xlim(x_min, x_max)
ax.set_ylim(y_min, y_max)

ax.legend(loc="upper left", frameon=True, ncol=2)
save_plot("01_matching_chart_professional")


# ============================================================
# Plot 2: Active constraint map
# ============================================================
x_env = envelope["wing_loading"].values
y_env = envelope["power_to_weight_W_per_N"].values

interp_values = {}

for name, df in constraints.items():
    interp_values[name] = np.interp(
        x_env,
        df["wing_loading"].values,
        df["power_to_weight_W_per_N"].values,
    )

active = []

for i in range(len(x_env)):
    values_here = {name: values[i] for name, values in interp_values.items()}
    active.append(max(values_here, key=values_here.get))


fig, ax = plt.subplots()

for name, df in constraints.items():
    ax.plot(
        df["wing_loading"],
        df["power_to_weight_W_per_N"],
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
ax.set_ylabel("Required Shaft Power-to-Weight, P/W [W/N]")
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
            group.get("power_to_weight_W_per_N", group["thrust_to_weight"]),
            linewidth=2.0,
            label=f"CD₀ = {cd0:.3f}",
        )

    ax.scatter(
        study["best_wing_loading"],
        study.get("best_power_to_weight", study["best_thrust_to_weight"]),
        color="black",
        edgecolor="white",
        s=70,
        zorder=5,
        label="Optimum points",
    )

    ax.set_title("Carpet Plot: Effect of Zero-Lift Drag Coefficient")
    ax.set_xlabel("Wing Loading, W/S [N/m²]")
    ax.set_ylabel("Required Shaft Power-to-Weight, P/W [W/N]")
    ax.legend(loc="upper left", frameon=True, ncol=2)

    save_plot("03_cd0_carpet_plot")


    # ========================================================
    # Plot 4: Best P/W vs CD0
    # ========================================================
    fig, ax = plt.subplots(figsize=(9, 5.5))

    ax.plot(
        study["cd_0"],
        study.get("best_power_to_weight", study["best_thrust_to_weight"]),
        marker="o",
        color="black",
        linewidth=2.2,
    )

    ax.set_title("Sensitivity of Optimum P/W to CD₀")
    ax.set_xlabel("Zero-Lift Drag Coefficient, CD₀ [-]")
    ax.set_ylabel("Optimum Required P/W [W/N]")

    for _, row in study.iterrows():
        ax.annotate(
            f"{row.get('best_power_to_weight', row['best_thrust_to_weight']):.3f}",
            xy=(row["cd_0"], row["best_power_to_weight"]),
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
    range_path = os.path.join(output_dir, "propeller_range_fuel_fraction_constraint.csv")

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
                group.get("power_to_weight_W_per_N", group["thrust_to_weight"]),
                linewidth=2.4,
                label=f"CD₀ = {cd0:.3f}"
            )

        if os.path.exists(study_path):
            study = pd.read_csv(study_path)

            feasible = study[study["range_feasible"] == 1]
            infeasible = study[study["range_feasible"] == 0]

            ax.scatter(
                feasible["best_wing_loading"],
                feasible.get("best_power_to_weight", feasible["best_thrust_to_weight"]),
                s=80,
                color="black",
                edgecolor="white",
                zorder=5,
                label="Feasible optimum"
            )

            if len(infeasible) > 0:
                ax.scatter(
                    infeasible["best_wing_loading"],
                    infeasible.get("best_power_to_weight", feasible["best_thrust_to_weight"]),
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
        ax.set_ylabel("Required Shaft Power-to-Weight, P/W [W/N]")

        ax.grid(True, linestyle="--", alpha=0.3)
        ax.legend(loc="upper left", frameon=True)

        save_plot("07_constraint_envelope_carpet_plot")


print()
print("Plot generation completed.")
print(f"Design point: W/S = {design_ws:.0f} N/m², P/W = {design_tw:.4f} W/N")
print(f"Plots saved to: {save_dir}")
