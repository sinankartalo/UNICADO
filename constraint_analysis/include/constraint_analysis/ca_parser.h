#pragma once

#include "constraint_analysis/constraint_analysis.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace constraint_analysis
{
    struct text_config
    {
        std::map<std::string, std::string> values;
    };


    text_config read_xml_config(
        const std::filesystem::path& path,
        const std::string& case_override_id = "");

    std::string xml_string(
        const text_config& config,
        const std::string& key);

    double xml_double(
        const text_config& config,
        const std::string& key);

    constraint_input build_constraint_input_from_config(
        const text_config& config,
        Engine* engine);

    struct mission_climb_condition
    {
        double altitude_m = 0.0;
        double speed_ms = 0.0;
        double climb_rate_ms = 0.0;
        double acceleration_ms2 = 0.0;
        double beta = 1.0;
    };

    class readMission
    {
    public:
        const std::filesystem::path missionCSV;
        std::vector<double> altitude;
        std::vector<double> total_mass;
        std::vector<double> range;
        std::vector<double> tas;
        std::vector<double> time_s;
        std::vector<double> climb_rate_ms;
        std::vector<std::string> mode_name;

        double get_total_range() const;
        double get_range_weighted_altitude() const;
        double get_range_weighted_tas() const;
        mission_climb_condition get_climb_condition(
            double altitude_m) const;

        readMission(const std::filesystem::path missionCSV) : missionCSV(missionCSV)
        {
            read_mission_data();
        }

        void read_mission_data();

        auto get_beta(const std::string segment, const double altitude) -> const double;

        auto get_beta(const std::string segment_from) -> const double;
    };
}
