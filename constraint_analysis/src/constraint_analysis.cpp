#include "constraint_analysis/constraint_analysis.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace constraint_analysis
{
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
        auto landing_limit = jet_analysis.compute_landing_constraint_limit(input);
        auto stall_limit = jet_analysis.compute_stall_speed_constraint_limit(input);
        auto gust_limit = jet_analysis.compute_gust_constraint_limit(input);
        if (input.propulsion == propulsion_type::propeller)
        {
            landing_limit.name = "propeller_landing_limit";
            stall_limit.name = "propeller_stall_speed_limit";
            gust_limit.name = "propeller_gust_limit";
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
        include_wing_loading(landing_limit.x_limit);
        include_wing_loading(stall_limit.x_limit);
        include_wing_loading(gust_limit.x_limit);
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
            output.curves.push_back(jet_analysis.compute_takeoff_constraint(sampled_input));
            output.curves.push_back(jet_analysis.compute_max_mach_constraint(sampled_input));
            output.curves.push_back(jet_analysis.compute_acceleration_constraint(sampled_input));
            output.curves.push_back(jet_analysis.compute_cruise_constraint(sampled_input));
            output.curves.push_back(jet_analysis.compute_climb_constraint(sampled_input));
            output.curves.push_back(jet_analysis.compute_turn_constraint(sampled_input));
        }
        else
        {
            if (input.propeller.model == nullptr)
            {
                throw std::runtime_error(
                    "Propeller analysis requires a valid aerodynamics::Propeller model.");
            }

            propeller_constraint_analysis propeller_analysis(atmosphere_);
            output.curves.push_back(propeller_analysis.compute_takeoff_constraint(sampled_input));
            output.curves.push_back(propeller_analysis.compute_acceleration_constraint(sampled_input));
            output.curves.push_back(propeller_analysis.compute_cruise_constraint(sampled_input));
            output.curves.push_back(propeller_analysis.compute_climb_constraint(sampled_input));
            output.curves.push_back(propeller_analysis.compute_turn_constraint(sampled_input));
        }

        output.vertical_constraints.push_back(landing_limit);
        output.vertical_constraints.push_back(stall_limit);
        output.vertical_constraints.push_back(gust_limit);

        if (input.propulsion == propulsion_type::jet)
        {
            range_constraint_analysis range_analysis(atmosphere_);
            output.range_constraints.push_back(
                range_analysis.compute_range_constraint(sampled_input)
            );
        }

        return output;
    }
}
