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

#ifndef INCLUDE_ENGINE_ENGINE_DATA_H_
#define INCLUDE_ENGINE_ENGINE_DATA_H_

/* === Includes === */
#include "engine_deck.h"
#include <aixml/node.h>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#ifdef BUILD_ENGINE_SHARED
#ifdef _WIN32
#define ENGINEDLLEXPORT __declspec(dllexport)
#else
#define ENGINEDLLEXPORT __attribute__((visibility("default")))
#endif
#elif defined(IMPORT_ENGINE_SHARED)
#ifdef _WIN32
#define ENGINEDLLEXPORT __declspec(dllimport)
#else
#define ENGINEDLLEXPORT
#endif
#else
#define ENGINEDLLEXPORT
#endif

/* === Classes === */

/**
 * @struct Dimensions
 * @brief Contains the dimensions of the engine.
 * @param height [m] The height of the engine.
 * @param width [m] The width of the engine.
 * @param length [m] The length of the engine.
 * @param diameter [m] The diameter of the propulsor (fan/propeller) of the engine.
 */
struct Dimensions
{
    double height{0.0};   /** [m] The height of the engine */
    double width{0.0};    /** [m] The width of the engine */
    double length{0.0};   /** [m] The length of the engine */
    double diameter{0.0}; /** [m] The diameter of the propulsor (fan/propeller) of the engine */
};

/**
 * @enum LTO phases
 * @brief Contains the four different Landing and Take-Off Cylce (LTO) stages.
 * @param takeoff // takeoff power/thrust setting for the LTO cycle
 * @param climb // climb power/thrust setting for the LTO cycle
 * @param approach // approach power/thrust setting for the LTO cycle
 * @param taxi // taxi power/thrust setting for the LTO cycle
 */
enum class LTOPhases : unsigned int
{
  takeoff, 
  climb,
  approach,
  taxi
};

class ENGINEDLLEXPORT EngineData
{
  public:
    /* === Constructors === */
    /**
     * @brief Construct a new Engine Data object
     * @note This will parse **ALL** the deck values.
     *
     * @param engine_xml The XML data of the engine.
     */
    explicit EngineData(const std::shared_ptr<node> &engine_xml);

    /**
     * @brief Construct a new Engine Data object
     * @note This will not parse the deck values immediately
     * when `lazy_load_deck` is set to true. Otherwise, all
     * deck values are parsed immediately.
     *
     * @param engine_xml The XML data of the engine.
     * @param lazy_load_deck If true, the deck values are only parsed when requested.
     */
    explicit EngineData(const std::shared_ptr<node> &engine_xml, const bool lazy_load_deck);

    /* === Default Constructors and Assignment Operators */
    EngineData(const EngineData &other) = default;
    auto operator=(const EngineData &other) -> EngineData & = default;
    EngineData(EngineData &&other) = default;
    auto operator=(EngineData &&other) -> EngineData & = default;
    ~EngineData() = default;

    /* === Getters === */
    /**
     * @brief The design operating point of the engine.
     *
     * @return OperatingPoint [-, m, -] The design operating point.
     */
    [[nodiscard]] auto design_point() const -> OperatingPoint;

    /**
     * @brief The design thrust of the engine.
     *
     * @return double The design thrust [N].
     */
    [[nodiscard]] auto design_thrust() const -> double;

    /**
     * @brief The dimensions of the engine.
     * @note The diameter refers to either the fan or the propeller.
     * This function selects the larger of the two values of the
     * engine XML assuming that one is set to zero.
     *
     * @return Dimensions [m, m, m, m] The dimensions (height, width, length, diameter_propulsor)
     * of the engine.
     */
    [[nodiscard]] auto dimensions() const -> Dimensions;

    /**
     * @brief The dry mass of the engine.
     *
     * @return double [kg] The dry mass.
     */
    [[nodiscard]] auto dry_mass() const -> double;

    /**
     * @brief The maximum continuous thrust (MCT) of the engine
     * measured at sea level and Mach = 0.
     *
     * @return double The Maximum Continuous Thrust [N].
     */
    [[nodiscard]] auto MCT() const -> double;

    /**
     * @brief Get the name of the engine.
     * @details The name is defined by the filename
     * of the engine XML file, which also should
     * correspond to the name of the engine directory.
     *
     * @return std::string The name of the engine.
     */
    [[nodiscard]] auto name() const -> std::string;

    /**
     * @brief The static thrust (SLST) of the engine measured
     * at sea level and Mach = 0.
     *
     * @return double The Sea Level Static Thrust [N].
     */
    [[nodiscard]] auto SLST() const -> double;

     /**
     * @brief The Nominal RPM of N1.
     *
     * @return double The N1 Nominal [RPM].
     */
    [[nodiscard]] auto N1_Nominal() const -> double;
    /**
     * @brief Minimum fraction of fuel flow and p3 to prevent flame-out
     *
     * @return double fuelflow/P3 [-].
     */
    [[nodiscard]] auto fuelflow_to_p3_ratio() const -> double;

    /**
     * @brief Maximum ratio of fuel flow to flow pressure after compression in take-off rating
     *
     * @return double ratio fuelflow/P3 [-].
     */
    [[nodiscard]] auto fuelflow_to_P3max_ratio_at_MTO() const -> double;

    /**
     * @brief Maximum ratio of fuel flow to flow pressure after compression in maximum continious rating.
     *
     * @return double ratio fuelflow/P3 [-].
     */
    [[nodiscard]] auto fuelflow_to_P3max_at_MCT() const -> double;

    /**
     * @brief Temperature limit at turbine exit (T49 or T5). Calculated by returning UnscaledSLST at TO-rating and ambient conditions at FlatRatingTemp_MTO
     *
     * @return double maximum turbine exit temperature during take-off [K].
     */
    [[nodiscard]] auto max_temperature_MTO() const -> double;

    /**
     * @brief Temperature limit at turbine exit (T49 or T5). Calculated by returning Maximum Continuous Thrust at MCT-rating and ambient conditions at FlatRatingTemp_MCT
     *
     * @return double maximum turbine exit temperature for continous thrut [K].
     */
    [[nodiscard]] auto max_temperature_MCT() const -> double;

    /**
     * @brief Maximum relative rotational speed for high pressure rotor - Limit in TCDS
     *
     * @return double maximum relative rotational speed for high pressure rotor [-].
     */
    [[nodiscard]] auto max_N2() const -> double;

    /**
     * @brief Maximum bleed air extraction of the engine for aircraft onboard systems as a fraction of core mass flow
     *
     * @return double maximum relative bleed based on core mass flow [-]
     */
    [[nodiscard]] auto max_relative_bleed() const -> double;

    /**
     * @brief Maximum shaft power extraction of the engine for aircraft onboard systems
     *
     * @return double maximum possible shaft power extraction [W].
     */
    [[nodiscard]] auto max_shaft_power_extraction() const -> double;

    /**
     * @brief Fraction of N_Climb/N_MCT; calculated such that N_Climb is about 0.9*N1 at SL, Ma=0, and ISA conditions
     *
     * @return double ratio of spool speed in climb to spool spped at MCT [-].
     */
    [[nodiscard]] auto fraction_N_to_climbrating() const -> double;

    /**
     * @brief Fraction of N_Cruise/N_MCT; calculated such that Cruise thrust is met at design point (cf. EngineDesignCondition)
     *
     * @return double ratio of spool speed in cruise to spool spped at MCT [-].
     */
    [[nodiscard]] auto fraction_N_to_cruiserating() const -> double;

    /**
     * @brief Unscaled Flat-Rated static thrust of the deck
     *
     * @return double unscaled SLST [N].
     */
    [[nodiscard]] auto unscaled_SLST() const -> double;

    /**
     * @brief Bypass Ratio
     *
     * @return double BPR [-].
     */
    [[nodiscard]] auto BPR() const -> double;

    /**
     * @brief Fuel flow according to the LTO cycle in the given LTO phase scaled using the scale factor.
     *
     * @return double fuelflow [kg/s].
     */
    [[nodiscard]] auto LTO_fuel_flow(const LTOPhases LTO_phase) const -> double;

    /**
     * @brief Emission of unburned hydrocarbons according to the LTO cycle.
     *
     * @return double HC [kg/kgFuel].
     */
    [[nodiscard]] auto LTO_emission_HC(const LTOPhases LTO_phase) const -> double;

    /**
     * @brief Emission of carbon monoxide according to the LTO cycle.
     *
     * @return double CO [kg/kgFuel].
     */
    [[nodiscard]] auto LTO_emission_CO(const LTOPhases LTO_phase) const -> double;

    /**
     * @brief Emission of nitrogen oxides according to the LTO cycle.
     *
     * @return double NOx [kg/kgFuel].
     */
    [[nodiscard]] auto LTO_emission_NOx(const LTOPhases LTO_phase) const -> double;

    /**
     * @brief Emission of smoke number according to the LTO cycle.
     *
     * @return double SN [-].
     */
    [[nodiscard]] auto LTO_emission_SN(const LTOPhases LTO_phase) const -> double;

    /**
     * @brief TODO: Tobi/Katrin: add description
     *
     * @return TODO: Tobi/Katrin: add description [?].
     */
    [[nodiscard]] auto get_NOx_severity_parameter() const -> double;

    /**
     * @brief Get the value of a deck at the specified operating point.
     * @note The deck values have to be parsed before this const function is called!
     *
     * @param deck_value The name of the deck value.
     * @param op_point The current operating point of the engine.
     * @return double The value of the deck at the specified operating point.
     * The unit of the value is the same as the unit of the deck value.
     */
    [[nodiscard]] auto get_deck_value(const std::string &deck_value, const OperatingPoint op_point) const -> double;

    /**
     * @brief Get the value of a deck at the specified operating point.
     * @note The deck values are lazy parsed on this non-const version of the function.
     *
     * @param deck_value The name of the deck value.
     * @param op_point The current operating point of the engine.
     * @return double The value of the deck at the specified operating point.
     * The unit of the value is the same as the unit of the deck value.
     */
    [[nodiscard]] auto get_deck_value(const std::string &deck_value, const OperatingPoint op_point) -> double;

    /**
     * @brief Get the minimal possible oparting point of the deck value.
     *
     * @param deck_value The name of the deck value.
     * @return OperatingPoint The lowest possible N, Ma & FL for this deck value
     */
    [[nodiscard]] auto get_lower_operating_point(const std::string &value_name) -> OperatingPoint;

    /**
     * @brief Get the maximal possible oparting point of the deck value.
     *
     * @param deck_value The name of the deck value.
     * @return OperatingPoint The highest possible N, Ma & FL for this deck value
     */
    [[nodiscard]] auto get_upper_operating_point(const std::string &value_name) -> OperatingPoint;

    

  private:
    /* === Member functions === */
    /**
     * @brief Template function to get a value from the XML file.
     * The path to the value is extracted from the paths_ map.
     * @note A default constructed value is returned if the node
     * was not found.
     *
     * @tparam T The value type of the extracted value.
     * @param map_name The name of the value in the paths_ map.
     * @return T The extracted value.
     */
    template <typename T>
    [[nodiscard]] auto get_value(const std::string &map_name) const -> T
    {
        /* Find the node in the xml data */
        node *const value = this->xml_->find(this->paths_.at(map_name));

        /* Return the value if the node was found */
        return value != nullptr ? T{*value} : T{};
    }

    /**
     * @brief Extract the unit of a value from the XML file.
     * The path to the value is extracted from the paths_ map.
     * @note An empty string is returned if the node unit was not found.
     *
     * @param map_name The name of the value in the paths_ map.
     * @return std::string The extracted unit if existing. Empty string otherwise.
     */
    [[nodiscard]] auto get_unit(const std::string &map_name) const -> std::string;

    /**
     * @brief Parse the deck value provided by a CSV file.
     *
     * @param file The path to the CSV file.
     */
    void parse_deck(const std::filesystem::path &file);

    /**
     * @brief Parse the deck values provided by CSV files in
     * the current engine directory.
     * @note The CSV files must be named after the deck values.
     *
     * @return bool True if the parsing was successful.
     */
    auto parse_decks() -> bool;

    /* === Member variables === */
    std::unordered_map<std::string, DeckValue> deck_values_;
    std::unordered_map<std::string, std::string> paths_;
    std::shared_ptr<node> xml_;
};

#endif // INCLUDE_ENGINE_ENGINE_DATA_H_

