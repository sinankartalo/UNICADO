#include "constraint_analysis/constraint_analysis.h"

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

        if (input.propulsion == propulsion_type::jet)
        {
            output.curves.push_back(jet_analysis.compute_takeoff_constraint(input));
            output.curves.push_back(jet_analysis.compute_max_mach_constraint(input));
            output.curves.push_back(jet_analysis.compute_supercruise_constraint(input));
            output.curves.push_back(jet_analysis.compute_acceleration_constraint(input));
            output.curves.push_back(jet_analysis.compute_cruise_constraint(input));
            output.curves.push_back(jet_analysis.compute_climb_constraint(input));
            output.curves.push_back(jet_analysis.compute_turn_constraint(input));
        }
        else
        {
            if (input.propeller.model == nullptr)
            {
                throw std::runtime_error(
                    "Propeller analysis requires a valid aerodynamics::Propeller model.");
            }

            propeller_constraint_analysis propeller_analysis(atmosphere_);
            output.curves.push_back(propeller_analysis.compute_takeoff_constraint(input));
            output.curves.push_back(propeller_analysis.compute_acceleration_constraint(input));
            output.curves.push_back(propeller_analysis.compute_cruise_constraint(input));
            output.curves.push_back(propeller_analysis.compute_climb_constraint(input));
            output.curves.push_back(propeller_analysis.compute_turn_constraint(input));
        }

        auto landing_limit = jet_analysis.compute_landing_constraint_limit(input);
        auto stall_limit = jet_analysis.compute_stall_speed_constraint_limit(input);
        auto gust_limit = jet_analysis.compute_gust_constraint_limit(input);
        if (input.propulsion == propulsion_type::propeller)
        {
            landing_limit.name = "propeller_landing_limit";
            stall_limit.name = "propeller_stall_speed_limit";
            gust_limit.name = "propeller_gust_limit";
        }
        output.vertical_constraints.push_back(landing_limit);
        output.vertical_constraints.push_back(stall_limit);
        output.vertical_constraints.push_back(gust_limit);

        if (input.propulsion == propulsion_type::jet)
        {
            range_constraint_analysis range_analysis(atmosphere_);
            output.range_constraints.push_back(
                range_analysis.compute_range_constraint(input)
            );
        }

        return output;
    }
}
