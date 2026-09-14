#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "constraint_analysis/ca_functions.h"

namespace constraint_analysis
{
    struct aerodynamic_aircraft_values
    {
        double wing_area_m2 = 0.0;
        double aspect_ratio = 0.0;
        double mean_aerodynamic_chord_m = 0.0;
        double cd0 = 0.0;
        double k = 0.0;
        double cl_max_takeoff = 0.0;
        double cl_max_landing = 0.0;
        double minimum_mach = 0.0;
        double maximum_mach = 0.0;
        std::vector<aerodynamic_polar_sample> polar_samples;
        std::vector<std::string> configuration_ids;
    };

    aerodynamic_aircraft_values read_aerodynamic_aircraft_values(
        const std::filesystem::path& polar_xml_path,
        const std::string& reference_wing_id);

    drag_polar read_drag_polar_at_condition(
        const std::filesystem::path& polar_xml_path,
        const std::string& reference_wing_id,
        double mach,
        double altitude_m);

}
