#include "constraint_analysis/constraint_analysis.h"
#include "constraint_analysis/ca_parser.h"
#include "atmosphere/atmosphere.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace constraint_analysis;

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

        const std::filesystem::path engine_directory =
            xml_string(config, "engine_directory_path");

        if (engine_directory.empty())
        {
            throw std::runtime_error(
                "Engine directory is empty. Set engine/engine_directory_path "
                "in the selected XML constraint case.");
        }

        std::unique_ptr<Engine> unicado_engine =
            std::make_unique<Engine>(engine_directory);

        constraint_input input =
            build_constraint_input_from_config(config, unicado_engine.get());

        std::cout << "Using UNICADO engine library: "
                  << engine_directory.string() << '\n';
        std::cout << "Using UNICADO atmosphere library.\n";
        std::cout << "Thrust lapse and TSFC are read from the UNICADO Engine deck.\n";

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

        const auto active_constraints =
            active_constraint_analyzer::analyze(output);

        // ============================================================
        // 3. WRITE MAIN CSV OUTPUTS
        // ============================================================
        constraint_output_writer::write_all_curves_to_csv(output, "output");

        constraint_output envelope_output;
        envelope_output.curves.push_back(envelope);
        constraint_output_writer::write_all_curves_to_csv(envelope_output, "output");

        // The plotting script reads this file so the marker is placed at the
        // quadratically interpolated envelope minimum instead of the nearest grid point.
        {
            std::ofstream file("output/design_point.csv");
            const double design_wing_area_m2 =
                input.aircraft.takeoff_weight_N / feasible_best_point.wing_loading;

            file << "wing_loading,thrust_to_weight,wing_area_m2,method\n";
            file << feasible_best_point.wing_loading << ","
                 << feasible_best_point.thrust_to_weight << ","
                 << design_wing_area_m2
                 << ",local_quadratic_interpolation\n";
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
        carpet_plot_study study{atm};

        const std::vector<double> cd0_values = {
            0.020,
            0.025,
            0.030,
            0.035
        };

        const auto carpet_points =
            study.run(input, cd0_values);

        carpet_plot_study::write_to_csv(
            carpet_points,
            "output/carpet_plot_study.csv");

        // ============================================================
        // 5. FULL CARPET PLOT DATA
        // ============================================================
        carpet_plot_full full_carpet{atm};

        const auto full_carpet_points =
            full_carpet.run(input, cd0_values);

        carpet_plot_full::write_to_csv(
            full_carpet_points,
            "output/carpet_plot_full.csv");

        // ============================================================
        // 6. TRUE CARPET CONSTRAINT FAMILY DATA
        // ============================================================
        true_carpet_constraints true_carpet{atm};

        const auto true_carpet_points =
            true_carpet.run(input, cd0_values);

        true_carpet_constraints::write_to_csv(
            true_carpet_points,
            "output/true_carpet_constraints.csv");

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
            << "  thrust_to_weight = " << sampled_best_point.thrust_to_weight
            << '\n';

        std::cout << "\n=== interpolated_best_design_point ===\n";
        std::cout
            << "wing_loading = " << best_point.wing_loading
            << "  thrust_to_weight = " << best_point.thrust_to_weight
            << '\n';

        std::cout << "\n=== feasible_best_design_point ===\n";
        const double design_wing_area_m2 =
            input.aircraft.takeoff_weight_N / feasible_best_point.wing_loading;
        std::cout
            << "wing_loading = " << feasible_best_point.wing_loading
            << "  thrust_to_weight = " << feasible_best_point.thrust_to_weight
            << "  wing_area = " << design_wing_area_m2 << " m^2"
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