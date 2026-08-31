#include "io/aerodynamics_xml.h"

#include <aerodynamics/aerodynamics_v3.h>
#include <interpolation/data_types.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace constraint_analysis
{
    namespace
    {
        aerodynamics::Component& cached_component(
            const std::filesystem::path& polar_xml_path,
            const std::string& reference_wing_id)
        {
            static std::unordered_map<
                std::string, std::unique_ptr<aerodynamics::Aircraft>> cache;
            const std::string key =
                polar_xml_path.lexically_normal().string() + "|" +
                reference_wing_id;
            auto it = cache.find(key);
            if (it == cache.end())
            {
                aerodynamics::Aircraft::trim_settings settings;
                settings.method = "linearized";
                auto aircraft = std::make_unique<aerodynamics::Aircraft>(
                    polar_xml_path.string(), settings);
                it = cache.emplace(key, std::move(aircraft)).first;
            }
            const auto component_it =
                it->second->components.find(reference_wing_id);
            if (component_it == it->second->components.end())
                throw std::runtime_error(
                    "Reference wing ID not found in aerodynamic polar XML: " +
                    reference_wing_id);
            return component_it->second;
        }
    }

    drag_polar read_drag_polar_at_condition(
        const std::filesystem::path& polar_xml_path,
        const std::string& reference_wing_id,
        double mach,
        double altitude_m)
    {
        auto& component = cached_component(polar_xml_path, reference_wing_id);
        types::PropertyType conditions;
        conditions["Mach"] = mach;
        conditions["h"] = altitude_m;
        drag_polar result;
        result.cd_0 = component.get_property("CD_0", conditions);
        result.k = component.get_property("K2", conditions);
        if (!std::isfinite(result.cd_0) || result.cd_0 < 0.0 ||
            !std::isfinite(result.k) || result.k <= 0.0)
            throw std::runtime_error(
                "Aerodynamic interpolation returned an invalid drag polar.");
        return result;
    }

    aerodynamic_aircraft_values read_aerodynamic_aircraft_values(
        const std::filesystem::path& polar_xml_path,
        const std::string& reference_wing_id)
    {
        if (!std::filesystem::exists(polar_xml_path))
        {
            throw std::runtime_error(
                "Could not find aerodynamic polar XML file: " + polar_xml_path.string());
        }

        auto& component = cached_component(polar_xml_path, reference_wing_id);
        if (component.configurations.empty() || component.configurations.front().polars.empty())
        {
            throw std::runtime_error("No aerodynamic polar data found for component: " + reference_wing_id);
        }

        const auto& first_polar = component.configurations.front().polars.front();
        types::PropertyType conditions;
        conditions["Mach"] = first_polar.conditions.freestream_M;
        conditions["h"] = first_polar.conditions.altitude;

        aerodynamic_aircraft_values values;
        values.wing_area_m2 = component.area;
        values.mean_aerodynamic_chord_m = component.chord;
        values.cd0 = component.get_property("CD_0", conditions);
        values.k = component.get_property("K2", conditions);
        values.aspect_ratio = 0.0;

        if (values.k <= 0.0)
        {
            throw std::runtime_error("Invalid K2 value read from aerodynamics polar XML.");
        }

        double cl_max = -std::numeric_limits<double>::infinity();
        values.minimum_mach = std::numeric_limits<double>::infinity();
        values.maximum_mach = -std::numeric_limits<double>::infinity();
        for (const auto& configuration : component.configurations)
        {
            values.configuration_ids.push_back(configuration.configuration_ID);
            for (const auto& polar : configuration.polars)
            {
                values.minimum_mach = std::min(
                    values.minimum_mach,
                    polar.conditions.freestream_M);
                values.maximum_mach = std::max(
                    values.maximum_mach,
                    polar.conditions.freestream_M);
                if (polar.CL.size() > 0)
                {
                    const double local_cl_max = polar.CL.maxCoeff();
                    cl_max = std::max(cl_max, local_cl_max);
                    types::PropertyType local_conditions;
                    local_conditions["Mach"] =
                        polar.conditions.freestream_M;
                    local_conditions["h"] = polar.conditions.altitude;
                    values.polar_samples.push_back({
                        polar.conditions.freestream_M,
                        polar.conditions.altitude,
                        component.get_property("CD_0", local_conditions),
                        component.get_property("K2", local_conditions),
                        local_cl_max,
                        configuration.configuration_ID});
                }
            }
        }

        if (!std::isfinite(cl_max))
        {
            throw std::runtime_error("Could not determine CLmax from aerodynamics polar XML.");
        }
        if (!std::isfinite(values.minimum_mach) ||
            !std::isfinite(values.maximum_mach) ||
            values.maximum_mach < values.minimum_mach)
        {
            throw std::runtime_error(
                "Could not determine Mach coverage from aerodynamics polar XML.");
        }

        values.cl_max_takeoff = cl_max;
        values.cl_max_landing = cl_max;
        return values;
    }
}
