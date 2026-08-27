#include "constraint_analysis/constraint_analysis.h"
#include "constraint_analysis/ca_parser.h"
#include "atmosphere/atmosphere.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using namespace constraint_analysis;

namespace
{
    double interpolate_envelope_thrust_to_weight(
        const constraint_curve& envelope,
        double wing_loading)
    {
        if (envelope.points.empty())
        {
            throw std::runtime_error(
                "Cannot evaluate the aircraft point because the constraint envelope is empty.");
        }

        if (wing_loading < envelope.points.front().x ||
            wing_loading > envelope.points.back().x)
        {
            throw std::runtime_error(
                "Aircraft wing loading from the aerodynamic reference area lies "
                "outside the configured wing-loading design space.");
        }

        const auto right = std::lower_bound(
            envelope.points.begin(),
            envelope.points.end(),
            wing_loading,
            [](const curve_point& point, double value)
            {
                return point.x < value;
            });

        if (right == envelope.points.begin())
        {
            return right->y;
        }

        if (right == envelope.points.end())
        {
            return envelope.points.back().y;
        }

        if (std::abs(right->x - wing_loading) < 1.0e-12)
        {
            return right->y;
        }

        const auto left = std::prev(right);
        const double ratio =
            (wing_loading - left->x) / (right->x - left->x);

        return left->y + ratio * (right->y - left->y);
    }

    bool satisfies_vertical_constraints(
        double wing_loading,
        const std::vector<vertical_constraint>& constraints)
    {
        for (const auto& constraint : constraints)
        {
            if (constraint.is_upper_limit && wing_loading > constraint.x_limit)
            {
                return false;
            }

            if (!constraint.is_upper_limit && wing_loading < constraint.x_limit)
            {
                return false;
            }
        }

        return true;
    }

    const constraint_curve& find_curve(
        const constraint_output& output,
        const std::string& name)
    {
        const auto found = std::find_if(
            output.curves.begin(), output.curves.end(),
            [&](const constraint_curve& curve) { return curve.name == name; });
        if (found == output.curves.end())
            throw std::runtime_error("Missing constraint curve: " + name);
        return *found;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        const std::filesystem::path output_root = "output";
        std::filesystem::create_directories(output_root);

        // Command-line arguments are optional.
        // Supported forms:
        //   app.exe
        //   app.exe CASE_ID
        //   app.exe "config\\another_config.xml"
        //   app.exe "config\\another_config.xml" CASE_ID
        //
        // The engine directory is always read from engine/engine_directory_path
        // in the selected XML constraint case.
        std::filesystem::path config_path = "config/constraint_analysis_conf.xml";
        std::string case_override_id;

        if (argc > 3)
        {
            throw std::runtime_error(
                "Too many command-line arguments. Use: app.exe [config.xml] [case_ID] "
                "or app.exe [case_ID].");
        }

        if (argc >= 2)
        {
            const std::filesystem::path first_argument = argv[1];

            if (first_argument.extension() == ".xml")
            {
                config_path = first_argument;
                case_override_id = (argc == 3) ? argv[2] : "";
            }
            else
            {
                case_override_id = argv[1];

                if (argc == 3)
                {
                    throw std::runtime_error(
                        "When the first argument is a case ID, no second argument is allowed. "
                        "To use another XML file, provide the XML path first and the case ID second.");
                }
            }
        }

        const auto config = read_xml_config(config_path, case_override_id);
        const std::string active_case_id =
            xml_string(config, "active_constraint_case_id");
        const std::filesystem::path output_directory =
            output_root / active_case_id;
        std::filesystem::create_directories(output_directory);
        std::filesystem::remove(
            output_directory / "carpet_plot_full.csv");

        std::cout << "Using XML configuration: " << config_path << '\n';
        std::cout << "Using constraint case ID: " << active_case_id << '\n';
        std::cout << "Case output directory: "
                  << output_directory.string() << '\n';

        const bool is_propeller =
            xml_string(config, "propulsion_type") == "propeller";

        std::unique_ptr<Engine> unicado_engine;
        std::unique_ptr<aerodynamics::Propeller> propeller_model;

        if (!is_propeller)
        {
            const std::filesystem::path engine_directory =
                xml_string(config, "engine_directory_path");
            if (engine_directory.empty())
            {
                throw std::runtime_error("Engine directory is empty.");
            }
            unicado_engine = std::make_unique<Engine>(engine_directory);
            std::cout << "Using UNICADO engine library: "
                      << engine_directory.string() << '\n';
        }
        else
        {
            const std::string propeller_deck =
                xml_string(config, "propeller_deck_path");
            const double diameter_m =
                xml_double(config, "propeller_diameter_m");
            propeller_model = std::make_unique<aerodynamics::Propeller>(
                propeller_deck, diameter_m);
            std::cout << "Using UNICADO aerodynamics Propeller deck: "
                      << propeller_deck << '\n';
        }

        constraint_input input =
            build_constraint_input_from_config(config, unicado_engine.get());

        input.propeller.model = propeller_model.get();
        {
            std::ofstream metadata(output_directory / "analysis_metadata.csv");
            metadata << "case_id,propulsion_type,y_axis,y_unit\n";
            metadata << active_case_id << ","
                     << (is_propeller ? "propeller" : "jet") << ","
                     << (is_propeller ? "shaft_power_to_weight" : "thrust_to_weight")
                     << "," << (is_propeller ? "W/N" : "-") << "\n";
        }
        std::cout << "Using UNICADO atmosphere library.\n";
        if (is_propeller)
        {
            std::cout << "Matching-chart y-axis: required shaft P/W [W/N].\n";
            std::cout << "Propeller deck status: supplied authoritative analysis data.\n";

            // Do not let files from an earlier jet run masquerade as propeller
            // evidence when the plotting script is run in the same directory.
            for (const char* stale_file : {
                     "carpet_plot_study.csv", "true_carpet_constraints.csv",
                     "jet_cd0_k_carpet.csv",
                     "jet_k_sensitivity_curves.csv",
                     "jet_range_fuel_fraction_constraint.csv",
                     "propeller_model_limitations.csv"})
            {
                std::filesystem::remove(
                    output_directory / stale_file);
            }
        }
        else
        {
            std::cout << "Thrust lapse and TSFC are read from the UNICADO Engine deck.\n";
        }

        // ============================================================
        // 1. RUN CONSTRAINT ANALYSIS
        // ============================================================
        atmosphere atm;
        constraint_analysis_tool tool{atm};

        const constraint_output output = tool.run(input);

        if (is_propeller)
        {
            propeller_constraint_analysis propeller_analysis{atm};
            const propeller_climb_coverage coverage =
                propeller_analysis.assess_climb_coverage(input);

            std::ofstream coverage_file(
                output_directory / "propeller_climb_mission_coverage.csv");
            coverage_file <<
                "coverage_status,total_mission_points,valid_deck_points,"
                "invalid_deck_points,coverage_fraction,"
                "first_invalid_altitude_m,first_invalid_speed_ms,"
                "first_invalid_advance_ratio,first_invalid_reason\n";
            const double coverage_fraction =
                coverage.total_mission_points == 0
                    ? 0.0
                    : static_cast<double>(coverage.valid_deck_points) /
                        static_cast<double>(coverage.total_mission_points);
            coverage_file
                << (coverage.invalid_deck_points == 0
                        ? "full_mission_coverage"
                        : "partial_mission_coverage") << ","
                << coverage.total_mission_points << ","
                << coverage.valid_deck_points << ","
                << coverage.invalid_deck_points << ","
                << coverage_fraction << ","
                << coverage.first_invalid_altitude_m << ","
                << coverage.first_invalid_speed_ms << ","
                << coverage.first_invalid_advance_ratio << ",\""
                << coverage.first_invalid_reason << "\"\n";

            std::ofstream climb_points_file(
                output_directory / "propeller_climb_operating_points.csv");
            climb_points_file <<
                "altitude_m,speed_ms,roc_ms,acceleration_ms2,beta_climb,"
                "rpm,pitch_deg,advance_ratio,CT,CP,eta,tip_mach,"
                "tip_mach_limit,status\n";
            for (const auto& mission_point : input.climb.mission_points)
            {
                try
                {
                    const auto point =
                        propeller_analysis.select_best_airborne_operating_point(
                            input,
                            mission_point.altitude_m,
                            mission_point.speed_ms);
                    climb_points_file
                        << mission_point.altitude_m << ","
                        << mission_point.speed_ms << ","
                        << mission_point.roc_ms << ","
                        << mission_point.acceleration_ms2 << ","
                        << mission_point.beta_climb << ","
                        << point.rpm << "," << point.pitch_deg << ","
                        << point.advance_ratio << ","
                        << point.thrust_coefficient << ","
                        << point.power_coefficient << ","
                        << point.efficiency << "," << point.tip_mach << ","
                        << point.tip_mach_limit << ",selected\n";
                }
                catch (const std::exception&)
                {
                    climb_points_file
                        << mission_point.altitude_m << ","
                        << mission_point.speed_ms << ","
                        << mission_point.roc_ms << ","
                        << mission_point.acceleration_ms2 << ","
                        << mission_point.beta_climb
                        << ",,,,,,,,,outside_automatic_selection_domain\n";
                }
            }

            if (coverage.invalid_deck_points > 0)
            {
                std::cout
                    << "WARNING: Propeller climb uses partial mission coverage: "
                    << coverage.valid_deck_points << "/"
                    << coverage.total_mission_points
                    << " points are inside the supplied deck. First invalid: "
                    << "altitude=" << coverage.first_invalid_altitude_m
                    << " m, V=" << coverage.first_invalid_speed_ms
                    << " m/s, J=" << coverage.first_invalid_advance_ratio
                    << ". See propeller_climb_mission_coverage.csv.\n";
            }
        }

        // ============================================================
        // 2. POST-PROCESSING
        // ============================================================
        const constraint_curve envelope =
            constraint_envelope_analyzer::build_envelope(output);

        const design_point sampled_best_point =
            design_point_finder::find_minimum_point(envelope);

        const design_point best_point =
            design_point_finder::find_interpolated_minimum_point(output);

        const design_point feasible_best_point =
            design_point_finder::find_interpolated_feasible_minimum_point(
                output,
                output.vertical_constraints);

        if (input.aircraft.wing_area_m2 <= 0.0)
        {
            throw std::runtime_error(
                "Aerodynamic reference wing area must be positive.");
        }

        // The aircraft wing area is an authoritative input from the UNICADO
        // aerodynamics model. The matching chart therefore evaluates the
        // existing aircraft at W_TO / S_ref instead of sizing a new wing area
        // from the envelope optimum.
        const double aircraft_wing_loading =
            input.aircraft.takeoff_weight_N / input.aircraft.wing_area_m2;
        const double aircraft_required_thrust_to_weight =
            interpolate_envelope_thrust_to_weight(
                envelope,
                aircraft_wing_loading);
        const bool aircraft_wing_loading_feasible =
            satisfies_vertical_constraints(
                aircraft_wing_loading,
                output.vertical_constraints);

        const auto active_constraints =
            active_constraint_analyzer::analyze(output);

        // ============================================================
        // 3. WRITE MAIN CSV OUTPUTS
        // ============================================================
        constraint_output_writer::write_all_curves_to_csv(
            output, output_directory.string());

        constraint_output envelope_output;
        envelope_output.curves.push_back(envelope);
        constraint_output_writer::write_all_curves_to_csv(
            envelope_output, output_directory.string());

        // Keep both engineering interpretations in one traceable plotting
        // interface: the existing aircraft from aerodynamic Sref and the
        // minimum feasible point proposed by the constraint analysis.
        {
            std::ofstream file(output_directory / "design_point.csv");
            file << "wing_loading,"
                    << (is_propeller ? "shaft_power_to_weight" : "thrust_to_weight")
                    << ",wing_area_m2,";
            if (is_propeller)
                file << "required_total_shaft_power_W,";
            file << "vertical_constraints_feasible,method\n";
            file << aircraft_wing_loading << ","
                 << aircraft_required_thrust_to_weight << ","
                 << input.aircraft.wing_area_m2 << ",";
            if (is_propeller)
                file << aircraft_required_thrust_to_weight *
                            input.aircraft.takeoff_weight_N << ",";
            file << aircraft_wing_loading_feasible << ","
                 << "aerodynamics_reference_area\n";
            file << feasible_best_point.wing_loading << ","
                 << feasible_best_point.thrust_to_weight << ","
                 << input.aircraft.takeoff_weight_N /
                        feasible_best_point.wing_loading << ",";
            if (is_propeller)
                file << feasible_best_point.thrust_to_weight *
                            input.aircraft.takeoff_weight_N << ",";
            file << true << "," << "best_design_point\n";
        }

        for (const auto& vc : output.vertical_constraints)
        {
            std::ofstream file(output_directory / (vc.name + ".csv"));
            file << "W/S_limit,type\n";
            file << vc.x_limit << ","
                 << (vc.is_upper_limit ? "max" : "min") << "\n";
        }

        for (const auto& range_result : output.range_constraints)
        {
            std::ofstream file(output_directory / (range_result.name + ".csv"));
            file << "wing_loading,lift_to_drag,required_fuel_fraction,feasible\n";

            for (const auto& point : range_result.points)
            {
                file << point.wing_loading << ","
                     << point.lift_to_drag << ","
                     << point.required_fuel_fraction << ","
                     << point.feasible << "\n";
            }
        }

        if (is_propeller)
        {
            std::ofstream carpet(
                output_directory / "propeller_constraint_carpet.csv");
            carpet << "constraint,wing_loading_N_m2,"
                       "required_shaft_power_to_weight_W_N,"
                       "required_total_shaft_power_W\n";
            for (const auto& curve : output.curves)
            {
                for (const auto& point : curve.points)
                {
                    carpet << curve.name << "," << point.x << "," << point.y
                           << "," << point.y * input.aircraft.takeoff_weight_N
                           << "\n";
                }
            }
        }

        // ============================================================
        // 4. CARPET PLOT PARAMETER STUDY
        // ============================================================
        // All single- and two-parameter aerodynamic studies use the same
        // relative band around the imported aircraft polar.
        const std::vector<double> sensitivity_factors = {
            0.80, 0.85, 0.90, 0.95, 1.00,
            1.05, 1.10, 1.15, 1.20};
        std::vector<double> cd0_values;
        for (double factor : sensitivity_factors)
        {
            cd0_values.push_back(input.aircraft.polar.cd_0 * factor);
        }

        std::vector<carpet_study_point> carpet_points;
        std::vector<true_carpet_constraint_point> true_carpet_points;
        std::vector<jet_aerodynamic_carpet_point> jet_aero_carpet_points;
        std::vector<k_sensitivity_curve_point> k_sensitivity_points;

        // Use the same imported-polar-relative grid for both propulsion
        // architectures.  The analysis tool selects T/W or P/W equations
        // from the configured propulsion type; the sweep itself is common.
        std::vector<double> cd0_carpet_values;
        std::vector<double> induced_drag_factor_values;
        for (double factor : sensitivity_factors)
        {
            cd0_carpet_values.push_back(
                input.aircraft.polar.cd_0 * factor);
            induced_drag_factor_values.push_back(
                input.aircraft.polar.k * factor);
        }

        if (!is_propeller)
        {
            carpet_plot_study study{atm};
            carpet_points = study.run(input, cd0_values);
            carpet_plot_study::write_to_csv(
                carpet_points,
                (output_directory / "carpet_plot_study.csv").string());

            true_carpet_constraints true_carpet{atm};
            true_carpet_points = true_carpet.run(input, cd0_values);
            true_carpet_constraints::write_to_csv(
                true_carpet_points,
                (output_directory / "true_carpet_constraints.csv").string());

            // Use a relative sensitivity band around the authoritative polar
            // rather than replacing it with unrelated absolute assumptions.
            jet_aerodynamic_carpet_study aero_carpet{atm};
            jet_aero_carpet_points = aero_carpet.run(
                input, cd0_carpet_values, induced_drag_factor_values);
            jet_aerodynamic_carpet_study::write_to_csv(
                jet_aero_carpet_points,
                (output_directory / "jet_cd0_k_carpet.csv").string());

            const auto scaled_values = [&sensitivity_factors](double nominal)
            {
                std::vector<double> values;
                values.reserve(sensitivity_factors.size());
                for (double factor : sensitivity_factors)
                    values.push_back(nominal * factor);
                return values;
            };
            const auto takeoff_distance_values =
                scaled_values(input.takeoff.runway_m);
            const auto acceleration_requirement_values =
                scaled_values(input.acceleration.acceleration_ms2);
            const std::vector<double> thrust_lapse_scale_values =
                sensitivity_factors;

            jet_two_parameter_carpet_study paired_carpet{atm};
            const auto cd0_takeoff_points = paired_carpet.run(
                input,
                jet_carpet_parameter::cd0, cd0_carpet_values,
                jet_carpet_parameter::takeoff_distance_m,
                takeoff_distance_values);
            jet_two_parameter_carpet_study::write_to_csv(
                cd0_takeoff_points, "cd_0", "takeoff_distance_m",
                (output_directory /
                    "jet_cd0_takeoff_distance_carpet.csv").string());

            const auto k_acceleration_points = paired_carpet.run(
                input,
                jet_carpet_parameter::induced_drag_factor,
                induced_drag_factor_values,
                jet_carpet_parameter::acceleration_requirement_ms2,
                acceleration_requirement_values);
            jet_two_parameter_carpet_study::write_to_csv(
                k_acceleration_points, "induced_drag_factor",
                "acceleration_requirement_ms2",
                (output_directory /
                    "jet_k_acceleration_carpet.csv").string());

            const auto cd0_lapse_points = paired_carpet.run(
                input,
                jet_carpet_parameter::cd0, cd0_carpet_values,
                jet_carpet_parameter::thrust_lapse_scale,
                thrust_lapse_scale_values);
            jet_two_parameter_carpet_study::write_to_csv(
                cd0_lapse_points, "cd_0", "thrust_lapse_scale",
                (output_directory /
                    "jet_cd0_thrust_lapse_carpet.csv").string());

            k_sensitivity_study k_sensitivity{atm};
            k_sensitivity_points = k_sensitivity.run(
                input, induced_drag_factor_values);
            k_sensitivity_study::write_to_csv(
                k_sensitivity_points,
                (output_directory /
                    "jet_k_sensitivity_curves.csv").string());
        }
        else
        {
            true_carpet_constraints cd0_sensitivity{atm};
            true_carpet_points = cd0_sensitivity.run(
                input, cd0_carpet_values);
            true_carpet_constraints::write_to_csv(
                true_carpet_points,
                (output_directory /
                    "propeller_cd0_sensitivity_curves.csv").string());

            jet_aerodynamic_carpet_study aero_carpet{atm};
            jet_aero_carpet_points = aero_carpet.run(
                input, cd0_carpet_values, induced_drag_factor_values);
            jet_aerodynamic_carpet_study::write_to_csv(
                jet_aero_carpet_points,
                (output_directory /
                    "propeller_cd0_k_carpet.csv").string());

            k_sensitivity_study k_sensitivity{atm};
            k_sensitivity_points = k_sensitivity.run(
                input, induced_drag_factor_values);
            k_sensitivity_study::write_to_csv(
                k_sensitivity_points,
                (output_directory /
                    "propeller_k_sensitivity_curves.csv").string());
        }

        // ============================================================
        // 5. PROPELLER OPERATING-POINT EVIDENCE
        // ============================================================
        if (is_propeller)
        {
            propeller_constraint_analysis propeller_analysis{atm};
            const auto map_points =
                propeller_constraint_analysis::read_performance_map(
                    input.propeller.deck_path);
            {
                std::ofstream map_file(
                    output_directory / "propeller_performance_map.csv");
                map_file << "pitch_deg,advance_ratio,thrust_coefficient,"
                            "power_coefficient,efficiency\n";
                for (const auto& point : map_points)
                {
                    map_file << point.pitch_deg << ","
                             << point.advance_ratio << ","
                             << point.thrust_coefficient << ","
                             << point.power_coefficient << ","
                             << point.efficiency << "\n";
                }
            }
            const std::vector<propeller_operating_point> points = {
                propeller_analysis.select_best_airborne_operating_point(
                    input, input.cruise.altitude_m, input.cruise.speed_ms),
                propeller_analysis.select_best_airborne_operating_point(
                    input,
                    input.climb.representative_point.altitude_m,
                    input.climb.representative_point.speed_ms),
                propeller_analysis.select_best_airborne_operating_point(
                    input, input.turn.altitude_m, input.turn.speed_ms),
                propeller_analysis.select_best_airborne_operating_point(
                    input, input.acceleration.altitude_m,
                    input.acceleration.speed_ms)
            };

            std::ofstream file(
                output_directory / "propeller_operating_points.csv");
            file << "altitude_m,speed_ms,density_kg_m3,rpm,pitch_deg,"
                    "advance_ratio,CT,CP,eta,thrust_N,shaft_power_W,"
                    "speed_of_sound_ms,tip_speed_ms,tip_mach,"
                    "tip_mach_limit,tip_mach_status\n";
            for (const auto& point : points)
            {
                file << point.altitude_m << "," << point.speed_ms << ","
                     << point.density_kg_m3 << "," << point.rpm << ","
                     << point.pitch_deg << "," << point.advance_ratio << ","
                     << point.thrust_coefficient << ","
                     << point.power_coefficient << "," << point.efficiency << ","
                     << point.thrust_N << "," << point.shaft_power_W << ","
                     << point.speed_of_sound_ms << "," << point.tip_speed_ms << ","
                     << point.tip_mach << "," << point.tip_mach_limit << ","
                     << (point.tip_mach_feasible ? "PASS" : "FAIL") << "\n";
            }

            const auto takeoff_result =
                propeller_analysis.solve_takeoff_ground_roll(
                    input, feasible_best_point.wing_loading, true);
            {
                std::ofstream profile(
                    output_directory / "propeller_takeoff_profile.csv");
                profile << "wing_loading_N_m2,speed_ms,distance_m,"
                           "acceleration_ms2,lift_N,drag_N,"
                           "rolling_resistance_N,required_thrust_N,"
                           "required_total_shaft_power_W,advance_ratio,CT,CP,"
                           "eta,deck_single_thrust_N,deck_single_shaft_power_W,"
                           "deck_total_thrust_N,deck_total_shaft_power_W,"
                           "speed_of_sound_ms,tip_speed_ms,tip_mach,"
                           "tip_mach_limit,tip_mach_status\n";
                for (const auto& step : takeoff_result.steps)
                {
                    profile << takeoff_result.wing_loading_N_m2 << ","
                            << step.speed_ms << "," << step.distance_m << ","
                            << step.acceleration_ms2 << "," << step.lift_N << ","
                            << step.drag_N << ","
                            << step.rolling_resistance_N << ","
                            << step.required_thrust_N << ","
                            << step.required_total_shaft_power_W << ","
                            << step.deck_point.advance_ratio << ","
                            << step.deck_point.thrust_coefficient << ","
                            << step.deck_point.power_coefficient << ","
                            << step.deck_point.efficiency << ","
                            << step.deck_point.thrust_N << ","
                            << step.deck_point.shaft_power_W << ","
                            << input.propeller.count * step.deck_point.thrust_N << ","
                            << input.propeller.count * step.deck_point.shaft_power_W << ","
                            << step.deck_point.speed_of_sound_ms << ","
                            << step.deck_point.tip_speed_ms << ","
                            << step.deck_point.tip_mach << ","
                            << step.deck_point.tip_mach_limit << ","
                            << (step.deck_point.tip_mach_feasible ? "PASS" : "FAIL")
                            << "\n";
                }
            }

            struct capacity_case
            {
                const char* name;
                const char* curve;
                double altitude_m;
                double speed_ms;
            };
            const std::vector<capacity_case> capacity_cases = {
                {"acceleration", "propeller_acceleration_constraint",
                 input.acceleration.altitude_m, input.acceleration.speed_ms},
                {"cruise", "propeller_cruise_constraint",
                 input.cruise.altitude_m, input.cruise.speed_ms},
                {"climb", "propeller_climb_constraint",
                 input.climb.representative_point.altitude_m,
                 input.climb.representative_point.speed_ms},
                {"turn", "propeller_turn_constraint",
                 input.turn.altitude_m, input.turn.speed_ms},
            };

            std::ofstream capacity(
                output_directory / "propeller_capacity_check.csv");
            capacity << "case,wing_loading_N_m2,altitude_m,speed_ms,rpm,"
                        "pitch_deg,advance_ratio,required_power_to_weight_W_N,"
                        "required_total_shaft_power_W,required_total_thrust_N,"
                        "deck_total_shaft_power_W,deck_total_thrust_N,"
                        "deck_power_margin_W,deck_thrust_margin_N,"
                        "tip_mach,tip_mach_limit,tip_mach_status,data_status\n";

            const auto limiting_takeoff_step = std::max_element(
                takeoff_result.steps.begin(), takeoff_result.steps.end(),
                [&](const propeller_takeoff_step& left,
                    const propeller_takeoff_step& right)
                {
                    const double left_ratio = std::max(
                        left.required_total_shaft_power_W /
                            (input.propeller.count *
                             left.deck_point.shaft_power_W),
                        left.required_thrust_N /
                            (input.propeller.count *
                             left.deck_point.thrust_N));
                    const double right_ratio = std::max(
                        right.required_total_shaft_power_W /
                            (input.propeller.count *
                             right.deck_point.shaft_power_W),
                        right.required_thrust_N /
                            (input.propeller.count *
                             right.deck_point.thrust_N));
                    return left_ratio < right_ratio;
                });
            if (limiting_takeoff_step == takeoff_result.steps.end())
                throw std::runtime_error("Integrated takeoff profile is empty.");
            {
                const auto& step = *limiting_takeoff_step;
                const double deck_power_W =
                    input.propeller.count * step.deck_point.shaft_power_W;
                const double deck_thrust_N =
                    input.propeller.count * step.deck_point.thrust_N;
                capacity << "takeoff,"
                         << feasible_best_point.wing_loading << ","
                         << input.takeoff.altitude_m << "," << step.speed_ms << ","
                         << step.deck_point.rpm << ","
                         << step.deck_point.pitch_deg << ","
                         << step.deck_point.advance_ratio << ","
                         << takeoff_result.required_shaft_power_to_weight_W_N << ","
                         << step.required_total_shaft_power_W << ","
                         << step.required_thrust_N << ","
                         << deck_power_W << "," << deck_thrust_N << ","
                         << deck_power_W - step.required_total_shaft_power_W << ","
                         << deck_thrust_N - step.required_thrust_N << ","
                         << step.deck_point.tip_mach << ","
                         << step.deck_point.tip_mach_limit << ","
                         << (step.deck_point.tip_mach_feasible ? "PASS" : "FAIL") << ","
                         << "limiting_integrated_supplied_deck_point\n";
            }

            for (const auto& item : capacity_cases)
            {
                const auto point =
                    propeller_analysis.select_best_airborne_operating_point(
                        input, item.altitude_m, item.speed_ms);
                const double required_W_N =
                    interpolate_envelope_thrust_to_weight(
                        find_curve(output, item.curve),
                        feasible_best_point.wing_loading);
                const double required_power_W =
                    required_W_N * input.aircraft.takeoff_weight_N;
                const double required_thrust_N =
                    required_power_W * point.thrust_N / point.shaft_power_W;
                const double deck_power_W =
                    input.propeller.count * point.shaft_power_W;
                const double deck_thrust_N =
                    input.propeller.count * point.thrust_N;
                capacity << item.name << ","
                         << feasible_best_point.wing_loading << ","
                         << item.altitude_m << "," << item.speed_ms << ","
                         << point.rpm << "," << point.pitch_deg << ","
                         << point.advance_ratio << "," << required_W_N << ","
                         << required_power_W << "," << required_thrust_N << ","
                         << deck_power_W << "," << deck_thrust_N << ","
                         << deck_power_W - required_power_W << ","
                         << deck_thrust_N - required_thrust_N << ","
                         << point.tip_mach << "," << point.tip_mach_limit << ","
                         << (point.tip_mach_feasible ? "PASS" : "FAIL") << ","
                         << "supplied_propeller_operating_point\n";
            }

            std::ofstream scope(
                output_directory / "propeller_model_scope.csv");
            scope << "item,status,implementation_basis\n"
                     "propeller_aerodynamic_map,implemented,"
                     "supplied_UNICADO_propeller_deck\n"
                     "takeoff_ground_roll,implemented,"
                     "speed_integrated_ground_roll_with_deck_CT_and_CP\n"
                     "airborne_constraints,implemented_with_coverage_flag,"
                     "climb_uses_only_supplied_deck_covered_mission_points;"
                     "see_propeller_climb_mission_coverage.csv\n"
                     "vertical_constraints,implemented,"
                     "landing_stall_and_gust_wing_loading_limits\n"
                     "propeller_tip_mach,implemented,"
                     "helical_tip_speed_checked_against_configured_limit\n"
                     "installed_shaft_power,not_evaluated,"
                     "matching_chart_reports_required_power\n"
                     "range_fuel_burn,not_evaluated,"
                     "not_required_by_available_data_scope\n";

            const auto worst_operating_point = std::max_element(
                points.begin(), points.end(),
                [](const propeller_operating_point& left,
                   const propeller_operating_point& right)
                {
                    return left.tip_mach < right.tip_mach;
                });
            const auto worst_takeoff_step = std::max_element(
                takeoff_result.steps.begin(), takeoff_result.steps.end(),
                [](const propeller_takeoff_step& left,
                   const propeller_takeoff_step& right)
                {
                    return left.deck_point.tip_mach < right.deck_point.tip_mach;
                });
            const double maximum_tip_mach = std::max(
                worst_operating_point->tip_mach,
                worst_takeoff_step->deck_point.tip_mach);
            std::cout << "\n=== propeller_tip_mach_check ===\n"
                      << "maximum_tip_mach = " << maximum_tip_mach << "\n"
                      << "tip_mach_limit = " << input.propeller.tip_mach_limit << "\n"
                      << "status = "
                      << (maximum_tip_mach <= input.propeller.tip_mach_limit
                              ? "PASS" : "FAIL")
                      << "\n";
        }

        // ============================================================
        // 7. PRINT RESULTS
        // ============================================================
        std::cout << std::fixed << std::setprecision(4);

        for (const auto& curve : output.curves)
        {
            std::cout << "\n=== " << curve.name << " ===\n";

            for (const auto& point : curve.points)
            {
                std::cout
                    << "x = " << std::setw(10) << point.x
                    << "  y = " << std::setw(10) << point.y
                    << '\n';
            }
        }

        std::cout << "\n=== vertical_constraints ===\n";

        for (const auto& vc : output.vertical_constraints)
        {
            std::cout
                << vc.name
                << " -> W/S "
                << (vc.is_upper_limit ? "max" : "min")
                << " = "
                << vc.x_limit
                << '\n';
        }

        std::cout << "\n=== constraint_envelope ===\n";

        for (const auto& point : envelope.points)
        {
            std::cout
                << "x = " << std::setw(10) << point.x
                << "  y = " << std::setw(10) << point.y
                << '\n';
        }

        std::cout << "\n=== sampled_best_design_point ===\n";
        std::cout
            << "wing_loading = " << sampled_best_point.wing_loading
            << (is_propeller ? "  shaft_power_to_weight = " : "  thrust_to_weight = ")
            << sampled_best_point.thrust_to_weight
            << '\n';

        std::cout << "\n=== interpolated_best_design_point ===\n";
        std::cout
            << "wing_loading = " << best_point.wing_loading
            << (is_propeller ? "  shaft_power_to_weight = " : "  thrust_to_weight = ")
            << best_point.thrust_to_weight
            << '\n';

        std::cout << "\n=== minimum_feasible_envelope_point_reference ===\n";
        std::cout
            << "wing_loading = " << feasible_best_point.wing_loading
            << (is_propeller ? "  shaft_power_to_weight = " : "  thrust_to_weight = ")
            << feasible_best_point.thrust_to_weight
            << '\n';

        std::cout << "\n=== aircraft_point_from_aerodynamics_area ===\n";
        std::cout
            << "wing_area = " << input.aircraft.wing_area_m2 << " m^2"
            << "  wing_loading = " << aircraft_wing_loading
            << (is_propeller
                    ? "  required_shaft_power_to_weight = "
                    : "  required_thrust_to_weight = ")
            << aircraft_required_thrust_to_weight
            << "  vertical_constraints_feasible = "
            << (aircraft_wing_loading_feasible ? "yes" : "no")
            << (is_propeller ? "  required_total_shaft_power = " : "") ;
        if (is_propeller)
            std::cout << aircraft_required_thrust_to_weight *
                             input.aircraft.takeoff_weight_N << " W";
        std::cout << '\n';

        std::cout << "\n=== active_constraints ===\n";

        for (const auto& point : active_constraints)
        {
            std::cout
                << "x = " << std::setw(10) << point.x
                << "  y = " << std::setw(10) << point.y
                << "  active = " << point.active_constraint_name
                << '\n';
        }

        std::cout << "\n=== range_fuel_fraction_constraints ===\n";

        for (const auto& range_result : output.range_constraints)
        {
            std::cout
                << range_result.name
                << "  available fuel fraction = "
                << range_result.available_fuel_fraction
                << '\n';

            for (const auto& point : range_result.points)
            {
                std::cout
                    << "W/S = " << std::setw(10) << point.wing_loading
                    << "  L/D = " << std::setw(10) << point.lift_to_drag
                    << "  fuel_req = " << std::setw(10) << point.required_fuel_fraction
                    << "  feasible = " << (point.feasible ? "yes" : "no")
                    << '\n';
            }
        }

        if (!is_propeller)
        {
        std::cout << "\n=== carpet_plot_study ===\n";

        for (const auto& point : carpet_points)
        {
            std::cout
                << "CD0 = " << point.cd_0
                << "  best W/S = " << point.best_wing_loading
                << "  best T/W = " << point.best_thrust_to_weight
                << "  range feasible = " << (point.range_feasible ? "yes" : "no")
                << '\n';
        }

        std::cout << "\n=== true_carpet_constraints ===\n";
        std::cout
            << "True carpet constraint points written: "
            << true_carpet_points.size()
            << '\n';
        std::cout << "CSV: "
                  << (output_directory / "true_carpet_constraints.csv").string()
                  << "\n";

        std::cout << "\n=== jet_cd0_k_carpet ===\n";
        std::cout << "Aerodynamic carpet points written: "
                  << jet_aero_carpet_points.size() << '\n';
        std::cout << "CSV: "
                  << (output_directory / "jet_cd0_k_carpet.csv").string()
                  << "\n";
        }

        if (is_propeller)
        {
            const double required_total_shaft_power_W =
                feasible_best_point.thrust_to_weight *
                input.aircraft.takeoff_weight_N;
            const double required_wing_area_m2 =
                input.aircraft.takeoff_weight_N /
                feasible_best_point.wing_loading;
            std::cout << "\n=== propeller_design_requirements ===\n";
            std::cout << "required_best_shaft_power_to_weight = "
                      << feasible_best_point.thrust_to_weight << " W/N\n";
            std::cout << "required_total_shaft_power = "
                      << required_total_shaft_power_W << " W\n";
            std::cout << "required_total_shaft_power = "
                      << required_total_shaft_power_W / 1.0e6 << " MW\n";
            std::cout << "required_wing_area = "
                      << required_wing_area_m2 << " m^2\n";
            std::cout << "CSV: "
                      << (output_directory / "propeller_operating_points.csv").string()
                      << "\n";
            std::cout << "Performance map CSV: "
                      << (output_directory / "propeller_performance_map.csv").string()
                      << "\n";
        }

        {
            std::ofstream latest_case(output_root / "latest_case.txt");
            latest_case << active_case_id << '\n';
        }

        std::cout << "\nCSV files written into "
                  << output_directory.string() << "\n";
    }
    catch (const std::string& error)
    {
        std::cerr << "string error: " << error << '\n';
        return 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "std::exception error: " << exception.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "unknown error occurred\n";
        return 1;
    }

    return 0;
}
