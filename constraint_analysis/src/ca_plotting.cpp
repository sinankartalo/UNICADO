#include "constraint_analysis/constraint_analysis.h"
#include "constraint_analysis/ca_minfinder.h"

#include "constraint_analysis/ca_plotting.h"


// ============================================================
// merged from: src/constraint_output_writer.cpp
// ============================================================
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <utility>

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


#include <fstream>
#include <stdexcept>

namespace constraint_analysis
{
    namespace
    {
        double jet_carpet_parameter_value(
            const constraint_input& input,
            jet_carpet_parameter parameter)
        {
            switch (parameter)
            {
                case jet_carpet_parameter::takeoff_distance_m:
                    return input.takeoff.runway_m;
                case jet_carpet_parameter::acceleration_ms2:
                    if (input.acceleration.mission_points.size() != 1)
                        throw std::runtime_error(
                            "Acceleration carpet requires one explicit performance condition.");
                    return input.acceleration.mission_points.front().acceleration_ms2;
                case jet_carpet_parameter::climb_rate_ms:
                    if (input.climb.mission_points.size() != 1)
                        throw std::runtime_error(
                            "Climb-rate carpet requires one explicit performance condition.");
                    return input.climb.mission_points.front().roc_ms;
            }
            throw std::runtime_error("Unknown jet carpet parameter.");
        }

        void set_jet_carpet_parameter(
            constraint_input& input,
            jet_carpet_parameter parameter,
            double value)
        {
            if (!std::isfinite(value) || value <= 0.0)
            {
                throw std::runtime_error(
                    "Jet carpet parameters must be positive and finite.");
            }
            switch (parameter)
            {
                case jet_carpet_parameter::takeoff_distance_m:
                    input.takeoff.runway_m = value;
                    return;
                case jet_carpet_parameter::acceleration_ms2:
                    if (input.acceleration.mission_points.size() != 1)
                        throw std::runtime_error(
                            "Acceleration carpet requires performance condition_source.");
                    input.acceleration.mission_points.front().acceleration_ms2 = value;
                    input.acceleration.acceleration_ms2 = value;
                    return;
                case jet_carpet_parameter::climb_rate_ms:
                    if (input.climb.mission_points.size() != 1)
                        throw std::runtime_error(
                            "Climb-rate carpet requires performance condition_source.");
                    input.climb.mission_points.front().roc_ms = value;
                    input.climb.representative_point =
                        input.climb.mission_points.front();
                    return;
            }
            throw std::runtime_error("Unknown jet carpet parameter.");
        }

        double interpolate_constraint_at(
            const constraint_curve& curve,
            double wing_loading)
        {
            if (curve.points.empty())
            {
                throw std::runtime_error(
                    "Cannot evaluate an empty carpet constraint curve.");
            }
            const auto upper = std::lower_bound(
                curve.points.begin(), curve.points.end(), wing_loading,
                [](const curve_point& point, double ws)
                {
                    return point.x < ws;
                });
            if (upper == curve.points.begin())
                return upper->y;
            if (upper == curve.points.end())
                return curve.points.back().y;

            const auto& right = *upper;
            const auto& left = *(upper - 1);
            const double fraction =
                (wing_loading - left.x) / (right.x - left.x);
            return left.y + fraction * (right.y - left.y);
        }
    }

    jet_two_parameter_carpet_study::jet_two_parameter_carpet_study(
        const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    std::vector<jet_two_parameter_carpet_point>
    jet_two_parameter_carpet_study::run(
        const constraint_input& base_input,
        jet_carpet_parameter parameter_a,
        const std::vector<double>& parameter_a_values,
        jet_carpet_parameter parameter_b,
        const std::vector<double>& parameter_b_values) const
    {
        if (parameter_a == parameter_b)
        {
            throw std::runtime_error(
                "Two-parameter carpet inputs must be independent.");
        }

        const double baseline_a =
            jet_carpet_parameter_value(base_input, parameter_a);
        const double baseline_b =
            jet_carpet_parameter_value(base_input, parameter_b);
        constraint_analysis_tool tool(atmosphere_);
        std::vector<jet_two_parameter_carpet_point> results;
        const std::size_t total_points =
            parameter_a_values.size() * parameter_b_values.size();
        results.reserve(total_points);
        std::size_t completed_points = 0;

        for (double value_a : parameter_a_values)
        {
            for (double value_b : parameter_b_values)
            {
                constraint_input input = base_input;
                set_jet_carpet_parameter(input, parameter_a, value_a);
                set_jet_carpet_parameter(input, parameter_b, value_b);

                const constraint_output output = tool.run(input);
                const design_point optimum =
                    design_point_finder::find_interpolated_feasible_minimum_point(
                        output, output.vertical_constraints);

                std::vector<std::pair<double, std::string>> constraint_values;
                for (const auto& curve : output.curves)
                {
                    constraint_values.emplace_back(
                        interpolate_constraint_at(
                            curve, optimum.wing_loading),
                        curve.name);
                }
                if (constraint_values.size() < 2)
                {
                    throw std::runtime_error(
                        "Two-parameter carpet requires at least two constraints.");
                }
                std::sort(
                    constraint_values.begin(), constraint_values.end(),
                    [](const auto& left, const auto& right)
                    {
                        return left.first > right.first;
                    });

                const auto is_baseline_value = [](double value, double baseline)
                {
                    return std::abs(value - baseline) <=
                        1.0e-12 * std::max(1.0, std::abs(baseline));
                };
                results.push_back({
                    value_a,
                    value_b,
                    optimum.wing_loading,
                    optimum.thrust_to_weight,
                    is_baseline_value(value_a, baseline_a) &&
                        is_baseline_value(value_b, baseline_b),
                    constraint_values[0].second,
                    constraint_values[1].second,
                    constraint_values[0].first - constraint_values[1].first});

                ++completed_points;
                std::cout << "\rDesigner carpet progress: "
                          << completed_points << "/" << total_points
                          << std::flush;
            }
        }
        std::cout << '\n';
        return results;
    }

    void jet_two_parameter_carpet_study::write_to_csv(
        const std::vector<jet_two_parameter_carpet_point>& points,
        const std::string& parameter_a_column,
        const std::string& parameter_b_column,
        const std::string& file_path)
    {
        std::ofstream file(file_path);
        if (!file.is_open())
        {
            throw std::runtime_error(
                "Could not open two-parameter carpet CSV: " + file_path);
        }
        file << parameter_a_column << "," << parameter_b_column
             << ",best_wing_loading,best_required_loading,is_baseline,"
                "active_constraint_name,second_constraint_name,"
                "constraint_margin\n";
        for (const auto& point : points)
        {
            file << point.parameter_a_value << ","
                 << point.parameter_b_value << ","
                 << point.best_wing_loading << ","
                 << point.best_required_loading << ","
                 << point.is_baseline << ","
                 << point.active_constraint_name << ","
                 << point.second_constraint_name << ","
                 << point.constraint_margin << "\n";
        }
    }


}
