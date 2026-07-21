#include "io/aerodynamics_xml.h"

#include <aerodynamics/aerodynamics_v2.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace constraint_analysis
{
    namespace
    {
        std::string read_text_file(const std::filesystem::path& path)
        {
            std::ifstream file(path);
            if (!file)
            {
                throw std::runtime_error("Could not open aerodynamic polar XML file: " + path.string());
            }

            std::ostringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        std::vector<double> parse_semicolon_values(const std::string& text)
        {
            std::vector<double> values;
            std::stringstream stream(text);
            std::string token;

            while (std::getline(stream, token, ';'))
            {
                if (!token.empty())
                {
                    values.push_back(std::stod(token));
                }
            }

            return values;
        }

        std::string extract_tag(const std::string& block, const std::string& tag)
        {
            const std::regex expression(
                "<" + tag + R"(\s*[^>]*>([\s\S]*?)</)" + tag + ">",
                std::regex::icase);
            std::smatch match;

            if (!std::regex_search(block, match, expression))
            {
                throw std::runtime_error("Missing <" + tag + "> in trimmed aerodynamic polar XML.");
            }

            return match[1].str();
        }

        std::string extract_configuration(const std::string& xml, const std::string& id)
        {
            const std::regex expression(
                R"(<configuration\s+ID=")" + id + R"("[^>]*>([\s\S]*?)</configuration>)",
                std::regex::icase);
            std::smatch match;

            if (!std::regex_search(xml, match, expression))
            {
                throw std::runtime_error("Configuration '" + id + "' not found in trimmed aerodynamic polar XML.");
            }

            return match[1].str();
        }

        std::vector<std::string> extract_polar_blocks(const std::string& configuration)
        {
            std::vector<std::string> blocks;
            const std::regex expression(R"(<polar\s+[^>]*>([\s\S]*?)</polar>)", std::regex::icase);

            for (std::sregex_iterator it(configuration.begin(), configuration.end(), expression), end;
                 it != end; ++it)
            {
                blocks.push_back((*it)[1].str());
            }

            if (blocks.empty())
            {
                throw std::runtime_error("No <polar> blocks found in trimmed aerodynamic configuration.");
            }

            return blocks;
        }

        double maximum_cl_in_configuration(const std::string& xml, const std::string& id)
        {
            const auto blocks = extract_polar_blocks(extract_configuration(xml, id));
            double maximum = -std::numeric_limits<double>::infinity();

            for (const auto& block : blocks)
            {
                const auto cl = parse_semicolon_values(extract_tag(block, "Cl"));
                if (!cl.empty())
                {
                    maximum = std::max(maximum, *std::max_element(cl.begin(), cl.end()));
                }
            }

            if (!std::isfinite(maximum))
            {
                throw std::runtime_error("Could not determine CLmax for configuration '" + id + "'.");
            }

            return maximum;
        }

        aerodynamic_aircraft_values read_trimmed_aircraft_polar(
            const std::filesystem::path& polar_xml_path)
        {
            const std::string xml = read_text_file(polar_xml_path);
            const auto clean_blocks = extract_polar_blocks(extract_configuration(xml, "clean"));

            // Use the first clean polar, matching the previous component-polar behavior.
            const auto cl = parse_semicolon_values(extract_tag(clean_blocks.front(), "Cl"));
            const auto cd = parse_semicolon_values(extract_tag(clean_blocks.front(), "Cd"));

            if (cl.size() != cd.size() || cl.size() < 3)
            {
                throw std::runtime_error("Trimmed clean polar has inconsistent Cl/Cd arrays.");
            }

            // Least-squares fit of Cd = CD0 + k*Cl^2.
            // Restrict the fit to the normal low/moderate-lift region so post-stall points
            // do not distort the preliminary parabolic drag polar.
            double n = 0.0;
            double sum_x = 0.0;
            double sum_y = 0.0;
            double sum_xx = 0.0;
            double sum_xy = 0.0;

            for (std::size_t i = 0; i < cl.size(); ++i)
            {
                if (cl[i] < -0.2 || cl[i] > 0.9)
                {
                    continue;
                }

                const double x = cl[i] * cl[i];
                const double y = cd[i];
                n += 1.0;
                sum_x += x;
                sum_y += y;
                sum_xx += x * x;
                sum_xy += x * y;
            }

            const double denominator = n * sum_xx - sum_x * sum_x;
            if (n < 3.0 || std::abs(denominator) < 1.0e-12)
            {
                throw std::runtime_error("Could not fit CD0 and k from trimmed clean polar.");
            }

            aerodynamic_aircraft_values values;
            values.k = (n * sum_xy - sum_x * sum_y) / denominator;
            values.cd0 = (sum_y - values.k * sum_x) / n;
            values.cl_max_takeoff = maximum_cl_in_configuration(xml, "takeoff");
            values.cl_max_landing = maximum_cl_in_configuration(xml, "landing");

            // A trimmed aircraft polar contains dimensionless aerodynamic data only.
            // Wing area is intentionally not imported; it is calculated after W/S is selected.
            values.wing_area_m2 = 0.0;
            values.aspect_ratio = 0.0;

            if (values.cd0 <= 0.0 || values.k <= 0.0)
            {
                throw std::runtime_error("Invalid CD0 or k fitted from trimmed aerodynamic polar.");
            }

            return values;
        }
    }

    aerodynamic_aircraft_values read_aerodynamic_aircraft_values(
        const std::filesystem::path& polar_xml_path,
        const std::string& reference_wing_id,
        const std::string& polar_format)
    {
        if (!std::filesystem::exists(polar_xml_path))
        {
            throw std::runtime_error(
                "Could not find aerodynamic polar XML file: " + polar_xml_path.string());
        }

        if (polar_format == "trimmed_aircraft_polar")
        {
            return read_trimmed_aircraft_polar(polar_xml_path);
        }

        if (polar_format != "component_polar")
        {
            throw std::runtime_error(
                "Unknown aerodynamic polar format: " + polar_format +
                ". Expected component_polar or trimmed_aircraft_polar.");
        }

        aerodynamics::Aircraft aircraft(polar_xml_path.string());
        const auto component_it = aircraft.components.find(reference_wing_id);

        if (component_it == aircraft.components.end())
        {
            throw std::runtime_error(
                "Reference wing ID not found in aerodynamic polar XML: " + reference_wing_id);
        }

        auto& component = component_it->second;
        if (component.configurations.empty() || component.configurations.front().polars.empty())
        {
            throw std::runtime_error("No aerodynamic polar data found for component: " + reference_wing_id);
        }

        const auto& first_polar = component.configurations.front().polars.front();
        std::vector<double> conditions = {
            first_polar.conditions.freestream_M,
            first_polar.conditions.altitude
        };

        aerodynamic_aircraft_values values;
        values.wing_area_m2 = component.area;
        values.cd0 = component.get_property("CD_0", conditions);
        values.k = component.get_property("K2", conditions);
        values.aspect_ratio = 0.0;

        if (values.k <= 0.0)
        {
            throw std::runtime_error("Invalid K2 value read from aerodynamics polar XML.");
        }

        double cl_max = -std::numeric_limits<double>::infinity();
        for (const auto& configuration : component.configurations)
        {
            for (const auto& polar : configuration.polars)
            {
                if (!polar.CL.empty())
                {
                    cl_max = std::max(cl_max, *std::max_element(polar.CL.begin(), polar.CL.end()));
                }
            }
        }

        if (!std::isfinite(cl_max))
        {
            throw std::runtime_error("Could not determine CLmax from aerodynamics polar XML.");
        }

        values.cl_max_takeoff = cl_max;
        values.cl_max_landing = cl_max;
        return values;
    }
}
