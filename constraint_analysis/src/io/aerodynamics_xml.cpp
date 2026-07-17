#include "io/aerodynamics_xml.h"

#include <aerodynamics/aerodynamics_v2.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace constraint_analysis
{
    aerodynamic_aircraft_values read_aerodynamic_aircraft_values(
        const std::filesystem::path& polar_xml_path,
        const std::string& reference_wing_id)
    {
        if (!std::filesystem::exists(polar_xml_path))
        {
            throw std::runtime_error(
                "Could not find aerodynamic polar XML file: " +
                polar_xml_path.string());
        }

        aerodynamics::Aircraft aircraft(polar_xml_path.string());

        const auto component_it = aircraft.components.find(reference_wing_id);

        if (component_it == aircraft.components.end())
        {
            throw std::runtime_error(
                "Reference wing ID not found in aerodynamic polar XML: " +
                reference_wing_id);
        }

        auto& component = component_it->second;

        if (component.configurations.empty() ||
            component.configurations.front().polars.empty())
        {
            throw std::runtime_error(
                "No aerodynamic polar data found for component: " +
                reference_wing_id);
        }

        const auto& first_polar =
            component.configurations.front().polars.front();

        std::vector<double> conditions = {
            first_polar.conditions.freestream_M,
            first_polar.conditions.altitude
        };

        aerodynamic_aircraft_values values;

        values.wing_area_m2 = component.area;
        values.cd0 = component.get_property("CD_0", conditions);
        values.k = component.get_property("K2", conditions);

        if (component.chord <= 0.0)
        {
            throw std::runtime_error(
                "Invalid chord value read from aerodynamics polar XML.");
        }

        values.aspect_ratio = 0.0;
        if (values.k <= 0.0)
        {
            throw std::runtime_error(
                "Invalid K2 value read from aerodynamics polar XML.");
        }

        values.cl_max = -std::numeric_limits<double>::infinity();

        for (const auto& configuration : component.configurations)
        {
            for (const auto& polar : configuration.polars)
            {
                if (!polar.CL.empty())
                {
                    const double local_cl_max =
                        *std::max_element(polar.CL.begin(), polar.CL.end());

                    values.cl_max = std::max(values.cl_max, local_cl_max);
                }
            }
        }

        if (!std::isfinite(values.cl_max))
        {
            throw std::runtime_error(
                "Could not determine CLmax from aerodynamics polar XML.");
        }

        return values;
    }
}