// This file merges the declarations that were previously split over multiple small headers.
#pragma once

#include "constraint_analysis/ca_functions.h"


// ============================================================
// merged from: design_point_finder.h
// ============================================================
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

        // Treats every constraint segment between two neighbouring grid points
        // as a straight line and also checks all curve intersections.
        // This removes the design-point error caused by a coarse W/S grid.
        static design_point find_interpolated_minimum_point(
            const constraint_output& output);

        // Same interpolation method, but restricted by vertical W/S limits.
        static design_point find_interpolated_feasible_minimum_point(
            const constraint_output& output,
            const std::vector<vertical_constraint>& vertical_constraints);
    };
}

// ============================================================
// merged from: constraint_envelope_analyzer.h
// ============================================================
namespace constraint_analysis
{
    class constraint_envelope_analyzer
    {
    public:
        static constraint_curve build_envelope(const constraint_output& output);
    };
}

// ============================================================
// merged from: feasible_design_point_finder.h
// ============================================================
namespace constraint_analysis
{
    class feasible_design_point_finder
    {
    public:
        static design_point find_feasible_minimum_point(
            const constraint_curve& envelope,
            const std::vector<vertical_constraint>& vertical_constraints);
    };
}

// ============================================================
// merged from: active_constraint_analyzer.h
// ============================================================
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
