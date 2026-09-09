#include "constraint_analysis/constraint_analysis.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace constraint_analysis
{
    namespace
    {
        struct mach_regime
        {
            const char* name;
            double minimum_mach;
            double maximum_mach;
        };

        constexpr std::array<mach_regime, 3> mission_mach_regimes = {{
            {"subsonic", 0.0, 0.95},
            {"transonic", 0.95, 1.20},
            {"supersonic", 1.20, std::numeric_limits<double>::infinity()}
        }};

        std::vector<climb_mission_point> points_in_regime(
            const std::vector<climb_mission_point>& points,
            const mach_regime& regime,
            const atmosphere& atmosphere,
            double aerodynamic_minimum_mach,
            double aerodynamic_maximum_mach)
        {
            std::vector<climb_mission_point> selected;
            for (const auto& point : points)
            {
                const double speed_of_sound =
                    atmosphere.getSpeedOfSound(point.altitude_m);
                const double mach = point.speed_ms / speed_of_sound;
                if (std::isfinite(mach) &&
                    mach >= regime.minimum_mach &&
                    mach < regime.maximum_mach &&
                    mach >= aerodynamic_minimum_mach &&
                    mach <= aerodynamic_maximum_mach)
                {
                    selected.push_back(point);
                }
            }
            return selected;
        }
    }

    constraint_analysis_tool::constraint_analysis_tool(const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    constraint_output constraint_analysis_tool::run(const constraint_input& input) const
    {
        constraint_output output;

        if (input.propulsion == propulsion_type::jet && input.engine == nullptr)
        {
            throw std::runtime_error(
                "constraint_analysis_tool requires a valid UNICADO Engine pointer.");
        }

        jet_constraint_analysis jet_analysis(atmosphere_);

        // Vertical constraints are independent of the sampled W/S grid.  Use
        // them, together with the imported aircraft point, to expand the grid
        // before computing any curve.  This prevents matching-chart curves
        // from ending before a landing, stall, gust, or aircraft marker that
        // the plot must display.
        if (input.active.landing_field_length)
        {
            auto limit = jet_analysis.compute_landing_constraint_limit(input);
            if (input.propulsion == propulsion_type::propeller)
                limit.name = "propeller_landing_limit";
            output.vertical_constraints.push_back(std::move(limit));
        }
        if (input.active.stall_speed)
        {
            auto limit = jet_analysis.compute_stall_speed_constraint_limit(input);
            if (input.propulsion == propulsion_type::propeller)
                limit.name = "propeller_stall_speed_limit";
            output.vertical_constraints.push_back(std::move(limit));
        }
        if (input.active.gust)
        {
            auto limit = jet_analysis.compute_gust_constraint_limit(input);
            if (input.propulsion == propulsion_type::propeller)
                limit.name = "propeller_gust_limit";
            output.vertical_constraints.push_back(std::move(limit));
        }

        constraint_input sampled_input = input;
        if (!(sampled_input.wing_loading_step > 0.0) ||
            !(sampled_input.wing_loading_min > 0.0) ||
            sampled_input.wing_loading_max < sampled_input.wing_loading_min)
        {
            throw std::runtime_error(
                "Wing-loading design space must have positive min/step and max >= min.");
        }

        double required_min = sampled_input.wing_loading_min;
        double required_max = sampled_input.wing_loading_max;
        const auto include_wing_loading = [&](double value)
        {
            if (std::isfinite(value) && value > 0.0)
            {
                required_min = std::min(required_min, value);
                required_max = std::max(required_max, value);
            }
        };
        for (const auto& limit : output.vertical_constraints)
            include_wing_loading(limit.x_limit);
        if (input.aircraft.wing_area_m2 > 0.0)
        {
            include_wing_loading(
                input.aircraft.takeoff_weight_N / input.aircraft.wing_area_m2);
        }

        const double required_span = required_max - required_min;
        const double padding = std::max(
            sampled_input.wing_loading_step, 0.05 * required_span);
        sampled_input.wing_loading_min = std::max(
            sampled_input.wing_loading_step,
            std::floor((required_min - padding) /
                       sampled_input.wing_loading_step) *
                sampled_input.wing_loading_step);
        sampled_input.wing_loading_max =
            std::ceil((required_max + padding) /
                      sampled_input.wing_loading_step) *
            sampled_input.wing_loading_step;

        if (input.propulsion == propulsion_type::jet)
        {
            if (input.active.takeoff_ground_roll)
                output.curves.push_back(
                    jet_analysis.compute_takeoff_constraint(sampled_input));
            if (input.active.max_mach)
                output.curves.push_back(
                    jet_analysis.compute_max_mach_constraint(sampled_input));
            if (input.active.horizontal_acceleration)
                output.curves.push_back(
                    jet_analysis.compute_acceleration_constraint(sampled_input));
            for (const auto& regime : mission_mach_regimes)
            {
                if (input.active.cruise)
                {
                    auto regime_input = sampled_input;
                    regime_input.cruise.mission_points = points_in_regime(
                        sampled_input.cruise.mission_points, regime, atmosphere_,
                        sampled_input.aircraft.aerodynamic_minimum_mach,
                        sampled_input.aircraft.aerodynamic_maximum_mach);
                    if (!regime_input.cruise.mission_points.empty())
                    {
                        auto curve = jet_analysis.compute_cruise_constraint(regime_input);
                        curve.name = std::string("jet_") + regime.name +
                            "_cruise_constraint";
                        output.curves.push_back(std::move(curve));
                    }
                }

                if (input.active.climb)
                {
                    auto regime_input = sampled_input;
                    regime_input.climb.mission_points = points_in_regime(
                        sampled_input.climb.mission_points, regime, atmosphere_,
                        sampled_input.aircraft.aerodynamic_minimum_mach,
                        sampled_input.aircraft.aerodynamic_maximum_mach);
                    if (!regime_input.climb.mission_points.empty())
                    {
                        auto curve = jet_analysis.compute_climb_constraint(regime_input);
                        curve.name = std::string("jet_") + regime.name +
                            "_climb_constraint";
                        output.curves.push_back(std::move(curve));
                    }
                }
            }
            if (input.active.constant_speed_turn)
                output.curves.push_back(
                    jet_analysis.compute_turn_constraint(sampled_input));
        }
        else
        {
            if (input.propeller.model == nullptr)
            {
                throw std::runtime_error(
                    "Propeller analysis requires a valid aerodynamics::Propeller model.");
            }

            propeller_constraint_analysis propeller_analysis(atmosphere_);
            if (input.active.takeoff_ground_roll)
                output.curves.push_back(
                    propeller_analysis.compute_takeoff_constraint(sampled_input));
            if (input.active.horizontal_acceleration)
                output.curves.push_back(
                    propeller_analysis.compute_acceleration_constraint(sampled_input));
            for (const auto& regime : mission_mach_regimes)
            {
                auto regime_input = sampled_input;
                regime_input.cruise.mission_points = points_in_regime(
                    sampled_input.cruise.mission_points, regime, atmosphere_,
                    sampled_input.aircraft.aerodynamic_minimum_mach,
                    sampled_input.aircraft.aerodynamic_maximum_mach);
                regime_input.cruise.allow_configured_fallback =
                    sampled_input.cruise.allow_configured_fallback &&
                    std::string(regime.name) == "subsonic";
                if (input.active.cruise &&
                    !regime_input.cruise.mission_points.empty())
                {
                    auto curve = propeller_analysis.compute_cruise_constraint(
                        regime_input);
                    curve.name = std::string("propeller_") + regime.name +
                        "_cruise_constraint";
                    output.curves.push_back(std::move(curve));
                }

                regime_input = sampled_input;
                regime_input.climb.mission_points = points_in_regime(
                    sampled_input.climb.mission_points, regime, atmosphere_,
                    sampled_input.aircraft.aerodynamic_minimum_mach,
                    sampled_input.aircraft.aerodynamic_maximum_mach);
                if (input.active.climb &&
                    !regime_input.climb.mission_points.empty())
                {
                    auto curve = propeller_analysis.compute_climb_constraint(
                        regime_input);
                    curve.name = std::string("propeller_") + regime.name +
                        "_climb_constraint";
                    output.curves.push_back(std::move(curve));
                }
            }
            if (input.active.constant_speed_turn)
                output.curves.push_back(
                    propeller_analysis.compute_turn_constraint(sampled_input));
        }

        if (output.curves.empty())
            throw std::runtime_error(
                "At least one curve-producing constraint must be active.");

        if (input.propulsion == propulsion_type::jet &&
            input.active.range_fuel_fraction)
        {
            range_constraint_analysis range_analysis(atmosphere_);
            output.range_constraints.push_back(
                range_analysis.compute_range_constraint(sampled_input)
            );
        }

        return output;
    }
}
