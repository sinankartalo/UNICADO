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

        if (input.engine == nullptr)
        {
            throw std::runtime_error(
                "constraint_analysis_tool requires a valid UNICADO Engine pointer.");
        }

        jet_constraint_analysis jet_analysis(atmosphere_);

        output.curves.push_back(jet_analysis.compute_takeoff_constraint(input));
        output.curves.push_back(jet_analysis.compute_max_mach_constraint(input));
        output.curves.push_back(jet_analysis.compute_supercruise_constraint(input));
        output.curves.push_back(jet_analysis.compute_acceleration_constraint(input));
        output.curves.push_back(jet_analysis.compute_cruise_constraint(input));
        output.curves.push_back(jet_analysis.compute_climb_constraint(input));
        output.curves.push_back(jet_analysis.compute_turn_constraint(input));

        output.vertical_constraints.push_back(
            jet_analysis.compute_landing_constraint_limit(input)
        );

        output.vertical_constraints.push_back(
            jet_analysis.compute_stall_speed_constraint_limit(input)
        );

        output.vertical_constraints.push_back(
            jet_analysis.compute_gust_constraint_limit(input)
        );

        range_constraint_analysis range_analysis(atmosphere_);
        output.range_constraints.push_back(
            range_analysis.compute_range_constraint(input)
        );

        return output;
    }
}