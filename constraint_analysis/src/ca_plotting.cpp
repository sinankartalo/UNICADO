#include "constraint_analysis/constraint_analysis.h"
#include "constraint_analysis/ca_minfinder.h"

#include "constraint_analysis/ca_plotting.h"


// ============================================================
// merged from: src/constraint_output_writer.cpp
// ============================================================
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <numbers>

namespace constraint_analysis
{
    void constraint_output_writer::write_curve_to_csv(const constraint_curve& curve, const std::string& file_path)
    {
        std::ofstream file(file_path);

        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file for writing: " + file_path);
        }

        file << "x,y\n";

        for (const auto& point : curve.points)
        {
            file << point.x << "," << point.y << "\n";
        }
    }

    void constraint_output_writer::write_all_curves_to_csv(const constraint_output& output, const std::string& folder_path)
    {
        std::filesystem::create_directories(folder_path);

        for (const auto& curve : output.curves)
        {
            const std::string file_path = folder_path + "/" + curve.name + ".csv";
            write_curve_to_csv(curve, file_path);
        }
    }
}


// ============================================================
// merged from: src/carpet_plot_generator.cpp
// ============================================================
namespace constraint_analysis
{
    std::vector<carpet_point> carpet_plot_generator::generate(
        const std::vector<double>& parameter_a_values,
        const std::vector<double>& parameter_b_values) const
    {
        std::vector<carpet_point> points;

        for (double a : parameter_a_values)
        {
            for (double b : parameter_b_values)
            {
                carpet_point point;
                point.parameter_a = a;
                point.parameter_b = b;
                point.x = a;
                point.y = b;
                points.push_back(point);
            }
        }

        return points;
    }
}


// ============================================================
// merged from: src/carpet_plot_study.cpp
// ============================================================
#include <fstream>
#include <stdexcept>

namespace constraint_analysis
{
    carpet_plot_study::carpet_plot_study(const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    std::vector<carpet_study_point> carpet_plot_study::run(
        const constraint_input& base_input,
        const std::vector<double>& cd0_values) const
    {
        std::vector<carpet_study_point> results;

        constraint_analysis_tool tool(atmosphere_);

        for (double cd0 : cd0_values)
        {
            constraint_input input = base_input;

            input.aircraft.polar.cd_0 = cd0;

            const constraint_output output = tool.run(input);

            const constraint_curve envelope =
                constraint_envelope_analyzer::build_envelope(output);

            const design_point feasible_point =
                feasible_design_point_finder::find_feasible_minimum_point(
                    envelope,
                    output.vertical_constraints);

            bool range_feasible = true;

            for (const auto& range_result : output.range_constraints)
            {
                bool local_feasible = false;

                for (const auto& point : range_result.points)
                {
                    if (point.wing_loading == feasible_point.wing_loading)
                    {
                        local_feasible = point.feasible;
                        break;
                    }
                }

                range_feasible = range_feasible && local_feasible;
            }

            results.push_back({
                cd0,
                feasible_point.wing_loading,
                feasible_point.thrust_to_weight,
                range_feasible
            });
        }

        return results;
    }

    void carpet_plot_study::write_to_csv(
        const std::vector<carpet_study_point>& points,
        const std::string& file_path)
    {
        std::ofstream file(file_path);

        if (!file.is_open())
        {
            throw std::runtime_error("Could not open carpet plot CSV file: " + file_path);
        }

        file << "cd_0,best_wing_loading,best_thrust_to_weight,range_feasible\n";

        for (const auto& point : points)
        {
            file
                << point.cd_0 << ","
                << point.best_wing_loading << ","
                << point.best_thrust_to_weight << ","
                << point.range_feasible << "\n";
        }
    }
}


// ============================================================
// merged from: src/carpet_plot_full.cpp
// ============================================================
#include <fstream>
#include <stdexcept>

namespace constraint_analysis
{
    carpet_plot_full::carpet_plot_full(const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    std::vector<carpet_full_point> carpet_plot_full::run(
        const constraint_input& base_input,
        const std::vector<double>& cd0_values) const
    {
        std::vector<carpet_full_point> results;

        constraint_analysis_tool tool(atmosphere_);

        for (double cd0 : cd0_values)
        {
            constraint_input input = base_input;

            input.aircraft.polar.cd_0 = cd0;

            const constraint_output output = tool.run(input);

            const constraint_curve envelope =
                constraint_envelope_analyzer::build_envelope(output);

            for (const auto& point : envelope.points)
            {
                results.push_back({
                    cd0,
                    point.x,
                    point.y
                });
            }
        }

        return results;
    }

    void carpet_plot_full::write_to_csv(
        const std::vector<carpet_full_point>& points,
        const std::string& file_path)
    {
        std::ofstream file(file_path);

        if (!file.is_open())
        {
            throw std::runtime_error("Could not open full carpet plot CSV file: " + file_path);
        }

        file << "cd_0,wing_loading,thrust_to_weight\n";

        for (const auto& point : points)
        {
            file
                << point.cd_0 << ","
                << point.wing_loading << ","
                << point.thrust_to_weight << "\n";
        }
    }
}


// ============================================================
// merged from: src/true_carpet_constraints.cpp
// ============================================================
#include <fstream>
#include <stdexcept>

namespace constraint_analysis
{
    true_carpet_constraints::true_carpet_constraints(const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    std::vector<true_carpet_constraint_point> true_carpet_constraints::run(
        const constraint_input& base_input,
        const std::vector<double>& cd0_values) const
    {
        std::vector<true_carpet_constraint_point> results;

        constraint_analysis_tool tool(atmosphere_);

        for (double cd0 : cd0_values)
        {
            constraint_input input = base_input;

            input.aircraft.polar.cd_0 = cd0;

            const constraint_output output = tool.run(input);

            for (const auto& curve : output.curves)
            {
                for (const auto& point : curve.points)
                {
                    results.push_back({
                        cd0,
                        curve.name,
                        point.x,
                        point.y
                    });
                }
            }
        }

        return results;
    }

    void true_carpet_constraints::write_to_csv(
        const std::vector<true_carpet_constraint_point>& points,
        const std::string& file_path)
    {
        std::ofstream file(file_path);

        if (!file.is_open())
        {
            throw std::runtime_error("Could not open true carpet constraints CSV file: " + file_path);
        }

        file << "cd_0,constraint_name,wing_loading,thrust_to_weight\n";

        for (const auto& point : points)
        {
            file
                << point.cd_0 << ","
                << point.constraint_name << ","
                << point.wing_loading << ","
                << point.thrust_to_weight << "\n";
        }
    }
}
