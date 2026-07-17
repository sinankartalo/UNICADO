/*
 * UNICADO - UNIversity Conceptual Aircraft Design and Optimization
 *
 * Copyright (C) 2025 UNICADO consortium
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Description:
 * This file is part of UNICADO.
 */

#include "engine/engine_data.h"
#include "SI_units.h"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <ranges>

/* === XML Interface === */
namespace xmlv2
{
    auto create_paths_map() -> std::unordered_map<std::string, std::string>
    {
        std::unordered_map<std::string, std::string> paths;
        paths.emplace("design_altitude", "EngineDesignCondition/flightAltitude");
        paths.emplace("design_Mach", "EngineDesignCondition/flightMachNumber");
        paths.emplace("design_thrust", "EngineDesignCondition/thrust");
        paths.emplace("diameter_fan", "Geometry/jetSpecific/FanDiameter");
        paths.emplace("diameter_propeller", "Geometry/propSpecific/PropDiameter");
        paths.emplace("dry_mass", "MassProperties/Dry_mass");
        paths.emplace("height_engine", "Geometry/OuterDimensions/h_Engine");
        paths.emplace("length_engine", "Geometry/OuterDimensions/l_Engine");
        paths.emplace("MCT", "EngineDesignCondition/MCT");
        paths.emplace("SLST", "EngineDesignCondition/SLST");
        paths.emplace("width_engine", "Geometry/OuterDimensions/w_Engine");
        paths.emplace("WF_to_P3_min", "Deck/WFToP3min");
        paths.emplace("WF_to_P3_max_at_MTO", "Deck/WFToP3max_MTO");
        paths.emplace("WF_to_P3_max_at_MCT", "Deck/WFToP3max_MCT");
        paths.emplace("max_temperature_MTO", "Deck/TempMax_MTO");
        paths.emplace("max_temperature_MCT", "Deck/TempMax_MCT");
        paths.emplace("max_N2", "Deck/N2max");
        paths.emplace("max_relative_bleed", "Deck/RelBleedMax");
        paths.emplace("max_shaft_power_extration", "Deck/ShaftPowerExtractionMax");
        paths.emplace("fraction_N_to_climbrating", "Deck/NfractionClimbRating");
        paths.emplace("fraction_N_to_cruiserating", "Deck/NfractionCruiseRating");
        paths.emplace("unscaled_SLST", "Deck/UnscaledSLST");
        paths.emplace("N1Nominal", "Deck/N1Nominal");
        paths.emplace("BPR", "EngineDesignCondition/BPR");
        paths.emplace("LTO_fuel_flow_taxi", "ICAOEngineData/LTOFuelFlow/Taxi");
        paths.emplace("LTO_fuel_flow_takeoff", "ICAOEngineData/LTOFuelFlow/Takeoff");
        paths.emplace("LTO_fuel_flow_climbout", "ICAOEngineData/LTOFuelFlow/ClimbOut");
        paths.emplace("LTO_fuel_flow_approach", "ICAOEngineData/LTOFuelFlow/Approach");
        paths.emplace("LTO_emissions_HC_taxi", "ICAOEngineData/LTOEmissions/HCFactor/Taxi");
        paths.emplace("LTO_emissions_HC_takeoff", "ICAOEngineData/LTOEmissions/HCFactor/Takeoff");
        paths.emplace("LTO_emissions_HC_climbout", "ICAOEngineData/LTOEmissions/HCFactor/ClimbOut");
        paths.emplace("LTO_emissions_HC_approach", "ICAOEngineData/LTOEmissions/HCFactor/Approach");
        paths.emplace("LTO_emissions_CO_taxi", "ICAOEngineData/LTOEmissions/COFactor/Taxi");
        paths.emplace("LTO_emissions_CO_takeoff", "ICAOEngineData/LTOEmissions/COFactor/Takeoff");
        paths.emplace("LTO_emissions_CO_climbout", "ICAOEngineData/LTOEmissions/COFactor/ClimbOut");
        paths.emplace("LTO_emissions_CO_approach", "ICAOEngineData/LTOEmissions/COFactor/Approach");
        paths.emplace("LTO_emissions_NOx_taxi", "ICAOEngineData/LTOEmissions/NOxFactor/Taxi");
        paths.emplace("LTO_emissions_NOx_takeoff", "ICAOEngineData/LTOEmissions/NOxFactor/Takeoff");
        paths.emplace("LTO_emissions_NOx_climbout", "ICAOEngineData/LTOEmissions/NOxFactor/ClimbOut");
        paths.emplace("LTO_emissions_NOx_approach", "ICAOEngineData/LTOEmissions/NOxFactor/Approach");
        paths.emplace("LTO_emissions_SN_taxi", "ICAOEngineData/LTOEmissions/SNFactor/Taxi");
        paths.emplace("LTO_emissions_SN_takeoff", "ICAOEngineData/LTOEmissions/SNFactor/Takeoff");
        paths.emplace("LTO_emissions_SN_climbout", "ICAOEngineData/LTOEmissions/SNFactor/ClimbOut");
        paths.emplace("LTO_emissions_SN_approach", "ICAOEngineData/LTOEmissions/SNFactor/Approach");
        paths.emplace("severity_parameter_NOx", "EmissionFactors/NOxFactor");
        return paths;
    }
} // namespace xmlv2

EngineData::EngineData(const std::shared_ptr<node> &engine_xml)
    : EngineData{engine_xml, false}
{
}

EngineData::EngineData(const std::shared_ptr<node> &engine_xml, const bool lazy_load_deck)
    : xml_{engine_xml}
{
    /* Create the map to the node paths */
    this->paths_ = xmlv2::create_paths_map();

    /* Parse the engine decks if not lazy loaded */
    if (!lazy_load_deck)
    {
        if (!this->parse_decks())
        {
            std::cerr << "Error while parsing the engine decks.\n";
        }
    }
}

auto EngineData::design_point() const -> OperatingPoint
{
    /* Return the design point */
    return {
        1.0,
        SI::length(this->get_value<double>("design_altitude"), this->get_unit("design_altitude")),
        this->get_value<double>("design_Mach")};
}

auto EngineData::design_thrust() const -> double
{
    /* Return the value in [N] */
    const std::string map_name = "design_thrust";
    return SI::force(this->get_value<double>(map_name), this->get_unit(map_name));
}

auto EngineData::dimensions() const -> Dimensions
{
    /* Read both fan and propeller diameter and decide which is valid */
    const double diameter_fan = SI::length(this->get_value<double>("diameter_fan"), this->get_unit("diameter_fan"));
    const double diameter_propeller = SI::length(this->get_value<double>("diameter_propeller"), this->get_unit("diameter_propeller"));

    /* Return the dimensions in [m] */
    return {
        SI::length(this->get_value<double>("height_engine"), this->get_unit("height_engine")),
        SI::length(this->get_value<double>("width_engine"), this->get_unit("width_engine")),
        SI::length(this->get_value<double>("length_engine"), this->get_unit("length_engine")),
        std::max({diameter_fan, diameter_propeller})};
}

auto EngineData::dry_mass() const -> double
{
    /* Return the value in [kg] */
    const std::string map_name = "dry_mass";
    return SI::mass(this->get_value<double>(map_name), this->get_unit(map_name));
}

auto EngineData::MCT() const -> double
{
    /* Return the value in [N] */
    const std::string map_name = "MCT";
    return SI::force(this->get_value<double>(map_name), this->get_unit(map_name));
}

auto EngineData::name() const -> std::string
{
    std::filesystem::path path_xml = this->xml_->getFullPath();
    return path_xml.stem().string();
}

auto EngineData::SLST() const -> double
{
    /* Return the value in [N] */
    const std::string map_name = "SLST";
    return SI::force(this->get_value<double>(map_name), this->get_unit(map_name));
}

auto EngineData::N1_Nominal() const -> double
{
    /* Return the value in [N] */
    const std::string map_name = "N1nominal";
    return this->get_value<double>(map_name);
}

auto EngineData::fuelflow_to_p3_ratio() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "WF_to_P3_min";
    return this->get_value<double>(map_name);
}

auto EngineData::fuelflow_to_P3max_ratio_at_MTO() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "WF_to_P3_max_at_MTO";
    return this->get_value<double>(map_name);
}

auto EngineData::fuelflow_to_P3max_at_MCT() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "WF_to_P3_max_at_MCT";
    return this->get_value<double>(map_name);
}

auto EngineData::max_temperature_MTO() const -> double
{
    /* Return the value in [K] */
    const std::string map_name = "max_temperature_MTO";
    return SI::temperature(this->get_value<double>(map_name), this->get_unit(map_name));
}

auto EngineData::max_temperature_MCT() const -> double
{
    /* Return the value in [K] */
    const std::string map_name = "max_temperature_MCT";
    return SI::temperature(this->get_value<double>(map_name), this->get_unit(map_name));
}

auto EngineData::max_N2() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "max_N2";
    return this->get_value<double>(map_name);
}

auto EngineData::max_relative_bleed() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "max_relative_bleed";
    return this->get_value<double>(map_name);
}

auto EngineData::max_shaft_power_extraction() const -> double
{
    /* Return the value in [W] */
    const std::string map_name = "max_shaft_power_extration";
    return SI::power(this->get_value<double>(map_name), this->get_unit(map_name));
}

auto EngineData::fraction_N_to_climbrating() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "fraction_N_to_climbrating";
    return this->get_value<double>(map_name);
}

auto EngineData::fraction_N_to_cruiserating() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "fraction_N_to_cruiserating";
    return this->get_value<double>(map_name);
}

auto EngineData::unscaled_SLST() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "unscaled_SLST";
    return this->get_value<double>(map_name);
}

auto EngineData::BPR() const -> double
{
    /* Return the value in [-] */
    const std::string map_name = "BPR";
    return this->get_value<double>(map_name);
}

auto EngineData::LTO_fuel_flow(const LTOPhases LTO_phase) const -> double
{
    /* Return emission index in [kg/s] */
    std::string map_name{};

    switch (LTO_phase) {
    case LTOPhases::taxi :
        map_name = "LTO_fuel_flow_taxi";
        return this->get_value<double>(map_name);
    case LTOPhases::takeoff :
        map_name = "LTO_fuel_flow_takeoff";
        return this->get_value<double>(map_name);
    case LTOPhases::climb :
        map_name = "LTO_fuel_flow_climbout";
        return this->get_value<double>(map_name);
    case LTOPhases::approach :
        map_name = "LTO_fuel_flow_approach";
        return this->get_value<double>(map_name);
    default:
        std::cout << "Unknown LTO phase." << std::endl;
        return 0.0;
    }
}

auto EngineData::LTO_emission_HC(const LTOPhases LTO_phase) const -> double
{
    /* Return value in [kg/kgFuel] */
    std::string map_name{};

    switch (LTO_phase) {
    case LTOPhases::taxi :
        map_name = "LTO_emissions_HC_taxi";
        return this->get_value<double>(map_name);
    case LTOPhases::takeoff :
        map_name = "LTO_emissions_HC_takeoff";
        return this->get_value<double>(map_name);
    case LTOPhases::climb :
        map_name = "LTO_emissions_HC_climbout";
        return this->get_value<double>(map_name);
    case LTOPhases::approach :
        map_name = "LTO_emissions_HC_approach";
        return this->get_value<double>(map_name);
    default:
        std::cout << "Unknown LTO phase." << std::endl;
        return 0.0;
    }
}

auto EngineData::LTO_emission_CO(const LTOPhases LTO_phase) const -> double
{
    /* Return value in [kg/kgFuel] */
    std::string map_name{};

    switch (LTO_phase) {
    case LTOPhases::taxi :
        map_name = "LTO_emissions_CO_taxi";
        return this->get_value<double>(map_name);
    case LTOPhases::takeoff :
        map_name = "LTO_emissions_CO_takeoff";
        return this->get_value<double>(map_name);
    case LTOPhases::climb :
        map_name = "LTO_emissions_CO_climbout";
        return this->get_value<double>(map_name);
    case LTOPhases::approach :
        map_name = "LTO_emissions_CO_approach";
        return this->get_value<double>(map_name);
    default:
        std::cout << "Unknown LTO phase." << std::endl;
        return 0.0;
    }
}

auto EngineData::LTO_emission_NOx(const LTOPhases LTO_phase) const -> double
{
    /* Return value in [kg/kgFuel] */
    std::string map_name{};

    switch (LTO_phase) {
    case LTOPhases::taxi :
        map_name = "LTO_emissions_NOx_taxi";
        return this->get_value<double>(map_name);
    case LTOPhases::takeoff :
        map_name = "LTO_emissions_NOx_takeoff";
        return this->get_value<double>(map_name);
    case LTOPhases::climb :
        map_name = "LTO_emissions_NOx_climbout";
        return this->get_value<double>(map_name);
    case LTOPhases::approach :
        map_name = "LTO_emissions_NOx_approach";
        return this->get_value<double>(map_name);
    default:
        std::cout << "Unknown LTO phase." << std::endl;
        return 0.0;
    }
}

auto EngineData::LTO_emission_SN(const LTOPhases LTO_phase) const -> double
{
    /* Return value in [kg/kgFuel] */
    std::string map_name{};

    switch (LTO_phase) {
    case LTOPhases::taxi :
        map_name = "LTO_emissions_SN_taxi";
        return this->get_value<double>(map_name);
    case LTOPhases::takeoff :
        map_name = "LTO_emissions_SN_takeoff";
        return this->get_value<double>(map_name);
    case LTOPhases::climb :
        map_name = "LTO_emissions_SN_climbout";
        return this->get_value<double>(map_name);
    case LTOPhases::approach :
        map_name = "LTO_emissions_SN_approach";
        return this->get_value<double>(map_name);
    default:
        std::cout << "Unknown LTO phase." << std::endl;
        return 0.0;
    }
}

auto EngineData::get_NOx_severity_parameter() const -> double
{
    const std::string map_name = "severity_parameter_NOx";
    return this->get_value<double>(map_name);
}

auto EngineData::get_unit(const std::string &map_name) const -> std::string
{
    /* Find the node in the xml data */
    node *const value = this->xml_->find(this->paths_.at(map_name));

    /* Nullptr check */
    if (value == nullptr)
    {
        return {};
    }

    /* Find the unit */
    std::string unit{};
    if (value->hasAttrib("Unit"))
    {
        /* XML v2 format */
        unit = value->getStringAttrib("Unit");
    }
    else if (value->find("unit") != nullptr)
    {
        /* XML v3 format */
        unit = static_cast<std::string>(value->at("unit"));
    }

    /* Return the value if the node was found */
    return unit;
}

auto EngineData::get_deck_value(const std::string &value_name, const OperatingPoint op_point) const -> double
{
    /* Check whether the value is provided by the engine deck */
    if (!this->deck_values_.contains(value_name))
    {
        throw std::out_of_range(
            "The specified deck value '" + value_name + "' is not available.\n"
                                                        "You called the 'const' version of this function, which cannot lazy load the deck values.\n"
                                                        "So either the deck value was not parsed at construction or the deck value does not exist.");
    }

    /* Get the deck value */
    return this->deck_values_.at(value_name).get_value_at(op_point);
}

auto EngineData::get_deck_value(const std::string &value_name, const OperatingPoint op_point) -> double
{
    /* Check whether the value has to be lazy loaded */
    if (!this->deck_values_.contains(value_name))
    {
        /* Get the current directory where the engine xml is read from */
        std::filesystem::path path_xml = this->xml_->name;
        try
        {
            /* Parse the deck */
            this->parse_deck(path_xml.parent_path() / (value_name + ".csv"));
        }
        catch (std::exception &e)
        {
            std::cerr << "Error while parsing the engine deck: " << e.what() << '\n';
        }
    }

    /* Get the deck value */
    return this->deck_values_.at(value_name).get_value_at(op_point);
}

auto EngineData::get_lower_operating_point(const std::string &value_name) -> OperatingPoint
{
     /* Check whether the value is provided by the engine deck */
    if (!this->deck_values_.contains(value_name))
    {
        throw std::out_of_range(
            "The specified deck value '" + value_name + "' in '"+ __func__ + "' is not available.\n");

        // std::stringstream errMsg;
        // errMsg << "Hello test Error." << std::endl;
        // throwError(__FILE__, __func__, __LINE__, errMsg.str());
    }

    /* Get the deck value */
    return this->deck_values_.at(value_name).lower_boundary();
}

auto EngineData::get_upper_operating_point(const std::string &value_name) -> OperatingPoint
{
     /* Check whether the value is provided by the engine deck */
    if (!this->deck_values_.contains(value_name))
    {
        throw std::out_of_range(
            "The specified deck value '" + value_name + "' in '"+ __func__ + "' is not available.\n");

        // std::stringstream errMsg;
        // errMsg << "Hello test Error." << std::endl;
        // throwError(__FILE__, __func__, __LINE__, errMsg.str());
    }

    /* Get the deck value */
    return this->deck_values_.at(value_name).upper_boundary();
}

void EngineData::parse_deck(const std::filesystem::path &file)
{
    /* Add the deck to the map */
    this->deck_values_.emplace(file.stem().string(), DeckValue{DeckData::from_csv(file)});
}

auto EngineData::parse_decks() -> bool
{
    /* The return flag */
    bool success{false};

    /* Get the current directory where the engine xml is read from */
    std::filesystem::path path_xml = this->xml_->name;
    path_xml = path_xml.parent_path().make_preferred();
    if (std::filesystem::exists(path_xml))
        path_xml = std::filesystem::absolute(path_xml);
    else
        path_xml = std::filesystem::current_path();

    try
    {
        /* Iterate over all CSV files in the directory */
        std::ranges::for_each((std::filesystem::directory_iterator{path_xml} |
                               std::views::filter([](const auto &entry)
                                                  { return entry.path().extension() == ".csv"; })),
                              [this](const auto &entry)
                              { this->parse_deck(entry.path()); });

        /* This was successful */
        success = true;
    }
    catch (std::exception &e)
    {
        std::cerr << "Error while parsing the engine decks: " << e.what() << '\n';
    }

    return success;
}
