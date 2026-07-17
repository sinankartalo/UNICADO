#include "io/aircraft_xml.h"

#include <aixml/node.h>
#include <aircraftGeometry2/geometry/import_geom.h>
#include <aircraftGeometry2/processing/measure.h>

#include <stdexcept>


namespace constraint_analysis
{
    geometry_aircraft_values read_geometry_aircraft_values(
        const std::filesystem::path& geometry_xml_path,
        const std::filesystem::path& geometry_data_dir)
    {
        if (!std::filesystem::exists(geometry_xml_path))
        {
            throw std::runtime_error(
                "Could not find geometry XML file: " +
                geometry_xml_path.string());
        }

        const auto aircraft_xml = aixml::openDocument(geometry_xml_path);

        const auto wings =
            geom2::import_aerodynamic_surfaces(
                aircraft_xml,
                geometry_data_dir,
                "wing");

        if (wings.empty())
        {
            throw std::runtime_error(
                "No wing aerodynamic surface found in geometry XML: " +
                geometry_xml_path.string());
        }

        geometry_aircraft_values values;
        values.wing_area_m2 =
            geom2::measure::reference_area(wings.front());

        values.aspect_ratio =
            geom2::measure::aspect_ratio(wings.front());

        values.wing_span_m =
            geom2::measure::span(wings.front());

        values.mean_aerodynamic_chord_m =
            geom2::measure::mean_aerodynamic_chord(wings.front());

        return values;
    }
}