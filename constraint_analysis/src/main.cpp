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
}

int main(int argc, char* argv[])
{
    try
    {
        std::filesystem::create_directories("output");

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

        std::cout << "Using XML configuration: " << config_path << '\n';
        std::cout << "Using constraint case ID: "
                  << xml_string(config, "active_constraint_case_id") << '\n';

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
            std::ofstream metadata("output/analysis_metadata.csv");
            metadata << "case_id,propulsion_type,y_axis,y_unit\n";
            metadata << xml_string(config, "active_constraint_case_id") << ","
                     << (is_propeller ? "propeller" : "jet") << ","
                     << (is_propeller ? "shaft_power_to_weight" : "thrust_to_weight")
                     << "," << (is_propeller ? "W/N" : "-") << "\n";
        }
        std::cout << "Using UNICADO atmosphere library.\n";
        if (is_propeller)
        {
            std::cout << "Matching-chart y-axis: required shaft P/W [W/N].\n";
            std::cout << "Propeller deck status: test data, not a selected production propeller.\n";
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
        constraint_output_writer::write_all_curves_to_csv(output, "output");

        constraint_output envelope_output;
        envelope_output.curves.push_back(envelope);
        constraint_output_writer::write_all_curves_to_csv(envelope_output, "output");

        // Keep both engineering interpretations in one traceable plotting
        // interface: the existing aircraft from aerodynamic Sref and the
        // minimum feasible point proposed by the constraint analysis.
        {
            std::ofstream file("output/design_point.csv");
            file << "wing_loading,"
                    << (is_propeller ? "shaft_power_to_weight" : "thrust_to_weight")
                    << ",wing_area_m2,"
                    "vertical_constraints_feasible,method\n";
            file << aircraft_wing_loading << ","
                 << aircraft_required_thrust_to_weight << ","
                 << input.aircraft.wing_area_m2 << ","
                 << aircraft_wing_loading_feasible << ","
                 << "aerodynamics_reference_area\n";
            file << feasible_best_point.wing_loading << ","
                 << feasible_best_point.thrust_to_weight << ","
                 << input.aircraft.takeoff_weight_N /
                        feasible_best_point.wing_loading << ","
                 << true << ","
                 << "best_design_point\n";
        }

        for (const auto& vc : output.vertical_constraints)
        {
            std::ofstream file("output/" + vc.name + ".csv");
            file << "W/S_limit,type\n";
            file << vc.x_limit << ","
                 << (vc.is_upper_limit ? "max" : "min") << "\n";
        }

        for (const auto& range_result : output.range_constraints)
        {
            std::ofstream file("output/" + range_result.name + ".csv");
            file << "wing_loading,lift_to_drag,required_fuel_fraction,feasible\n";

            for (const auto& point : range_result.points)
            {
                file << point.wing_loading << ","
                     << point.lift_to_drag << ","
                     << point.required_fuel_fraction << ","
                     << point.feasible << "\n";
            }
        }

        // ============================================================
        // 4. CARPET PLOT PARAMETER STUDY
        // ============================================================
        const std::vector<double> cd0_values = {
            0.020,
            0.025,
            0.030,
            0.035
        };

        std::vector<carpet_study_point> carpet_points;
        std::vector<carpet_full_point> full_carpet_points;
        std::vector<true_carpet_constraint_point> true_carpet_points;

        if (!is_propeller)
        {
            carpet_plot_study study{atm};
            carpet_points = study.run(input, cd0_values);
            carpet_plot_study::write_to_csv(
                carpet_points, "output/carpet_plot_study.csv");

            carpet_plot_full full_carpet{atm};
            full_carpet_points = full_carpet.run(input, cd0_values);
            carpet_plot_full::write_to_csv(
                full_carpet_points, "output/carpet_plot_full.csv");

            true_carpet_constraints true_carpet{atm};
            true_carpet_points = true_carpet.run(input, cd0_values);
            true_carpet_constraints::write_to_csv(
                true_carpet_points, "output/true_carpet_constraints.csv");
        }

        // ============================================================
        // 5. PROPELLER OPERATING-POINT EVIDENCE
        // ============================================================
        if (is_propeller)
        {
            propeller_constraint_analysis propeller_analysis{atm};
            const std::vector<propeller_operating_point> points = {
                propeller_analysis.evaluate(
                    input, input.cruise.altitude_m, input.cruise.speed_ms,
                    input.propeller.continuous),
                propeller_analysis.evaluate(
                    input, input.climb.altitude_m, input.climb.speed_ms,
                    input.propeller.continuous),
                propeller_analysis.evaluate(
                    input, input.turn.altitude_m, input.turn.speed_ms,
                    input.propeller.continuous),
                propeller_analysis.evaluate(
                    input, input.acceleration.altitude_m,
                    input.acceleration.speed_ms, input.propeller.continuous)
            };

            std::ofstream file("output/propeller_operating_points.csv");
            file << "altitude_m,speed_ms,density_kg_m3,rpm,pitch_deg,"
                    "advance_ratio,CT,CP,eta,thrust_N,shaft_power_W\n";
            for (const auto& point : points)
            {
                file << point.altitude_m << "," << point.speed_ms << ","
                     << point.density_kg_m3 << "," << point.rpm << ","
                     << point.pitch_deg << "," << point.advance_ratio << ","
                     << point.thrust_coefficient << ","
                     << point.power_coefficient << "," << point.efficiency << ","
                     << point.thrust_N << "," << point.shaft_power_W << "\n";
            }
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
            << '\n';

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

        std::cout << "\n=== full_carpet_plot ===\n";
        std::cout
            << "Full carpet plot points written: "
            << full_carpet_points.size()
            << '\n';
        std::cout << "CSV: output/carpet_plot_full.csv\n";

        std::cout << "\n=== true_carpet_constraints ===\n";
        std::cout
            << "True carpet constraint points written: "
            << true_carpet_points.size()
            << '\n';
        std::cout << "CSV: output/true_carpet_constraints.csv\n";
        }

        if (is_propeller)
        {
            const double available_power_to_weight =
                input.propeller.maximum_total_shaft_power_W /
                input.aircraft.takeoff_weight_N;
            std::cout << "\n=== propeller_power_availability ===\n";
            std::cout << "maximum_total_shaft_power = "
                      << input.propeller.maximum_total_shaft_power_W << " W\n";
            std::cout << "available_shaft_power_to_weight = "
                      << available_power_to_weight << " W/N\n";
            std::cout << "required_best_shaft_power_to_weight = "
                      << feasible_best_point.thrust_to_weight << " W/N\n";
            std::cout << "installed_power_feasible = "
                      << (available_power_to_weight >=
                                  feasible_best_point.thrust_to_weight
                              ? "yes" : "no")
                      << '\n';
            std::cout << "CSV: output/propeller_operating_points.csv\n";
        }

        std::cout << "\nCSV files written into ./output folder\n";
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
