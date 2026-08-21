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

// ============================================================
// merged from: carpet_plot_generator.h
// ============================================================
#include <vector>

namespace constraint_analysis
{
    class carpet_plot_generator
    {
    public:
        std::vector<carpet_point> generate(
            const std::vector<double>& parameter_a_values,
            const std::vector<double>& parameter_b_values) const;
    };
}

// ============================================================
// merged from: carpet_plot_study.h
// ============================================================
#include <vector>
#include <string>

namespace constraint_analysis
{
    struct jet_aerodynamic_carpet_point
    {
        double cd_0 = 0.0;
        double induced_drag_factor = 0.0;
        double best_wing_loading = 0.0;
        double best_thrust_to_weight = 0.0;
        bool range_feasible = false;
        bool is_baseline = false;
        double boundary_relaxation_fraction = 0.0;
        bool original_boundaries_feasible = false;
        std::string active_constraint_name;
    };

    class jet_aerodynamic_carpet_study
    {
    public:
        explicit jet_aerodynamic_carpet_study(const atmosphere& atmosphere);

        std::vector<jet_aerodynamic_carpet_point> run(
            const constraint_input& base_input,
            const std::vector<double>& cd0_values,
            const std::vector<double>& induced_drag_factor_values,
            double boundary_relaxation_fraction = 0.0) const;

        static void write_to_csv(
            const std::vector<jet_aerodynamic_carpet_point>& points,
            const std::string& file_path);

    private:
        const atmosphere& atmosphere_;
    };

    struct carpet_study_point
    {
        double cd_0 = 0.0;
        double best_wing_loading = 0.0;
        double best_thrust_to_weight = 0.0;
        bool range_feasible = false;
    };

    class carpet_plot_study
    {
    public:
        explicit carpet_plot_study(const atmosphere& atmosphere);

        std::vector<carpet_study_point> run(
            const constraint_input& base_input,
            const std::vector<double>& cd0_values) const;

        static void write_to_csv(
            const std::vector<carpet_study_point>& points,
            const std::string& file_path);

    private:
        const atmosphere& atmosphere_;
    };
}

// ============================================================
// merged from: carpet_plot_full.h
// ============================================================
#include <string>
#include <vector>

namespace constraint_analysis
{
    struct carpet_full_point
    {
        double cd_0 = 0.0;
        double wing_loading = 0.0;
        double thrust_to_weight = 0.0;
    };

    class carpet_plot_full
    {
    public:
        explicit carpet_plot_full(const atmosphere& atmosphere);

        std::vector<carpet_full_point> run(
            const constraint_input& base_input,
            const std::vector<double>& cd0_values) const;

        static void write_to_csv(
            const std::vector<carpet_full_point>& points,
            const std::string& file_path);

    private:
        const atmosphere& atmosphere_;
    };
}

// ============================================================
// merged from: true_carpet_constraints.h
// ============================================================
#include <string>
#include <vector>

namespace constraint_analysis
{
    struct true_carpet_constraint_point
    {
        double cd_0 = 0.0;
        std::string constraint_name;
        double wing_loading = 0.0;
        double thrust_to_weight = 0.0;
    };

    class true_carpet_constraints
    {
    public:
        explicit true_carpet_constraints(const atmosphere& atmosphere);

        std::vector<true_carpet_constraint_point> run(
            const constraint_input& base_input,
            const std::vector<double>& cd0_values) const;

        static void write_to_csv(
            const std::vector<true_carpet_constraint_point>& points,
            const std::string& file_path);

    private:
        const atmosphere& atmosphere_;
    };

    struct k_sensitivity_curve_point
    {
        double induced_drag_factor = 0.0;
        std::string constraint_name;
        double wing_loading = 0.0;
        double thrust_to_weight = 0.0;
        double gust_wing_loading_limit = 0.0;
    };

    class k_sensitivity_study
    {
    public:
        explicit k_sensitivity_study(const atmosphere& atmosphere);

        std::vector<k_sensitivity_curve_point> run(
            const constraint_input& base_input,
            const std::vector<double>& induced_drag_factor_values) const;

        static void write_to_csv(
            const std::vector<k_sensitivity_curve_point>& points,
            const std::string& file_path);

    private:
        const atmosphere& atmosphere_;
    };
}
