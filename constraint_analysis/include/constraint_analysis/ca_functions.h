#pragma once

#include <string>
#include <vector>

#include <engine/engine.h>
#include <atmosphere/atmosphere.h>
#include <aerodynamics/aerodynamics_v3.h>

#include "energy_based/energy_based.h"

// Analysis inputs
namespace constraint_analysis
{
    enum class propulsion_type
    {
        jet,
        propeller
    };

    struct propeller_setting
    {
        double rpm = 0.0;
        double pitch_deg = 0.0;
    };

    struct propeller_data
    {
        aerodynamics::Propeller* model = nullptr;
        std::string deck_path;
        double diameter_m = 0.0;
        double tip_mach_limit = 0.9999;
        int count = 1;
    };


    struct drag_polar
    {
        double cd_0 = 0.0;
        double k = 0.0;
    };

    struct aerodynamic_polar_sample
    {
        double mach = 0.0;
        double altitude_m = 0.0;
        double cd_0 = 0.0;
        double k = 0.0;
        double cl_max = 0.0;
        std::string configuration_id;
    };

    struct aircraft_data
    {
        std::string aerodynamic_polar_xml_path;
        std::string reference_wing_id;
        double wing_area_m2 = 0.0;
        double takeoff_weight_N = 0.0;
        double aspect_ratio = 0.0;
        double mean_aerodynamic_chord_m = 0.0;
        double cl_max_takeoff = 0.0;
        double cl_max_landing = 0.0;
        double aerodynamic_minimum_mach = 0.0;
        double aerodynamic_maximum_mach = 0.0;
        drag_polar polar;
        std::vector<aerodynamic_polar_sample> polar_samples;
    };

    struct takeoff_constraint
    {
        double altitude_m = 0.0;
        double runway_m = 0.0;
        double speed_factor = 1.2;

        double beta_to = 1.0;

        double mu_ro = 0.02;
        double cd_ground = 0.04;
    };

    struct max_mach_constraint
    {
        double altitude_m = 0.0;
        double mach = 0.0;
        double beta_max_mach = 1.0;
    };

    struct climb_mission_point
    {
        double altitude_m = 0.0;
        double speed_ms = 0.0;
        double roc_ms = 0.0;
        double acceleration_ms2 = 0.0;
        double beta_climb = 1.0;
    };

    struct acceleration_constraint
    {
        double altitude_m = 0.0;
        double speed_ms = 0.0;
        double acceleration_ms2 = 0.0;
        double beta_acceleration = 1.0;
        std::vector<climb_mission_point> mission_points;
    };

    struct climb_constraint
    {
        // Mission climb history used to build the worst-case envelope.
        std::vector<climb_mission_point> mission_points;

        // Representative climb point with the largest kinematic demand.
        climb_mission_point representative_point;
    };

    struct cruise_constraint
    {
        double altitude_m = 0.0;
        double speed_ms = 0.0;

        double beta_cruise = 1.0;
        std::vector<climb_mission_point> mission_points;
        bool allow_configured_fallback = false;
    };

    struct turn_constraint
    {
        double altitude_m = 0.0;
        double speed_ms = 0.0;
        double load_factor = 1.0;

        double beta_turn = 1.0;
    };

    struct landing_constraint
    {
        double altitude_m = 0.0;
        double runway_m = 0.0;
        double speed_factor = 1.15;
        double mu_brake = 0.40;
        double cd_brake = 0.10;

        double beta_landing = 0.85;
    };

    struct stall_speed_constraint
    {
        // Altitude and weight fraction are inherited from the landing case.
        double altitude_m = 0.0;
        double speed_limit_ms = 0.0;
        double beta_stall = 1.0;
    };

    struct gust_constraint
    {
        // Mission cruise points used to determine the governing gust limit.
        std::vector<climb_mission_point> mission_points;
    };

    struct range_constraint
    {
        double altitude_m = 0.0;
        double speed_ms = 0.0;
        double range_m = 0.0;
        double beta_start = 0.95;
        double available_fuel_fraction = 0.25;
    };

    struct mission_verification_point
    {
        std::string segment;
        climb_mission_point condition;
        double time_s = 0.0;
        double range_m = 0.0;
        std::size_t source_index = 0;
    };

    struct mission_verification_data
    {
        // Mission samples used only for post-design verification.
        std::vector<mission_verification_point> points;
    };

    struct constraint_activation
    {
        bool takeoff_ground_roll = true;
        bool landing_field_length = true;
        bool stall_speed = true;
        bool gust = true;
        bool max_mach = true;
        bool horizontal_acceleration = true;
        bool cruise = true;
        bool climb = true;
        bool constant_speed_turn = true;
        bool range_fuel_fraction = true;
    };

    struct constraint_input
    {
        // Constraint source: user-defined performance cases or mission history.
        std::string condition_source = "performance";
        constraint_activation active;
        propulsion_type propulsion = propulsion_type::jet;
        aircraft_data aircraft;

        // Engine model used for thrust-lapse and TSFC data.
        Engine* engine = nullptr;
        // Optional thrust-lapse scale factor; 1.0 is nominal.
        double installed_thrust_lapse_scale = 1.0;
        propeller_data propeller;

        takeoff_constraint takeoff;
        max_mach_constraint max_mach;
        acceleration_constraint acceleration;
        climb_constraint climb;
        cruise_constraint cruise;
        turn_constraint turn;
        landing_constraint landing;
        stall_speed_constraint stall_speed;
        gust_constraint gust;
        range_constraint range;
        mission_verification_data mission_verification;

        double wing_loading_min = 0.0;
        double wing_loading_max = 0.0;
        double wing_loading_step = 0.0;
    };

    struct propeller_operating_point
    {
        double altitude_m = 0.0;
        double speed_ms = 0.0;
        double density_kg_m3 = 0.0;
        double speed_of_sound_ms = 0.0;
        double rpm = 0.0;
        double pitch_deg = 0.0;
        double advance_ratio = 0.0;
        double thrust_coefficient = 0.0;
        double power_coefficient = 0.0;
        double efficiency = 0.0;
        double thrust_N = 0.0;
        double shaft_power_W = 0.0;
        double tip_speed_ms = 0.0;
        double tip_mach = 0.0;
        double tip_mach_limit = 0.0;
        bool tip_mach_feasible = false;
    };

    struct propeller_map_point
    {
        double pitch_deg = 0.0;
        double advance_ratio = 0.0;
        double thrust_coefficient = 0.0;
        double power_coefficient = 0.0;
        double efficiency = 0.0;
    };

    struct propeller_takeoff_step
    {
        double speed_ms = 0.0;
        double distance_m = 0.0;
        double acceleration_ms2 = 0.0;
        double lift_N = 0.0;
        double drag_N = 0.0;
        double rolling_resistance_N = 0.0;
        double required_thrust_N = 0.0;
        double required_total_shaft_power_W = 0.0;
        propeller_operating_point deck_point;
    };

    struct propeller_takeoff_result
    {
        double wing_loading_N_m2 = 0.0;
        double takeoff_speed_ms = 0.0;
        double required_shaft_power_to_weight_W_N = 0.0;
        double integrated_ground_roll_m = 0.0;
        std::vector<propeller_takeoff_step> steps;
    };

    struct propeller_climb_coverage
    {
        std::size_t total_mission_points = 0;
        std::size_t valid_deck_points = 0;
        std::size_t invalid_deck_points = 0;
        double first_invalid_altitude_m = 0.0;
        double first_invalid_speed_ms = 0.0;
        double first_invalid_advance_ratio = 0.0;
        std::string first_invalid_reason;
    };
}

// Analysis outputs
#include <string>
#include <vector>

namespace constraint_analysis
{
    struct curve_point
    {
        double x = 0.0;
        double y = 0.0;
    };

    struct constraint_curve
    {
        std::string name;
        std::vector<curve_point> points;
    };

    struct vertical_constraint
    {
        std::string name;
        double x_limit = 0.0;

        // Upper limits bound W/S from above; lower limits bound it from below.
        bool is_upper_limit = true;
    };

    struct range_constraint_point
    {
        double wing_loading = 0.0;
        double lift_to_drag = 0.0;
        double required_fuel_fraction = 0.0;
        bool feasible = false;
    };

    struct range_constraint_result
    {
        std::string name;
        double available_fuel_fraction = 0.0;
        std::vector<range_constraint_point> points;
    };

    struct constraint_output
    {
        std::vector<constraint_curve> curves;
        std::vector<vertical_constraint> vertical_constraints;
        std::vector<range_constraint_result> range_constraints;
    };
}

// Utility functions
namespace constraint_analysis
{
    class constraint_utilities
    {
    public:
        static double compute_dynamic_pressure(double density, double speed);
        static double compute_lift_coefficient(double wing_loading, double q, double load_factor = 1.0);
        static double compute_drag_coefficient(double cd_0, double k, double cl);
    };
}


// Takeoff ground-roll model
namespace constraint_analysis
{
    /* Mattingly Case 6 ground-roll model, solved for T_SL/W_TO.
       The correction term is xi_TO = C_D + C_DR - mu_TO*C_L. */
    struct mattingly_takeoff_ground_roll_input
    {
        double wing_loading = 0.0;      // W_TO / S [N/m^2]
        double ground_roll_m = 0.0;     // takeoff ground roll distance S_G [m]
        double density = 0.0;           // air density rho [kg/m^3]

        double alpha = 1.0;             // installed takeoff thrust lapse [-]
        double beta = 1.0;              // takeoff weight fraction W / W_TO [-]

        double cl_max = 0.0;            // takeoff maximum lift coefficient [-]
        double k_to = 1.2;              // V_TO / V_stall [-]

        double cd = 0.0;                // representative drag coefficient during ground roll [-]
        double cdr = 0.0;               // additional drag coefficient [-]
        double cl = 0.0;                // representative lift coefficient during ground roll [-]
        double mu = 0.02;               // rolling friction coefficient [-]
    };

    struct mattingly_takeoff_ground_roll_result
    {
        double xi_to = 0.0;
        double thrust_to_weight_sl = 0.0;
    };

    class mattingly_takeoff_ground_roll
    {
    public:
        static mattingly_takeoff_ground_roll_result compute(
            const mattingly_takeoff_ground_roll_input& input);
    };
}

// Landing braking-roll model
namespace constraint_analysis
{
    /* Mattingly Eq. 2.33 braking-roll model, solved for the
       maximum takeoff wing loading W_TO/S. */
    struct mattingly_landing_braking_roll_input
    {
        double ground_roll_m = 0.0;     // landing/braking ground roll [m]
        double density = 0.0;           // air density rho [kg/m^3]
        double beta = 1.0;              // landing weight fraction W_L / W_TO [-]

        double cl_max = 0.0;            // landing CLmax [-]
        double k_landing = 1.15;        // touchdown speed factor [-]

        double cd = 0.0;                // representative drag coefficient during braking [-]
        double cdr = 0.0;               // additional drag coefficient [-]
        double cl = 0.0;                // representative lift coefficient during braking [-]
        double mu = 0.40;               // braking friction coefficient [-]
    };

    struct mattingly_landing_braking_roll_result
    {
        double xi_landing = 0.0;        // CD + CDR - mu*CL [-]
        double wing_loading_limit = 0.0;
    };

    class mattingly_landing_braking_roll
    {
    public:
        static mattingly_landing_braking_roll_result compute(
            const mattingly_landing_braking_roll_input& input);
    };
}


// Jet constraint analysis
namespace constraint_analysis
{
    class jet_constraint_analysis
    {
    public:
        explicit jet_constraint_analysis(const atmosphere& atmosphere);

        constraint_curve compute_takeoff_constraint(const constraint_input& input) const;
        vertical_constraint compute_landing_constraint_limit(const constraint_input& input) const;
        vertical_constraint compute_stall_speed_constraint_limit(const constraint_input& input) const;
        vertical_constraint compute_gust_constraint_limit(const constraint_input& input) const;

        constraint_curve compute_max_mach_constraint(const constraint_input& input) const;
        constraint_curve compute_acceleration_constraint(const constraint_input& input) const;
        constraint_curve compute_cruise_constraint(const constraint_input& input) const;
        constraint_curve compute_climb_constraint(const constraint_input& input) const;
        constraint_curve compute_turn_constraint(const constraint_input& input) const;

    private:
        const atmosphere& atmosphere_;
    };

    class propeller_constraint_analysis
    {
    public:
        explicit propeller_constraint_analysis(const atmosphere& atmosphere);

        static std::vector<propeller_map_point> read_performance_map(
            const std::string& deck_path);

        propeller_operating_point evaluate(
            const constraint_input& input,
            double altitude_m,
            double speed_ms,
            const propeller_setting& setting) const;

        propeller_operating_point select_best_airborne_operating_point(
            const constraint_input& input,
            double altitude_m,
            double speed_ms) const;

        propeller_takeoff_result solve_takeoff_ground_roll(
            const constraint_input& input,
            double wing_loading_N_m2,
            bool retain_steps = false) const;

        constraint_curve compute_takeoff_constraint(const constraint_input& input) const;
        constraint_curve compute_acceleration_constraint(const constraint_input& input) const;
        constraint_curve compute_cruise_constraint(const constraint_input& input) const;
        constraint_curve compute_climb_constraint(const constraint_input& input) const;
        constraint_curve compute_turn_constraint(const constraint_input& input) const;

        propeller_climb_coverage assess_climb_coverage(
            const constraint_input& input) const;

    private:
        constraint_curve compute_airborne_constraint(
            const constraint_input& input,
            const std::string& name,
            double altitude_m,
            double speed_ms,
            double beta,
            double load_factor,
            double climb_rate_ms,
            double acceleration_ms2) const;

        constraint_curve compute_mission_airborne_constraint(
            const constraint_input& input,
            const std::string& name,
            const std::vector<climb_mission_point>& mission_points) const;

        const atmosphere& atmosphere_;
    };
}


// Range constraint analysis
namespace constraint_analysis
{
    class range_constraint_analysis
    {
    public:
        explicit range_constraint_analysis(const atmosphere& atmosphere);

        range_constraint_result compute_range_constraint(const constraint_input& input) const;

    private:
        const atmosphere& atmosphere_;
    };
}
