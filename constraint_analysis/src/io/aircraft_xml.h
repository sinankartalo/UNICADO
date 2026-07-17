#ifndef AIRCRAFTXML_H_
#define AIRCRAFTXML_H_

#pragma once

#include <filesystem>

namespace constraint_analysis
{
    struct geometry_aircraft_values
    {
        double wing_area_m2 = 0.0;
        double aspect_ratio = 0.0;
        double wing_span_m = 0.0;
        double mean_aerodynamic_chord_m = 0.0;
    };

    geometry_aircraft_values read_geometry_aircraft_values(
        const std::filesystem::path& geometry_xml_path,
        const std::filesystem::path& geometry_data_dir);
}

#endif 