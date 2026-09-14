// This file merges the declarations that were previously split over multiple small headers.
#pragma once

#include "constraint_analysis/ca_functions.h"

#include <atmosphere/atmosphere.h>


// ============================================================
// merged from: constraint_output_writer.h
// ============================================================
#include <string>

namespace constraint_analysis
{
    class constraint_output_writer
    {
    public:
        static void write_curve_to_csv(const constraint_curve& curve, const std::string& file_path);
        static void write_all_curves_to_csv(const constraint_output& output, const std::string& folder_path);
    };
}

#include <vector>
#include <string>

namespace constraint_analysis
{
    enum class jet_carpet_parameter
    {
        cd0,
        induced_drag_factor,
        takeoff_distance_m,
        acceleration_ms2,
        climb_rate_ms,
        acceleration_severity_scale,
        thrust_lapse_scale
    };

    struct jet_two_parameter_carpet_point
    {
        double parameter_a_value = 0.0;
        double parameter_b_value = 0.0;
        double best_wing_loading = 0.0;
        // T/W for jets and shaft P/W [W/N] for propellers.
        double best_required_loading = 0.0;
        bool is_baseline = false;
        std::string active_constraint_name;
        std::string second_constraint_name;
        double constraint_margin = 0.0;
    };

    class jet_two_parameter_carpet_study
    {
    public:
        explicit jet_two_parameter_carpet_study(
            const atmosphere& atmosphere);

        std::vector<jet_two_parameter_carpet_point> run(
            const constraint_input& base_input,
            jet_carpet_parameter parameter_a,
            const std::vector<double>& parameter_a_values,
            jet_carpet_parameter parameter_b,
            const std::vector<double>& parameter_b_values) const;

        static void write_to_csv(
            const std::vector<jet_two_parameter_carpet_point>& points,
            const std::string& parameter_a_column,
            const std::string& parameter_b_column,
            const std::string& file_path);

    private:
        const atmosphere& atmosphere_;
    };


}
