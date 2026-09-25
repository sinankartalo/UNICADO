#pragma once

#include "constraint_analysis/ca_functions.h"


// Design-point search
namespace constraint_analysis
{
    struct design_point
    {
        double wing_loading = 0.0;
        double thrust_to_weight = 0.0;
    };

    class design_point_finder
    {
    public:
        // Finds the minimum only among the sampled envelope points.
        static design_point find_minimum_point(const constraint_curve& envelope);

        // Checks line segments and curve intersections between grid points.
        static design_point find_interpolated_minimum_point(
            const constraint_output& output);

        // Applies the same search inside the vertical W/S limits.
        static design_point find_interpolated_feasible_minimum_point(
            const constraint_output& output,
            const std::vector<vertical_constraint>& vertical_constraints);
    };
}

// Constraint envelope
namespace constraint_analysis
{
    class constraint_envelope_analyzer
    {
    public:
        static constraint_curve build_envelope(const constraint_output& output);
    };
}

// Active-constraint identification
#include <string>
#include <vector>

namespace constraint_analysis
{
    struct active_constraint_point
    {
        double x = 0.0;
        double y = 0.0;
        std::string active_constraint_name;
    };

    class active_constraint_analyzer
    {
    public:
        static std::vector<active_constraint_point> analyze(const constraint_output& output);
    };
}
