#include "energy_based/energy_based.h"

#include <cmath>
#include <stdexcept>

namespace constraint_analysis
{
    mattingly_airborne_case_result mattingly_airborne_case::compute(
        const mattingly_airborne_case_input& input)
    {
        constexpr double g = 9.80665;

        mattingly_airborne_case_result result;

        result.lift_coefficient =
            (input.load_factor * input.beta * input.wing_loading)
            / input.dynamic_pressure;

        result.drag_coefficient =
            input.k1 * result.lift_coefficient * result.lift_coefficient
            + input.k2 * result.lift_coefficient
            + input.cd0
            + input.cdr;

        result.drag_to_weight =
            (input.dynamic_pressure * result.drag_coefficient)
            / (input.beta * input.wing_loading);

        result.energy_term =
            input.climb_rate / input.velocity
            + input.acceleration / g;

        result.thrust_to_weight_sl =
            (input.beta / input.alpha)
            * (result.drag_to_weight + result.energy_term);

        return result;
    }

    double mattingly_jet_mission_segment::cruise_weight_fraction(
        const mattingly_jet_cruise_segment_input& input)
    {
        return std::exp(
            -(input.range_m * input.tsfc_1_per_s)
            / (input.velocity_ms * input.lift_to_drag)
        );
    }

    double mattingly_jet_mission_segment::loiter_weight_fraction(
        const mattingly_jet_loiter_segment_input& input)
    {
        return std::exp(
            -(input.endurance_s * input.tsfc_1_per_s)
            / input.lift_to_drag
        );
    }
}