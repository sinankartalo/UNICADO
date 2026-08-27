#include "constraint_analysis/ca_parser.h"
#include "io/aerodynamics_xml.h"

#include <aixml/node.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <sstream>
#include <stdexcept>

namespace constraint_analysis
{
    static std::string trim_copy(const std::string& text)
    {
        const std::string whitespace = " \t\r\n";
        const std::size_t begin = text.find_first_not_of(whitespace);

        if (begin == std::string::npos)
        {
            return "";
        }

        const std::size_t end = text.find_last_not_of(whitespace);
        return text.substr(begin, end - begin + 1);
    }

    std::string xml_string(
        const text_config& config,
        const std::string& key)
    {
        const auto it = config.values.find(key);

        if (it == config.values.end())
        {
            throw std::runtime_error("Missing config value: " + key);
        }

        return it->second;
    }


    double xml_double(
        const text_config& config,
        const std::string& key)
    {
        return std::stod(xml_string(config, key));
    }

    static bool has_xml_extension(const std::filesystem::path& path)
    {
        std::string extension = path.extension().string();
        for (char& c : extension)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return extension == ".xml";
    }

    static std::string xml_node_text(node& xml_node)
    {
        node* value_node = xml_node.find("value", 1);
        if (value_node != nullptr)
        {
            return trim_copy(static_cast<std::string>(*value_node));
        }

        return trim_copy(static_cast<std::string>(xml_node));
    }

    static std::string xml_required_string(
        node& parent,
        const std::string& path)
    {
        node* xml_node = parent.find(path);
        if (xml_node == nullptr)
        {
            throw std::runtime_error("Missing XML value: " + path);
        }

        const std::string value = xml_node_text(*xml_node);
        if (value.empty())
        {
            throw std::runtime_error("Empty XML value: " + path);
        }

        return value;
    }

    static void xml_map_value(
        text_config& config,
        node& parent,
        const std::string& config_key,
        const std::string& xml_path)
    {
        config.values[config_key] = xml_required_string(parent, xml_path);
    }

    static std::string get_active_case_id(
        node& document,
        node& constraint_cases,
        const std::string& case_override_id)
    {
        if (!case_override_id.empty())
        {
            return case_override_id;
        }

        if (constraint_cases.hasAttrib("active_case_ID"))
        {
            return constraint_cases.getStringAttrib("active_case_ID");
        }

        node* active_case_node = document.find("active_constraint_case_ID");
        if (active_case_node != nullptr)
        {
            return xml_node_text(*active_case_node);
        }

        throw std::runtime_error(
            "XML config must define either constraint_cases active_case_ID "
            "or control_settings/active_constraint_case_ID/value.");
    }

    text_config read_xml_config(
        const std::filesystem::path& path,
        const std::string& case_override_id)
    {
        if (!has_xml_extension(path))
        {
            throw std::runtime_error("read_xml_config expects an .xml file: " + path.string());
        }

        std::shared_ptr<node> document;
        try
        {
            document = aixml::openDocument(path);
        }
        catch (const std::string& error)
        {
            throw std::runtime_error(error);
        }

        node* constraint_cases = document->find("constraint_cases");
        if (constraint_cases == nullptr)
        {
            throw std::runtime_error("XML config is missing program_settings/constraint_cases.");
        }

        const std::string active_case_id =
            get_active_case_id(*document, *constraint_cases, case_override_id);

        node* selected_case = constraint_cases->find("constraint_case@ID=" + active_case_id, 1);
        if (selected_case == nullptr)
        {
            throw std::runtime_error(
                "Could not find constraint_case with ID: " + active_case_id);
        }

        text_config config;
        config.values["active_constraint_case_id"] = active_case_id;
        node* propulsion_type_node = selected_case->find("engine/propulsion_type");
        config.values["propulsion_type"] =
            propulsion_type_node == nullptr
                ? "jet"
                : xml_node_text(*propulsion_type_node);

        xml_map_value(config, *selected_case,
            "aerodynamic_polar_xml_path",
            "aircraft/aerodynamics/polar_xml_path");
        xml_map_value(config, *selected_case,
            "reference_wing_id",
            "aircraft/aerodynamics/reference_wing_ID");
        xml_map_value(config, *selected_case,
            "mission_csv_path",
            "mission/mission_csv_path");
        if (config.values["propulsion_type"] == "propeller")
        {
            xml_map_value(config, *selected_case,
                "propeller_deck_path",
                "engine/propeller/deck_path");
            xml_map_value(config, *selected_case,
                "propeller_diameter_m",
                "engine/propeller/diameter");
            xml_map_value(config, *selected_case,
                "propeller_tip_mach_limit",
                "engine/propeller/tip_mach_limit");
            xml_map_value(config, *selected_case,
                "propeller_count",
                "engine/propeller/count");
            xml_map_value(config, *selected_case,
                "propeller_takeoff_rpm",
                "engine/propeller/takeoff/RPM");
        }
        else
        {
            xml_map_value(config, *selected_case,
                "engine_directory_path",
                "engine/engine_directory_path");
        }

        xml_map_value(config, *selected_case,
            "wing_loading_min",
            "design_space/wing_loading_min");
        xml_map_value(config, *selected_case,
            "wing_loading_max",
            "design_space/wing_loading_max");
        xml_map_value(config, *selected_case,
            "wing_loading_step",
            "design_space/wing_loading_step");

        // A case may either contain its own <standard_set> (legacy format)
        // or reference a shared set through <constraint_set_ref>.
        // Shared sets keep repeated constraint definitions out of every case.
        node* standard_set_node = selected_case->find("constraints/standard_set");

        if (standard_set_node == nullptr)
        {
            node* set_ref_node = selected_case->find("constraints/constraint_set_ref");
            if (set_ref_node == nullptr)
            {
                throw std::runtime_error(
                    "Selected case must define constraints/standard_set or "
                    "constraints/constraint_set_ref.");
            }

            const std::string set_id = xml_node_text(*set_ref_node);

            // First locate the shared <constraint_sets> container, then search
            // inside it for the requested <standard_set>. Searching the whole
            // document with a combined path is not handled reliably by aixml.
            node* constraint_sets_node = document->find("constraint_sets");
            if (constraint_sets_node != nullptr)
            {
                standard_set_node = constraint_sets_node->find(
                    "standard_set@ID=" + set_id, 1);
            }

            if (standard_set_node == nullptr)
            {
                throw std::runtime_error(
                    "Could not find shared standard_set with ID: " + set_id);
            }
        }

        const std::string standard_set = "";

        xml_map_value(config, *standard_set_node,
            "takeoff_altitude_m",
            standard_set + "takeoff_ground_roll/altitude");
        xml_map_value(config, *standard_set_node,
            "takeoff_runway_m",
            standard_set + "takeoff_ground_roll/takeoff_field_length");
        xml_map_value(config, *standard_set_node,
            "takeoff_speed_factor",
            standard_set + "takeoff_ground_roll/k_TO");
        xml_map_value(config, *standard_set_node,
            "takeoff_mu_ro",
            standard_set + "takeoff_ground_roll/friction_coefficient");
        xml_map_value(config, *standard_set_node,
            "takeoff_cd_ground",
            standard_set + "takeoff_ground_roll/ground_drag_coefficient");

        xml_map_value(config, *standard_set_node,
            "landing_altitude_m",
            standard_set + "landing_field_length/altitude");
        xml_map_value(config, *standard_set_node,
            "landing_runway_m",
            standard_set + "landing_field_length/landing_field_length");
        xml_map_value(config, *standard_set_node,
            "landing_speed_factor",
            standard_set + "landing_field_length/k_TD");
        xml_map_value(config, *standard_set_node,
            "landing_mu_brake",
            standard_set + "landing_field_length/friction_coefficient");
        xml_map_value(config, *standard_set_node,
            "landing_cd_brake",
            standard_set + "landing_field_length/braking_drag_coefficient");

        xml_map_value(config, *standard_set_node,
            "stall_speed_limit_ms",
            standard_set + "stall_speed/stall_speed_limit");

        xml_map_value(config, *standard_set_node,
            "max_mach_altitude_m",
            standard_set + "max_mach/altitude");
        xml_map_value(config, *standard_set_node,
            "max_mach",
            standard_set + "max_mach/Mach");

        xml_map_value(config, *standard_set_node,
            "supercruise_altitude_m",
            standard_set + "supercruise/altitude");
        xml_map_value(config, *standard_set_node,
            "supercruise_mach",
            standard_set + "supercruise/Mach");

        xml_map_value(config, *standard_set_node,
            "acceleration_altitude_m",
            standard_set + "horizontal_acceleration/altitude");
        xml_map_value(config, *standard_set_node,
            "acceleration_speed_ms",
            standard_set + "horizontal_acceleration/speed");
        xml_map_value(config, *standard_set_node,
            "acceleration_ms2",
            standard_set + "horizontal_acceleration/acceleration");

        xml_map_value(config, *standard_set_node,
            "cruise_altitude_m",
            standard_set + "cruise/altitude");
        xml_map_value(config, *standard_set_node,
            "cruise_speed_ms",
            standard_set + "cruise/speed");

        xml_map_value(config, *standard_set_node,
            "turn_altitude_m",
            standard_set + "constant_speed_turn/altitude");
        xml_map_value(config, *standard_set_node,
            "turn_speed_ms",
            standard_set + "constant_speed_turn/speed");
        xml_map_value(config, *standard_set_node,
            "turn_load_factor",
            standard_set + "constant_speed_turn/load_factor");

        xml_map_value(config, *standard_set_node,
            "range_available_fuel_fraction",
            standard_set + "range_fuel_fraction/available_fuel_fraction");

        return config;
    }


    constraint_input build_constraint_input_from_config(
        const text_config& config,
        Engine* engine)
    {
        constraint_input input;

        const std::filesystem::path aerodynamic_polar_xml_path =
            xml_string(config, "aerodynamic_polar_xml_path");

        const std::string reference_wing_id =
            xml_string(config, "reference_wing_id");

        const aerodynamic_aircraft_values aero_values =
            read_aerodynamic_aircraft_values(
                aerodynamic_polar_xml_path,
                reference_wing_id
            );

        const std::filesystem::path mission_csv_path =
            xml_string(config, "mission_csv_path");

        readMission mission_data(mission_csv_path);

        input.aircraft.wing_area_m2 = aero_values.wing_area_m2;
        input.aircraft.takeoff_weight_N = mission_data.total_mass.front() * 9.80665;
        input.aircraft.aspect_ratio = aero_values.aspect_ratio;
        input.aircraft.polar.cd_0 = aero_values.cd0;
        input.aircraft.polar.k = aero_values.k;
        input.aircraft.cl_max_takeoff = aero_values.cl_max_takeoff;
        input.aircraft.cl_max_landing = aero_values.cl_max_landing;

        input.engine = engine;
        const std::string propulsion = xml_string(config, "propulsion_type");
        if (propulsion == "propeller")
        {
            input.propulsion = propulsion_type::propeller;
            input.propeller.deck_path = xml_string(config, "propeller_deck_path");
            input.propeller.diameter_m = xml_double(config, "propeller_diameter_m");
            input.propeller.tip_mach_limit =
                xml_double(config, "propeller_tip_mach_limit");
            if (input.propeller.tip_mach_limit <= 0.0 ||
                input.propeller.tip_mach_limit > 1.5)
            {
                throw std::runtime_error(
                    "propeller tip_mach_limit must be in (0, 1.5].");
            }
            input.propeller.count = static_cast<int>(
                xml_double(config, "propeller_count"));
            input.propeller.takeoff.rpm =
                xml_double(config, "propeller_takeoff_rpm");
        }
        else if (propulsion == "jet")
        {
            input.propulsion = propulsion_type::jet;
        }
        else
        {
            throw std::runtime_error(
                "Unsupported propulsion_type: " + propulsion);
        }

        input.wing_loading_min = xml_double(config, "wing_loading_min");
        input.wing_loading_max = xml_double(config, "wing_loading_max");
        input.wing_loading_step = xml_double(config, "wing_loading_step");

        input.takeoff.altitude_m = xml_double(config, "takeoff_altitude_m");
        input.takeoff.runway_m = xml_double(config, "takeoff_runway_m");
        input.takeoff.speed_factor = xml_double(config, "takeoff_speed_factor");
        input.takeoff.beta_to = mission_data.get_beta("takeoff", input.takeoff.altitude_m);
        input.takeoff.mu_ro = xml_double(config, "takeoff_mu_ro");
        input.takeoff.cd_ground = xml_double(config, "takeoff_cd_ground");

        input.landing.altitude_m = xml_double(config, "landing_altitude_m");
        input.landing.runway_m = xml_double(config, "landing_runway_m");
        input.landing.speed_factor = xml_double(config, "landing_speed_factor");
        input.landing.mu_brake = xml_double(config, "landing_mu_brake");
        input.landing.cd_brake = xml_double(config, "landing_cd_brake");
        input.landing.beta_landing = mission_data.get_beta("landing", input.landing.altitude_m);

        // Stall-speed constraint uses the same condition as landing.
        // Only the maximum allowed stall speed remains a requirement input.
        // Altitude and beta are inherited automatically from the landing case
        // to avoid entering the same physical condition twice.
        input.stall_speed.altitude_m = input.landing.altitude_m;
        input.stall_speed.speed_limit_ms = xml_double(config, "stall_speed_limit_ms");
        input.stall_speed.beta_stall = input.landing.beta_landing;

        input.max_mach.altitude_m = xml_double(config, "max_mach_altitude_m");
        input.max_mach.mach = xml_double(config, "max_mach");
        input.max_mach.beta_max_mach = mission_data.get_beta("cruise");

        input.supercruise.altitude_m = xml_double(config, "supercruise_altitude_m");
        input.supercruise.mach = xml_double(config, "supercruise_mach");
        input.supercruise.beta_supercruise = mission_data.get_beta("cruise");

        input.acceleration.altitude_m = xml_double(config, "acceleration_altitude_m");
        input.acceleration.speed_ms = xml_double(config, "acceleration_speed_ms");
        input.acceleration.acceleration_ms2 = xml_double(config, "acceleration_ms2");
        input.acceleration.beta_acceleration = mission_data.get_beta("cruise");

        input.cruise.altitude_m = xml_double(config, "cruise_altitude_m");
        input.cruise.speed_ms = xml_double(config, "cruise_speed_ms");
        input.cruise.beta_cruise = mission_data.get_beta("cruise", input.cruise.altitude_m);

        // Gust constraint uses the cruise flight condition automatically.
        // All remaining gust quantities are derived in compute_gust_constraint_limit().
        input.gust.altitude_m = input.cruise.altitude_m;
        input.gust.speed_ms = input.cruise.speed_ms;
        input.gust.beta_gust = input.cruise.beta_cruise;

        input.climb.mission_points = mission_data.get_climb_conditions();
        input.climb.representative_point = *std::max_element(
            input.climb.mission_points.begin(),
            input.climb.mission_points.end(),
            [](const climb_mission_point& left,
               const climb_mission_point& right)
            {
                constexpr double g = 9.80665;
                const double left_energy =
                    left.roc_ms / left.speed_ms +
                    left.acceleration_ms2 / g;
                const double right_energy =
                    right.roc_ms / right.speed_ms +
                    right.acceleration_ms2 / g;
                return left_energy < right_energy;
            });

        input.turn.altitude_m = xml_double(config, "turn_altitude_m");
        input.turn.speed_ms = xml_double(config, "turn_speed_ms");
        input.turn.load_factor = xml_double(config, "turn_load_factor");
        input.turn.beta_turn = mission_data.get_beta("cruise");

        input.range.altitude_m = mission_data.get_range_weighted_altitude();
        input.range.speed_ms = mission_data.get_range_weighted_tas();
        input.range.range_m = mission_data.get_total_range();
        input.range.beta_start = mission_data.get_beta("cruise", input.range.altitude_m);
        input.range.available_fuel_fraction = xml_double(config, "range_available_fuel_fraction");

        std::cout << "Using UNICADO aerodynamics library.\n";
        std::cout << "Aerodynamic polar XML: "
                << aerodynamic_polar_xml_path.string() << '\n';
        std::cout << "Reference wing ID = " << reference_wing_id << '\n';
        std::cout << "Wing reference area from aerodynamics = "
                  << aero_values.wing_area_m2 << " m^2\n";
        std::cout << "CD0 from aerodynamics = "
                << input.aircraft.polar.cd_0 << '\n';
        std::cout << "Induced drag factor k from aerodynamics = "
                << input.aircraft.polar.k << '\n';
        std::cout << "Takeoff CLmax from aerodynamics = "
                << aero_values.cl_max_takeoff << '\n';
        std::cout << "Landing CLmax from aerodynamics = "
                << aero_values.cl_max_landing << '\n';
        std::cout << "Using mission CSV: "
                << mission_csv_path.string() << '\n';
        std::cout << "Mission beta takeoff = "
                << input.takeoff.beta_to << '\n';
        std::cout << "Mission beta cruise = "
                << input.cruise.beta_cruise << '\n';
        std::cout << "Mission beta landing = "
                << input.landing.beta_landing << '\n';
        std::cout << "Stall speed limit = "
                << input.stall_speed.speed_limit_ms << " m/s "
                << "at landing altitude = "
                << input.stall_speed.altitude_m << " m, beta = "
                << input.stall_speed.beta_stall << '\n';
        std::cout << "Gust constraint flight condition inherited from cruise: "
                << "V = " << input.gust.speed_ms << " m/s, altitude = "
                << input.gust.altitude_m << " m, beta = "
                << input.gust.beta_gust << '\n';
        const auto& representative_climb = input.climb.representative_point;
        std::cout << "Climb constraint scans "
                  << input.climb.mission_points.size()
                  << " mission climb points. Highest kinematic demand point: "
                  << "V = " << representative_climb.speed_ms
                  << " m/s, ROC = " << representative_climb.roc_ms
                  << " m/s, dV/dt = "
                  << representative_climb.acceleration_ms2
                  << " m/s^2, altitude = "
                  << representative_climb.altitude_m
                  << " m, beta = "
                  << representative_climb.beta_climb << '\n';
        std::cout << "Mission range from CSV = "
                << input.range.range_m << " m\n";
        std::cout << "Range-weighted altitude from mission CSV = "
                << input.range.altitude_m << " m\n";
        std::cout << "Range-weighted TAS from mission CSV = "
                << input.range.speed_ms << " m/s\n";
                        
        return input;
    }

    void readMission::read_mission_data()
    {
        std::ifstream file(this->missionCSV);

        if (!file.is_open())
        {
            throw std::runtime_error(
                "Could not open mission CSV file: " +
                this->missionCSV.string());
        }

        std::string line;

        std::getline(file, line);

        std::istringstream header_stream(line);
        std::string column;

        int altitude_index = -1;
        int mass_index = -1;
        int mode_index = -1;
        int range_index = -1;
        int tas_index = -1;
        int time_index = -1;
        int roc_index = -1;
        int index = 0;

        while (std::getline(header_stream, column, ';'))
        {
            column = trim_copy(column);

            if (column == "Altitude [m]")
            {
                altitude_index = index;
            }
            else if (column == "Total mass [kg]")
            {
                mass_index = index;
            }
            else if (column == "Mode name [-]")
            {
                mode_index = index;
            }
            else if (column == "Range [m]")
            {
                range_index = index;
            }
            else if (column == "TAS [m/s]")
            {
                tas_index = index;
            }
            else if (column == "Time [s]")
            {
                time_index = index;
            }
            else if (column == "ROC [fpm]")
            {
                roc_index = index;
            }

            ++index;
        }

        if (altitude_index < 0 ||
            mass_index < 0 ||
            mode_index < 0 ||
            range_index < 0 ||
            tas_index < 0 ||
            time_index < 0 ||
            roc_index < 0)
        {
            throw std::runtime_error(
                "Mission CSV must contain Time [s], Range [m], Altitude [m], "
                "TAS [m/s], ROC [fpm], Total mass [kg], and Mode name [-].");
        }

        while (std::getline(file, line))
        {
            std::istringstream line_stream(line);
            std::string value;
            std::vector<std::string> row;

            while (std::getline(line_stream, value, ';'))
            {
                row.push_back(trim_copy(value));
            }

            const int required_size =
                std::max({range_index, altitude_index, tas_index, mass_index,
                          mode_index, time_index, roc_index}) + 1;

            if (static_cast<int>(row.size()) < required_size)
            {
                continue;
            }

            try
            {
                this->altitude.push_back(std::stod(row[altitude_index]));
                this->total_mass.push_back(std::stod(row[mass_index]));
                this->range.push_back(std::stod(row[range_index]));
                this->tas.push_back(std::stod(row[tas_index]));
                this->time_s.push_back(std::stod(row[time_index]));
                this->climb_rate_ms.push_back(
                    std::stod(row[roc_index]) * 0.00508);
                this->mode_name.push_back(row[mode_index]);
            }
            catch (...)
            {
                continue;
            }
        }

        if (this->total_mass.empty())
        {
            throw std::runtime_error("Mission CSV contains no valid data rows.");
        }
    }

    std::vector<climb_mission_point> readMission::get_climb_conditions() const
    {
        std::vector<climb_mission_point> conditions;
        const auto is_airborne_climb_row = [this](std::size_t index)
        {
            const std::string mode = trim_copy(this->mode_name[index]);
            return mode != "takeoff" && mode != "landing" &&
                this->climb_rate_ms[index] > 0.0;
        };

        for (std::size_t i = 0; i < this->altitude.size(); ++i)
        {
            if (!is_airborne_climb_row(i))
            {
                continue;
            }

            std::size_t lower = i;
            std::size_t upper = i;
            const std::string current_mode = trim_copy(this->mode_name[i]);
            if (i > 0 && trim_copy(this->mode_name[i - 1]) == current_mode)
            {
                lower = i - 1;
            }
            if (i + 1 < this->altitude.size() &&
                trim_copy(this->mode_name[i + 1]) == current_mode)
            {
                upper = i + 1;
            }

            // A central difference is used inside a climb segment. At either
            // end, the available one-sided pair is used instead.
            const double time_delta = this->time_s[upper] - this->time_s[lower];
            if (lower == upper || time_delta <= 0.0)
            {
                continue;
            }

            climb_mission_point point;
            point.altitude_m = this->altitude[i];
            point.speed_ms = this->tas[i];
            point.roc_ms = this->climb_rate_ms[i];
            point.acceleration_ms2 =
                (this->tas[upper] - this->tas[lower]) / time_delta;
            point.beta_climb =
                this->total_mass[i] / this->total_mass.front();

            if (std::isfinite(point.altitude_m) &&
                std::isfinite(point.speed_ms) && point.speed_ms > 0.0 &&
                std::isfinite(point.roc_ms) && point.roc_ms > 0.0 &&
                std::isfinite(point.acceleration_ms2) &&
                std::isfinite(point.beta_climb) && point.beta_climb > 0.0)
            {
                conditions.push_back(point);
            }
        }

        if (conditions.empty())
        {
            throw std::runtime_error(
                "Mission CSV contains no valid airborne positive-ROC points.");
        }

        return conditions;
    }

    auto readMission::get_beta(
        const std::string segment,
        const double altitude) -> const double
    {
        const std::string wanted_segment = trim_copy(segment);

        for (std::size_t i = 0; i < this->altitude.size(); ++i)
        {
            if (trim_copy(this->mode_name[i]) == wanted_segment &&
                this->altitude[i] >= altitude)
            {
                return this->total_mass[i] / this->total_mass.front();
            }
        }

        throw std::runtime_error(
            "Could not find mission beta for segment: " +
            wanted_segment);
    }

    auto readMission::get_beta(
        const std::string segment_from) -> const double
    {
        const std::string wanted_segment = trim_copy(segment_from);

        for (std::size_t i = 1; i < this->mode_name.size(); ++i)
        {
            if (trim_copy(this->mode_name[i - 1]) == wanted_segment &&
                trim_copy(this->mode_name[i]) != wanted_segment)
            {
                return this->total_mass[i] / this->total_mass.front();
            }
        }

        throw std::runtime_error(
            "Could not find mission beta after segment: " +
            wanted_segment);
    }

    double readMission::get_total_range() const
    {
        if (this->range.size() < 2)
        {
            throw std::runtime_error("Mission CSV contains insufficient range data.");
        }

        double total_range = 0.0;

        for (std::size_t i = 1; i < this->range.size(); ++i)
        {
            const double delta_range = this->range[i] - this->range[i - 1];

            if (delta_range > 0.0)
            {
                total_range += delta_range;
            }
        }

        if (total_range <= 0.0)
        {
            throw std::runtime_error("Mission CSV contains no positive range increments.");
        }

        return total_range;
    }

    double readMission::get_range_weighted_altitude() const
    {
        if (this->range.size() < 2 ||
            this->altitude.size() != this->range.size())
        {
            throw std::runtime_error("Mission CSV contains inconsistent altitude/range data.");
        }

        double weighted_altitude = 0.0;
        double total_range = 0.0;

        for (std::size_t i = 1; i < this->range.size(); ++i)
        {
            const double delta_range = this->range[i] - this->range[i - 1];

            if (delta_range > 0.0)
            {
                weighted_altitude += this->altitude[i] * delta_range;
                total_range += delta_range;
            }
        }

        if (total_range <= 0.0)
        {
            throw std::runtime_error("Mission CSV contains no positive range increments for altitude weighting.");
        }

        return weighted_altitude / total_range;
    }

    double readMission::get_range_weighted_tas() const
    {
        if (this->range.size() < 2 ||
            this->tas.size() != this->range.size())
        {
            throw std::runtime_error("Mission CSV contains inconsistent TAS/range data.");
        }

        double weighted_tas = 0.0;
        double total_range = 0.0;

        for (std::size_t i = 1; i < this->range.size(); ++i)
        {
            const double delta_range = this->range[i] - this->range[i - 1];

            if (delta_range > 0.0)
            {
                weighted_tas += this->tas[i] * delta_range;
                total_range += delta_range;
            }
        }

        if (total_range <= 0.0)
        {
            throw std::runtime_error("Mission CSV contains no positive range increments for TAS weighting.");
        }

        return weighted_tas / total_range;
    }

}
