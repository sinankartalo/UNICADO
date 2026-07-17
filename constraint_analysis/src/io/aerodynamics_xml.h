#pragma once

#include <filesystem>
#include <string>

namespace constraint_analysis
{
    struct aerodynamic_aircraft_values
    {
        double wing_area_m2 = 0.0;
        double aspect_ratio = 0.0;
        double cd0 = 0.0;
        double k = 0.0;
        double cl_max = 0.0;
    };

    aerodynamic_aircraft_values read_aerodynamic_aircraft_values(
        const std::filesystem::path& polar_xml_path,
        const std::string& reference_wing_id);
}