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

#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

/* === Includes === */
#include "../engine/engine_data.h"
#include <atmosphere/atmosphere.h>
#include <string>
#include <string_view>

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


/**
 * @enum LTO emission types
 * @brief Contains the four different emission types in the LTO cycle.
 * @param HC // unburned hydrocarbons
 * @param CO // carbon monoxide
 * @param NOx // nitrogen oxides
 * @param SN // smoke number
 */
enum class EngineEmissions : unsigned int
{
  HC, // emission in [kg_emission/kg_fuel]
  CO, // emission in [kg_emission/kg_fuel]
  NOx, // emission in [kg_emission/kg_fuel]
  SN // emission in [kg_emission/kg_fuel]
};

/**
 * @enum EngineStages
 * @brief Contains the engine stages
 * @param St2 // First compressor inlet
 * @param St3 // Last compressor exit, cold side heat exchanger inlet
 * @param St4 // Burner exit
 * @param St5 // Low-pressure turbine exit after addition of cooling air
 * @param St13 // Outer stream fan exit
 * @param St22 // Low pressure compressor inlet
 * @param St25 // High-pressure compressor inlet
 * @param St45 // Intermediate turbine stator exit
 * @param St18 // Bypass nozzle throat
 * @param St8 // Nozzle throat
 */
enum class EngineStage : unsigned int
{
  St2, 
  St3,
  St4,
  St5,
  St13,
  St22,
  St25,
  St45,
  St18,
  St8
};

/**
 * @enum engine stage properties
 * @brief Contains properties of the stages of an engine
 * @param FuelToAirRatio // ratio of fuel to air
 * @param MachNumber // mach number
 * @param TotalPressure // total pressure 
 * @param TotalTemperature // total temperature
 * @param StaticTemperature // static temperature
 * @param MassFlow // mass flow
 */
enum class StageProperties : unsigned int
{
  FuelToAirRatio, // ratio in [-]
  MachNumber, // speed ratio in [-]
  TotalPressure, // pressure in [Pa]
  TotalTemperature, // temperature in [K]
  StaticTemperature, // temperature in [K]
  MassFlow // massflow in [kg/s]
};

/** @class Engine
 *   @brief Class to get access to engine parameters
 */
class ENGINEDLLEXPORT Engine
{
public:
  /* === Constructor and Destructor === */
  /**
   * @brief Construct a new Engine object.
   * It is enough to pass the path to the engine directory.
   * The engine data XML file is then automatically loaded.
   *
   * @param engine_directory The path to the engine directory.
   */
  explicit Engine(const std::filesystem::path &engine_directory);

  /**
   * @brief Construct a new Engine object with a
   * thrust scale factor.
   *
   * @param engine_directory The path to the engine directory.
   * @param scale_factor The thrust scaling to apply to this engine.
   */
  Engine(const std::filesystem::path &engine_directory, double scale_factor);

  /* === Default Constructors and Assignment Operators */
  Engine(const Engine &other) = default;
  auto operator=(const Engine &other) -> Engine & = default;
  Engine(Engine &&other) = default;
  auto operator=(Engine &&other) -> Engine & = default;
  ~Engine() = default;

  /* === Getters === */
  /**
  * @brief Get the data from the unscaled engine as defined
  * in the engine data XML file.
  * @note The unscaled engine data is not affected by the
  * current operating point!
  *
  * @return const EngineData& The unscaled engine data.
  */
  [[nodiscard]] auto get_unscaled_engine() const -> const EngineData &;

  /**
  * @brief Get the thrust scale factor of the engine.
  * @return double [-] The scale factor of the thrust.
  */
  [[nodiscard]] auto get_scale_factor() const -> double;

  /**
  * @brief Get the engine dimension with the applied scaling.
  * @details The dimensions are scaled as follows:
  * - The cross section dimensions are scaled with the square root of the scale factor.
  * - The length is scaled by scale_factor^0.4 \cite Raymer p.285 Eq. 10.1
  * @return Dimensions [m] The scaled dimensions (height, width, length, diameter)
  * of the engine.
  */
  [[nodiscard]] auto get_engine_dimensions() const -> Dimensions;

  /**
  * @brief Get the current operating point of the engine.
  *
  * @return OperatingPoint The current operating point.
  */
  [[nodiscard]] auto get_operating_point() -> OperatingPoint;

  /* === Setters === */
  /**
  * @brief Set a new operating point of the engine.
  *
  * @param op The new operating point.
  */
  void set_operating_point(const OperatingPoint &op);

  /**
  * @brief Sets the correction factors for N1, temperature and mass flow
  *
  * @param N_correction_ The correction for the spool speed
  * @param W_correction_ The correction for the mass flow
  * @param T_correction_ The correction for the temperature
  **/
 void set_correction_factors(const OperatingPoint &op, const atmosphere& atm);

  /**
  * @brief Checks if the given offtakes are within limits
  *
  **/
 void check_offtake_boundaries(const OperatingPoint &op, double bleed_air_offtake, double shaft_power_offtake);

  /**
  * @brief Get the engines low pressure spool speed in percent of MCT using offtakes.
  *
  * @param flight_level: Current flight level [m]
  * @param mach_number: Current mach_number number [-]
  * @param atm: Current atmospheric conditions
  * @param derate: Value between 1.0 and 0.1 to artificially throttle the engine [-]
  * @param thrust_rating: Current rating [takeoff, maximum_continuous, climb, cruise, idle]
  * @param bleed_air_offtake: Current bleed air offtake [kg/s]
  * @param shaft_power_offtake: Current shaft power offtake [W]
  */
  void calculate_N1_with_penalties(const double &flight_level, 
      const double &mach_number, 
      const atmosphere &atm, 
      const double &derate, 
      const std::string &thrust_rating, 
      double bleed_air_offtake, 
      double shaft_power_offtake);

  /**
  * @brief Get the low pressure shaft spool speed using a thrust limit and penalties.
  *
  * @param flight_level: Current flight level [m]
  * @param mach_number: Current mach_number number [-]
  * @param atm: Current atmospheric conditions
  * @param derate: Value between 1.0 and 0.1 to artificially throttle the engine [-]
  * @param thrust_rating: Current rating [takeoff, maximum_continuous, climb, cruise, idle]
  * @param bleed_air_offtake: Current bleed air offtake [kg/s]
  * @param shaft_power_offtake: Current shaft power offtake [W]
  * @param thrust_limit: Aircraft thrust limitation. The resulting thrust is not allowed to exceed the thrust_limit, except if thrust_limit is lower than the thrust at the lowest possible shaft speed. [N]
  */
  void calculate_N1_with_thrustlimit(const double &flight_level, 
      const double &mach_number, 
      const atmosphere &atm, 
      const double &derate, 
      const std::string &thrust_rating, 
      double bleed_air_offtake, 
      double shaft_power_offtake, 
      double thrustLimit);

  /* === Access the deck data === */
  
  /**
  * @brief Get the scaled engine thrust at the current operating point for the aircraft 
  * @return double [N] The thrust of the engine.
  */
  [[nodiscard]] auto get_thrust_aircraft() -> double;

    /**
  * @brief Get the scaled engine thrust at the current operating point for the engine
  * @return double [N] The thrust of the engine.
  */
  [[nodiscard]] auto get_thrust() -> double;

   /**
    * @brief Get the thrust lapse at specific operating point with maximum thrust
    * 
    * @param atm: Current atmospheric conditions
    * @param mach: mach_number number at the operating condition [-]
    * @param thrust_rating: Current rating [takeoff, maximum_continuous, climb, cruise, idle]
    * @param altitude: Current flight altitude [m]
    */
   
   double get_thrust_lapse(const std::string &thrust_rating, const atmosphere& atm, double mach, double altitude);
  /**
  * @brief Get the scaled fuel flow at the current operating point
  * @return fuelDeck.getVal fuel flow [kg/s] depending on the variables above
  */
  [[nodiscard]] auto get_aircraft_fuelflow() -> double;

  /**
  * @brief Get the scaled fuel flow at the current operating point
  * @return fuelDeck.getVal fuel flow [kg/s] depending on the variables above
  */
  [[nodiscard]] auto get_fuelflow() -> double;

    /**
  * @brief Get the TSFC at the current operating point for the engine
  * @return double [kg/(Ns)] The TSFC of the engine.
  */
  [[nodiscard]] auto get_tsfc() -> double;

  /**
  * @brief Get the TSFC including penalties at the current operating point for the engine
  * @return double [kg/(Ns)] The TSFC of the engine.
  */
  [[nodiscard]] auto get_tsfc_penalties(double delta_isa_T, double shaft_offtake, double bleed_offtake) -> double;

  /**
  * @brief Get the thrust at the current operating point for the engine. The thrust setting is interpolated linearly.
  * @return double [N] The thrust of the engine.
  */
  [[nodiscard]] auto get_thrust_with_lever_position(double lever_position_percent, double mach, double altitude) -> double;

  /**
  * @brief Returns the temperature of the core engine mass flow at the low-pressure turbine outlet
  * @return The temperature of the core engine mass flow at the low-pressure turbine outlet [K]
  */
  [[nodiscard]] auto get_engine_EGT() -> double;

  /**
  * @brief Returns the engine pressure ratio
  * @return The engine pressure ratio [-]
  */
  [[nodiscard]] auto get_engine_EPR() -> double;

  /**
  * @brief Returns the overall pressure ratio
  * @return The overall pressure ratio [-]
  */
  [[nodiscard]] auto get_engine_OPR() -> double;

  /**
  * @brief NOx emission index with severity factor
  * @return NOx emission index [kg_emission/kg_fuel]
  */
  [[nodiscard]] auto get_engine_NOx_emission_index() -> double;

  /**
  * @brief Function to get the LTO emission index for the emission type & phase of the LTO cycle 
  * @return double [kg_emission/kg_fuel]
  */
  [[nodiscard]] auto get_LTO_emission_index(const LTOPhases LTO_cycle, 
      const EngineEmissions emission) -> double;

  /**
  * @brief Gets the scaled fuel flow for the LTO phase
  * @return double fuelflow [kg/s]
  */
  [[nodiscard]] auto get_LTO_fuelflow(const LTOPhases LTO_cycle) -> double;

  /**
  * @brief Gets the actual N1 rotations per second
  * @return double RPS of N1 [1/s]
  */
  [[nodiscard]] auto get_N1_RPS() -> double;

  /**
  * @brief Gets the maximum shaft power that can be taken from one engine.
  * @return double power offtake [W]
  */
  [[nodiscard]] auto get_max_shaft_power_offtake_engine() -> double;

  /**
  * @brief Gets the maximum possible bleed air offtake of the engine at current operating point
  * @return double [kg/s]
  */
  [[nodiscard]] auto get_max_bleed_offtake_at_current_operating_point() -> double;

    /**
  * @brief Gets the scaled dry mass of the engine 
  * @return double [kg/s]
  */
  [[nodiscard]] auto get_dry_mass() -> double;

  /**
  * @brief Gets the chosen physical property of the engine at the chosen stage of the engine.
  * @return double Various units
  */
  [[nodiscard]] auto get_physical_properties_stage(const StageProperties stage_property, 
      const EngineStage engine_stage) -> double;

  /**
  * @brief Gets the scaled SLST.
  * @return double [N]
  */
  [[nodiscard]] auto get_scaled_SLST() -> double;


private:
  /* === Properties === */
  EngineData engine_data_;  // [-] EngineData object to get access to engine parameters
  OperatingPoint op_;       // [-] The current operating point of the engine
  std::string enginetype_;  // [-] the type of the engine
  double scale_factor_;     // [-] Scale the engine data to achieve a scaled thrust output
  int number_of_engines_;   // [-] Number of engines of this engine type installed on aircraft

  /* Values for correction */
  double shaft_offtake_factor_;             // kp factor (Scholz: FUEL CONSUMPTION DUE TO SHAFT POWER OFF-TAKES FROM THE ENGINE)
  double N_correction_;                     // factor for speed correction (s. SA Braun) */
  double W_correction_;                     // factor for mass flow correction (s. SA Braun, Gl.:3.3) */
  double T_correction_;                     // factor for thrust correction (s. SA Braun) */
  double thrust_correction_bleed_;
  double RPS_N1;
  double fuel_flow_factor_shaft_offtakes_;
  double temperature_shaft_offtake_bleed_correction_;
  double N_upper_limit_;
  double N_lower_limit_;


  /* Constants for the calculation process */
  double N1_step_;
  
  /* Limits */
  double WF_to_P3_min_;                     // Minimum relative WF/P3 (Idle)
  double WF_to_P3_max_TO_;                  // Maximum relative WF/P3 (Over-temperature) maximum take-off rating
  double WF_to_P3_max_MCT_;                 // Maximum relative WF/P3 (Over-temperature) maximum continuous thrust rating
  double max_temperature_TO_;               // Maximum allowed temperature of core mass flow at certain station (either TIT (T45) or EGT (T49) for take-off rating [K]
  double max_temperature_continuous_thrust_; // Maximum allowed temperature of core mass flow at certain station (either TIT (T45) or EGT (T49) for maximum continuous thrust rating [K]
  double N2_max_;                           // Relative upper limit of high pressure shaft speed (user defined) [-]
  double N_fraction_climb_rating_;
  double N_fraction_cruise_rating_;
  double max_relative_bleed;                // [-]
  double max_shaft_power_extraction_;       // [W]
  double unscaled_SLST_;                    // Unscaled sea level static thrust of engine from engine data
  double dry_mass_;                        // The dry weight of the engine [kg]

  /* deck names */
  std::string thrustdeck; 
  std::string fuelflow; 
  std::string core_mass_flow; 
  std::string sNOx; 
  std::string fraction_fuelflow_to_p3;
  std::string temperature_limit_deck;
  std::string N_limit_deck;


  /* internal functions */

  /**
  * @brief Combustion chamber factor NOx Severity Parameter in engine.xml
  * @return double [-] NOx factor
  */
  [[nodiscard]] auto get_engine_severity_parameter() -> double;

  /**
  * @brief Gets N1 to converged value
  * @return double N1 value [-]
  */
  [[nodiscard]] auto converge_value(const OperatingPoint &op, double goal_value, double iteration_step, std::string deckvalue) -> double;

  void set_lower_N1_limit();
  void set_upper_N1_limit(const std::string &thrust_rating);
  void set_upper_N1_limit_temperature(const std::string &thrust_rating);
  void set_upper_N1_limit_with_overall_fuelflow_pressure_ratio(const std::string &thrust_rating);
  void set_upper_N1_limit_high_pressure_spool_speed(const std::string &thrust_rating);
  void set_N1_to_rating(const std::string &thrust_rating, const double &derate);
  void calculate_offtake_correction(const double &bleed_air_offtake, 
      const double &shaft_power_offtake);
};

#endif // ENGINE_ENGINE_H_

/*    GasTurb Nomenclatur
--------------------------------------------------------------
       A           area
       alt         altitude
       amb         ambient
       ax          axial
       Bld         bleed
       BPR         bypass ratio
       corr        corrected
       C           constant value, coefficient
       C           compressor
       CFG         thrust coefficient
       Cl          cooling
       d           diameter
       dH          enthalpy difference
       dp          design point
       DC          pressure distortion coefficient
       DT          temperature distortion coefficient
       f           factor
       f           fuel
       far         fuel-air-ratio
       F           thrust
       FN          net thrust
       FG          gross thrust
       h           enthalpy
       H           high-pressure spool
       HdlBld      handling bleed
       HPC         high-pressure compressor
       HPT         high-pressure turbine
       i           inner
       IPC         intermediate-pressure compressor (booster)
       L           low-pressure spool
       Lk          leakage
       LPC         low-pressure compressor (fan)
       LPT         low-pressure turbine
       M           Mach number
       N           spool speed
       NGV         nozzle guide vane (of a turbine)
       o           outer
       P           total pressure
       prop        propulsion
       PW          shaft power
       R           gas constant
       rel         relative
       RH          reheat (afterburner)
       RNI         Reynolds number index
       s           static
       S NOx       NOx severity parameter (used for NOx emission estimates)
       SD          shaft, delivered
       SFC         specific fuel consumption
       t           (blade) tip
       t           time
       T           total temperature
       TRQ         torque
       U           blade (tip) velocity
       V           velocity
       W           mass flow
       XN          relative spool speed
        Omega       Burner loading
        e34         Burner Efficiency
        W2Rstd      Corr Engine mass flow

       Stations:
      -------------------------

       0         ambient
       1         aircraft-engine interface
       2         first compressor inlet


       21        inner stream fan exit
       13        outer stream fan exit
       16        bypass exit
       161        cold side mixer inlet
       163        cold side mixing plane
       18        bypass nozzle throat


       24        intermediate compressor exit
       25        high-pressure compressor inlet


       3         last compressor exit, cold side heat exchanger inlet
       31        burner inlet
       35        cold side heat exchanger exit

       4         burner exit
       41        first turbine stator exit = rotor inlet

               two spool engines:

       43        high-pressure turbine exit before addition of cooling air
       44        high-pressure turbine exit after addition of cooling air
       45        low-pressure turbine inlet
       49        low-pressure turbine exit before addition of cooling air

               three spool engines:

       42        high-pressure turbine exit before addition of cooling air
       43        high-pressure turbine exit after addition of cooling air
       44        intermediate turbine inlet
       45        intermediate turbine stator exit
       46        intermediate turbine exit before addition of cooling air
       47        intermediate turbine exit after addition of cooling air
       48        low-pressure turbine inlet
       49        low-pressure turbine exit before addition of cooling air
       5         low-pressure turbine exit after addition of cooling air
       6         jet pipe inlet, reheat entry for turbojet, hot side heat exchanger inlet
       61        hot side mixer inlet
       63        hot side mixing plane
       64        mixed flow, reheat entry
       7         reheat exit, hot side heat exchanger exit
       8         nozzle throat
       9         nozzle exit (convergent-divergent nozzle only)

*/