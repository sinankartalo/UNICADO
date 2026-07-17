#pragma once

namespace constraint_analysis
{
    struct mattingly_airborne_case_input
    {
        double wing_loading = 0.0;
        double dynamic_pressure = 0.0;

        double alpha = 1.0;
        double beta = 1.0;

        double k1 = 0.0;
        double k2 = 0.0;
        double cd0 = 0.0;
        double cdr = 0.0;

        double load_factor = 1.0;
        double climb_rate = 0.0;
        double velocity = 0.0;
        double acceleration = 0.0;
    };

    struct mattingly_airborne_case_result
    {
        double lift_coefficient = 0.0;
        double drag_coefficient = 0.0;
        double drag_to_weight = 0.0;
        double energy_term = 0.0;
        double thrust_to_weight_sl = 0.0;
    };

    class mattingly_airborne_case
    {
    public:
        static mattingly_airborne_case_result compute(
            const mattingly_airborne_case_input& input);
    };

    struct mattingly_jet_cruise_segment_input
    {
        double range_m = 0.0;
        double velocity_ms = 0.0;
        double tsfc_1_per_s = 0.0;
        double lift_to_drag = 0.0;
    };

    struct mattingly_jet_loiter_segment_input
    {
        double endurance_s = 0.0;
        double tsfc_1_per_s = 0.0;
        double lift_to_drag = 0.0;
    };

    class mattingly_jet_mission_segment
    {
    public:
        static double cruise_weight_fraction(
            const mattingly_jet_cruise_segment_input& input);

        static double loiter_weight_fraction(
            const mattingly_jet_loiter_segment_input& input);
    };
}