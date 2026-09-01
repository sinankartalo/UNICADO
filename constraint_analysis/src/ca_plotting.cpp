#include "constraint_analysis/constraint_analysis.h"
#include "constraint_analysis/ca_minfinder.h"

#include "constraint_analysis/ca_plotting.h"


// ============================================================
// merged from: src/constraint_output_writer.cpp
// ============================================================
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <numbers>
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


// ============================================================
// merged from: src/carpet_plot_study.cpp
// ============================================================
#include <fstream>
#include <stdexcept>

namespace constraint_analysis
{
    jet_aerodynamic_carpet_study::jet_aerodynamic_carpet_study(
        const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    std::vector<jet_aerodynamic_carpet_point>
    jet_aerodynamic_carpet_study::run(
        const constraint_input& base_input,
        const std::vector<double>& cd0_values,
        const std::vector<double>& induced_drag_factor_values) const
    {
        std::vector<jet_aerodynamic_carpet_point> results;
        constraint_analysis_tool tool(atmosphere_);

        for (double cd0 : cd0_values)
        {
            for (double induced_drag_factor : induced_drag_factor_values)
            {
                constraint_input input = base_input;
                input.aircraft.operating_cd0_scale =
                    cd0 / base_input.aircraft.polar.cd_0;
                input.aircraft.operating_k_scale =
                    induced_drag_factor / base_input.aircraft.polar.k;
                input.aircraft.polar.cd_0 = cd0;
                input.aircraft.polar.k = induced_drag_factor;

                const constraint_output output = tool.run(input);
                const design_point feasible_point =
                    design_point_finder::find_interpolated_feasible_minimum_point(
                        output, output.vertical_constraints);

                std::string active_constraint_name;
                std::vector<std::pair<double, std::string>>
                    constraint_values;
                for (const auto& curve : output.curves)
                {
                    if (curve.points.empty())
                    {
                        continue;
                    }

                    const auto upper = std::lower_bound(
                        curve.points.begin(), curve.points.end(),
                        feasible_point.wing_loading,
                        [](const curve_point& point, double ws)
                        {
                            return point.x < ws;
                        });

                    double value = 0.0;
                    if (upper == curve.points.begin())
                    {
                        value = upper->y;
                    }
                    else if (upper == curve.points.end())
                    {
                        value = curve.points.back().y;
                    }
                    else
                    {
                        const auto& right = *upper;
                        const auto& left = *(upper - 1);
                        const double fraction =
                            (feasible_point.wing_loading - left.x) /
                            (right.x - left.x);
                        value = left.y + fraction * (right.y - left.y);
                    }

                    constraint_values.emplace_back(value, curve.name);
                }

                if (constraint_values.empty())
                {
                    throw std::runtime_error(
                        "Aerodynamic carpet point has no constraint values.");
                }
                std::sort(
                    constraint_values.begin(), constraint_values.end(),
                    [](const auto& left, const auto& right)
                    {
                        return left.first > right.first;
                    });
                active_constraint_name = constraint_values.front().second;
                const double active_constraint_value =
                    constraint_values.front().first;
                const std::string second_constraint_name =
                    constraint_values.size() > 1
                        ? constraint_values[1].second
                        : std::string{};
                const double second_constraint_value =
                    constraint_values.size() > 1
                        ? constraint_values[1].first
                        : active_constraint_value;
                const double constraint_margin =
                    active_constraint_value - second_constraint_value;

                bool range_feasible = true;
                for (const auto& range_result : output.range_constraints)
                {
                    if (range_result.points.empty())
                    {
                        range_feasible = false;
                        continue;
                    }

                    const auto upper = std::lower_bound(
                        range_result.points.begin(),
                        range_result.points.end(),
                        feasible_point.wing_loading,
                        [](const range_constraint_point& point, double ws)
                        {
                            return point.wing_loading < ws;
                        });

                    double required_fuel_fraction = 0.0;
                    if (upper == range_result.points.begin())
                    {
                        required_fuel_fraction = upper->required_fuel_fraction;
                    }
                    else if (upper == range_result.points.end())
                    {
                        required_fuel_fraction =
                            range_result.points.back().required_fuel_fraction;
                    }
                    else
                    {
                        const auto& right = *upper;
                        const auto& left = *(upper - 1);
                        const double fraction =
                            (feasible_point.wing_loading - left.wing_loading) /
                            (right.wing_loading - left.wing_loading);
                        required_fuel_fraction =
                            left.required_fuel_fraction + fraction *
                            (right.required_fuel_fraction -
                             left.required_fuel_fraction);
                    }

                    range_feasible = range_feasible &&
                        required_fuel_fraction <=
                            range_result.available_fuel_fraction;
                }

                const bool is_baseline =
                    std::abs(cd0 - base_input.aircraft.polar.cd_0) <=
                        1.0e-12 * std::max(
                            1.0, std::abs(base_input.aircraft.polar.cd_0)) &&
                    std::abs(
                        induced_drag_factor - base_input.aircraft.polar.k) <=
                        1.0e-12 * std::max(
                            1.0, std::abs(base_input.aircraft.polar.k));

                results.push_back({
                    cd0,
                    induced_drag_factor,
                    feasible_point.wing_loading,
                    feasible_point.thrust_to_weight,
                    range_feasible,
                    is_baseline,
                    active_constraint_name,
                    active_constraint_value,
                    second_constraint_name,
                    second_constraint_value,
                    constraint_margin});
            }
        }
        return results;
    }

    void jet_aerodynamic_carpet_study::write_to_csv(
        const std::vector<jet_aerodynamic_carpet_point>& points,
        const std::string& file_path)
    {
        std::ofstream file(file_path);
        if (!file.is_open())
            throw std::runtime_error(
                "Could not open jet aerodynamic carpet CSV file: " + file_path);

        file << "cd_0,induced_drag_factor,best_wing_loading,"
                "best_thrust_to_weight,range_feasible,is_baseline,"
                "active_constraint_name,active_constraint_value,"
                "second_constraint_name,second_constraint_value,"
                "constraint_margin\n";
        for (const auto& point : points)
        {
            file << point.cd_0 << ","
                 << point.induced_drag_factor << ","
                 << point.best_wing_loading << ","
                 << point.best_thrust_to_weight << ","
                 << point.range_feasible << ","
                 << point.is_baseline << ","
                 << point.active_constraint_name << ","
                 << point.active_constraint_value << ","
                 << point.second_constraint_name << ","
                 << point.second_constraint_value << ","
                 << point.constraint_margin << "\n";
        }
    }

    namespace
    {
        double jet_carpet_parameter_value(
            const constraint_input& input,
            jet_carpet_parameter parameter)
        {
            switch (parameter)
            {
                case jet_carpet_parameter::cd0:
                    return input.aircraft.polar.cd_0;
                case jet_carpet_parameter::induced_drag_factor:
                    return input.aircraft.polar.k;
                case jet_carpet_parameter::takeoff_distance_m:
                    return input.takeoff.runway_m;
                case jet_carpet_parameter::acceleration_severity_scale:
                    return 1.0;
                case jet_carpet_parameter::thrust_lapse_scale:
                    return input.installed_thrust_lapse_scale;
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
                case jet_carpet_parameter::cd0:
                    input.aircraft.operating_cd0_scale =
                        value / input.aircraft.polar.cd_0;
                    input.aircraft.polar.cd_0 = value;
                    return;
                case jet_carpet_parameter::induced_drag_factor:
                    input.aircraft.operating_k_scale =
                        value / input.aircraft.polar.k;
                    input.aircraft.polar.k = value;
                    return;
                case jet_carpet_parameter::takeoff_distance_m:
                    input.takeoff.runway_m = value;
                    return;
                case jet_carpet_parameter::acceleration_severity_scale:
                    // Preserve the mission operating conditions and scale
                    // only its kinematic specific-energy demand:
                    //   d(energy)/dt / W = ROC + V/g * dV/dt.
                    for (auto& point : input.acceleration.mission_points)
                    {
                        point.roc_ms *= value;
                        point.acceleration_ms2 *= value;
                    }
                    input.acceleration.acceleration_ms2 *= value;
                    return;
                case jet_carpet_parameter::thrust_lapse_scale:
                    input.installed_thrust_lapse_scale = value;
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
        if (base_input.propulsion != propulsion_type::jet)
        {
            throw std::runtime_error(
                "Jet two-parameter carpets require jet propulsion.");
        }
        if (parameter_a == parameter_b)
        {
            throw std::runtime_error(
                "Jet carpet parameters must be independent.");
        }

        const double baseline_a =
            jet_carpet_parameter_value(base_input, parameter_a);
        const double baseline_b =
            jet_carpet_parameter_value(base_input, parameter_b);
        constraint_analysis_tool tool(atmosphere_);
        std::vector<jet_two_parameter_carpet_point> results;
        results.reserve(parameter_a_values.size() * parameter_b_values.size());

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
                        "Jet carpet requires at least two constraints.");
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
            }
        }
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
                "Could not open jet two-parameter carpet CSV: " + file_path);
        }
        file << parameter_a_column << "," << parameter_b_column
             << ",best_wing_loading,best_thrust_to_weight,is_baseline,"
                "active_constraint_name,second_constraint_name,"
                "constraint_margin\n";
        for (const auto& point : points)
        {
            file << point.parameter_a_value << ","
                 << point.parameter_b_value << ","
                 << point.best_wing_loading << ","
                 << point.best_thrust_to_weight << ","
                 << point.is_baseline << ","
                 << point.active_constraint_name << ","
                 << point.second_constraint_name << ","
                 << point.constraint_margin << "\n";
        }
    }

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

            input.aircraft.operating_cd0_scale =
                cd0 / base_input.aircraft.polar.cd_0;
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

            input.aircraft.operating_cd0_scale =
                cd0 / base_input.aircraft.polar.cd_0;
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

    k_sensitivity_study::k_sensitivity_study(
        const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    std::vector<k_sensitivity_curve_point> k_sensitivity_study::run(
        const constraint_input& base_input,
        const std::vector<double>& induced_drag_factor_values) const
    {
        std::vector<k_sensitivity_curve_point> results;
        constraint_analysis_tool tool(atmosphere_);

        for (double induced_drag_factor : induced_drag_factor_values)
        {
            constraint_input input = base_input;
            input.aircraft.operating_k_scale =
                induced_drag_factor / base_input.aircraft.polar.k;
            input.aircraft.polar.k = induced_drag_factor;
            const constraint_output output = tool.run(input);

            const auto gust_limit = std::find_if(
                output.vertical_constraints.begin(),
                output.vertical_constraints.end(),
                [](const vertical_constraint& constraint)
                {
                    return constraint.name.find("gust") != std::string::npos;
                });
            if (gust_limit == output.vertical_constraints.end())
            {
                throw std::runtime_error(
                    "k sensitivity requires an active gust constraint.");
            }

            const auto acceleration_curve = std::find_if(
                output.curves.begin(), output.curves.end(),
                [](const constraint_curve& curve)
                {
                    return curve.name.find("acceleration") !=
                        std::string::npos;
                });
            if (acceleration_curve == output.curves.end())
            {
                throw std::runtime_error(
                    "k sensitivity requires an acceleration constraint curve.");
            }

            for (const auto& point : acceleration_curve->points)
            {
                results.push_back({
                    induced_drag_factor,
                    acceleration_curve->name,
                    point.x,
                    point.y,
                    gust_limit->x_limit});
            }
        }
        return results;
    }

    void k_sensitivity_study::write_to_csv(
        const std::vector<k_sensitivity_curve_point>& points,
        const std::string& file_path)
    {
        std::ofstream file(file_path);
        if (!file.is_open())
        {
            throw std::runtime_error(
                "Could not open k sensitivity CSV file: " + file_path);
        }

        file << "induced_drag_factor,constraint_name,wing_loading,"
                "thrust_to_weight,gust_wing_loading_limit\n";
        for (const auto& point : points)
        {
            file << point.induced_drag_factor << ","
                 << point.constraint_name << ","
                 << point.wing_loading << ","
                 << point.thrust_to_weight << ","
                 << point.gust_wing_loading_limit << "\n";
        }
    }
}
