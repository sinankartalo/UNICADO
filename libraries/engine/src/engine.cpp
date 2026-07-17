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

#include "engine/engine.h"
#include <aixml/node.h>
#include <standardFiles/functions.h>
#include <runtimeInfo/runtimeInfo.h>
#include <cmath>
#include "SI_units.h"

/* === Constructors === */
Engine::Engine(const std::filesystem::path &engine_directory)
    :
    engine_data_{
          aixml::openDocument(
              engine_directory / (engine_directory.filename().string() + ".xml"))},
    op_{this->engine_data_.design_point()},
    enginetype_{"turbofan"},
    scale_factor_{1.0},
    number_of_engines_{1},
    shaft_offtake_factor_{1.},
    N_correction_{1.0},
    W_correction_{1.0},
    T_correction_{1.0},
    thrust_correction_bleed_{1.0},
    RPS_N1{0.},
    fuel_flow_factor_shaft_offtakes_{1.0},
    temperature_shaft_offtake_bleed_correction_{1.0},
    N_upper_limit_{NAN},
    N_lower_limit_{NAN},

    // Constants for the calculation process
    N1_step_{0.005},

    // Engine Limits
    WF_to_P3_min_{engine_data_.fuelflow_to_p3_ratio()},
    WF_to_P3_max_TO_{engine_data_.fuelflow_to_P3max_ratio_at_MTO()},
    WF_to_P3_max_MCT_{engine_data_.fuelflow_to_P3max_at_MCT()},
    max_temperature_TO_{engine_data_.max_temperature_MTO()},
    max_temperature_continuous_thrust_{engine_data_.max_temperature_MCT()},
    N2_max_{engine_data_.max_N2()},
    N_fraction_climb_rating_{engine_data_.fraction_N_to_climbrating()},
    N_fraction_cruise_rating_{engine_data_.fraction_N_to_cruiserating()},
    max_relative_bleed{engine_data_.max_relative_bleed()},
    max_shaft_power_extraction_{engine_data_.max_shaft_power_extraction()},
    unscaled_SLST_{engine_data_.unscaled_SLST()},
    dry_mass_{engine_data_.dry_mass()},

    // Engine deck names
    thrustdeck{engine_data_.name() + "_FN"},            //thrust
    fuelflow{engine_data_.name()+"_WF"},                //fuel flow
    core_mass_flow{engine_data_.name()+"_W25"},         //core mass flow
    sNOx{engine_data_.name()+"_sNOx"},                  //severity NOX parameter
    fraction_fuelflow_to_p3{engine_data_.name()+"_WFqP3"}, // Ratio of fuel flow to P3
    temperature_limit_deck{engine_data_.name()+"_St5_T"},  //temperature limit
    //temperature_limit_deck{engine_data_.get_get_temperature_limit_deck_name()},
    N_limit_deck{engine_data_.name()+"_XN_HPC"}         // rel values of N2
{
}

Engine::Engine(const std::filesystem::path &engine_directory, const double scale_factor)
: Engine{engine_directory}
{
    this->scale_factor_ = scale_factor;
}

// === Getters ===
auto Engine::get_unscaled_engine() const -> const EngineData &
{
    return this->engine_data_;
}

auto Engine::get_scale_factor() const -> double
{
    return this->scale_factor_;
}

auto Engine::get_engine_dimensions() const -> Dimensions
{
    // The dimensions as defined in the XML file
    auto dimensions_unscaled = this->engine_data_.dimensions();
    // Scale the dimensions using the thrust scaling according to Raymer.
    // => The cross-section parameters are
    return {
        dimensions_unscaled.height * std::pow(this->scale_factor_, 0.5),
        dimensions_unscaled.width * std::pow(this->scale_factor_, 0.5),
        dimensions_unscaled.length * std::pow(this->scale_factor_, 0.4),
        dimensions_unscaled.diameter * std::pow(this->scale_factor_, 0.5)
    };
}

auto Engine::get_operating_point() -> OperatingPoint
{
    if (this->N_upper_limit_ < this->op_.N) {
        op_.N = this->N_upper_limit_;
    }
    if (this->N_lower_limit_ > this->op_.N) {
        this->op_.N = this->N_lower_limit_;
    }
    return this->op_;
}

// === Setters ===
void Engine::set_operating_point(const OperatingPoint &op)
{
    this->op_ = op;
}

void Engine::set_correction_factors(const OperatingPoint &op, const atmosphere& atm)
{
    // Calculate correction values for N1, mass flow, and Temperature
    double temp = atm.getTemperature(this->op_.altitude);
    double tempISA = atm.getTemperatureISA(this->op_.altitude);
    double pressure = atm.getPressure(this->op_.altitude);
    double pressureISA = atm.getPressureISA(this->op_.altitude);
    N_correction_ = sqrt(temp / tempISA);
    W_correction_ = sqrt(tempISA / temp) * (pressure / pressureISA);
    T_correction_ = temp / tempISA;
}

void Engine::calculate_N1_with_penalties(const double& flight_level,
                                         const double& mach_number,
                                         const atmosphere& atm,
                                         const double& derate,
                                         const std::string& thrust_rating,
                                         double bleed_air_offtake,
                                         double shaft_power_offtake)
{
    // Set environmental values
    this->op_.altitude = flight_level;
    this->op_.Mach = mach_number;
    this->set_correction_factors(op_, atm);
    // Calculate operating limits (Idle N1) w.r.t. operating point
    this->set_lower_N1_limit();
    this->set_upper_N1_limit(thrust_rating);
    this->calculate_offtake_correction(bleed_air_offtake, shaft_power_offtake);
    this->set_N1_to_rating(thrust_rating, derate);
    this->check_offtake_boundaries(op_, bleed_air_offtake, shaft_power_offtake);
}

void Engine::check_offtake_boundaries(const OperatingPoint &op, double bleedAir, double shaftPower)
{
    double coreMassFlow{engine_data_.get_deck_value(core_mass_flow, this->op_) * scale_factor_ / this->W_correction_};
    double relBleed{1.06 * bleedAir / coreMassFlow};
    if (relBleed > this->get_max_bleed_offtake_at_current_operating_point()) {
        std::stringstream errMsg;
        errMsg << "Relative bleed air extraction (" << relBleed << ") too large. ";
        errMsg << "Max. allowed extraction is " << this->max_relative_bleed << "." << std::endl;
        errMsg << "Altitude: " << this->op_.altitude << " m; ";
        errMsg << "Mach: " << this->op_.Mach << "; ";
        errMsg << "N1: " << this->op_.N;
        throwError(__FILE__, __func__, __LINE__, errMsg.str());
    }
    if (shaftPower > get_max_shaft_power_offtake_engine()) {
        throwError(__FILE__, __func__, __LINE__, "Shaft power extraction too large!");
    }
}

auto Engine::converge_value(const OperatingPoint &op,
                            double goal_value,
                            double iteration_step,
                            std::string deckvalue) -> double
{
    // Initialize variables for interpolation
    const int MAX_ITERATIONS{50};   // Assuming a max iteration count of 50 as value should be reached
    double delta, x1, x2, y1, y2, y, x_new;
    bool value_should_increase{false};
    int iterationcount{0};
    double step = iteration_step;

    // Helper lambda to fetch thrust or deck value
    auto get_thrust_or_deck_value = [&]() -> double {
        return (deckvalue == "_FN") ? this->get_thrust() :
            this->engine_data_.get_deck_value(engine_data_.name() + deckvalue, this->op_);
    };
    // Only go through loop if the delta is there
    double value_at_op = get_thrust_or_deck_value();
    delta = std::abs((goal_value-value_at_op)/value_at_op);
    // Loop to get to a convergence
    while (delta > ACCURACY_MEDIUM) {
        x1 = get_thrust_or_deck_value();
        y1 = op_.N;
        value_should_increase = (x1 < goal_value);
        // Initialize variable for iteration to be + or - step
        y2 = value_should_increase ? (this->op_.N + step) : (this->op_.N - step);
        // Check that the operating point is within the limits. Operating points must allow goal value
        this->op_.N = std::clamp(y2, this->N_lower_limit_, this->N_upper_limit_);
        // get linear interpolation to be on the point where we expect the value
        x2 = get_thrust_or_deck_value();
        // Interpolate
        y = y1 + ((y2 - y1) / (x2 - x1)) * (goal_value - x1);
        this->op_.N = y;
        // check if it is close enough otherwise set the limit for the new interpolation and do it again
        this->op_.N = std::clamp(y, this->N_lower_limit_, this->N_upper_limit_);
        x_new = get_thrust_or_deck_value();
        // Calculate delta
        delta = std::abs((x_new-goal_value)/goal_value);
        // Set new boundaries
        step /= 2.0;
        // Handel iteration boundaries
        if (++iterationcount > MAX_ITERATIONS) {
            std::cout << "Cannot converge to N1" << std::endl;
            throw 1;
        }
        if (y >= this->N_upper_limit_) {
            std::cout << "Reached upper limit: " << this->N_upper_limit_ << std::endl;
            this->op_.N = this->N_upper_limit_;
            break;
        }
        if (y <= this->N_lower_limit_) {
            std::cout << "Reached lower limit: " << this->N_lower_limit_ << std::endl;
            this->op_.N = this->N_lower_limit_;
            break;
        }
    }
    return op_.N;
}

void Engine::calculate_N1_with_thrustlimit(const double& flight_level,
                                           const double& mach_number,
                                           const atmosphere& atm,
                                           const double& derate,
                                           const std::string& thrust_rating,
                                           double bleed_air_offtake,
                                           double shaft_power_offtake,
                                           double thrust_limit) {
    // Set engine rating
    this->calculate_N1_with_penalties(flight_level,
                                      mach_number,
                                      atm,
                                      derate,
                                      thrust_rating,
                                      bleed_air_offtake,
                                      shaft_power_offtake);
    this->calculate_offtake_correction(bleed_air_offtake, shaft_power_offtake);
    // Engine deck names
    double thrust_at_currentN1_tmp = get_thrust();
    while (thrust_limit > thrust_at_currentN1_tmp)
    {
        this->op_.N += N1_step_;
        if (this->op_.N > this->N_upper_limit_) {
            this->op_.N = this->N_upper_limit_;
            // If upper limit is reached it makes no sense to continue while loop here
            // Throw error so mission can deal with it. It knows what to do with these numbers
            double thrust_at_max_N1_tmp = get_thrust();
            if (thrust_limit > thrust_at_max_N1_tmp) {
                // std::cout << "Engine unable to deliver the required thrust. Max thrust:" << thrust_at_max_N1_tmp << "N" << std::endl;
                throw 2;    // Mission knows what to do with that
                }
            }
        thrust_at_currentN1_tmp = get_thrust();
    }

    while (thrust_limit < thrust_at_currentN1_tmp)
    {
        this->op_.N -= N1_step_;
        if (this->op_.N < this->N_lower_limit_) {
            this->op_.N = this->N_lower_limit_;
            // If lower limit is reached it makes no sense to continue while loop here
            // Throw error so mission can deal with it. It knows what to do with these numbers
            double thrust_at_min_N1_tmp = get_thrust();    // get the thrust at the lowest possible shaft speed [N]
            if (thrust_limit < thrust_at_min_N1_tmp) {
                throw 3;    // Mission knows what to do with that
            }
        }
        thrust_at_currentN1_tmp = get_thrust();
    }
    // Calculate corrections for N_mom due to bleed & shaft power-offtakes
    this->calculate_offtake_correction(bleed_air_offtake, shaft_power_offtake);
    this->op_.N = this->converge_value(op_, thrust_limit, N1_step_, "_FN");
}

// === Access deck values ===
auto Engine::get_thrust_aircraft() -> double
{
    // Get the deck value and scale it directly with the thrust scale factor
    return SI::force(this->engine_data_.get_deck_value(thrustdeck, this->op_), "kN") *
        this->scale_factor_ *
        this->number_of_engines_ *
        this->thrust_correction_bleed_;
}

auto Engine::get_thrust() -> double
{
    // Get the deck value and scale it directly with the thrust scale factor
    return SI::force(this->engine_data_.get_deck_value(thrustdeck, this->op_), "kN") *
        this->scale_factor_ * thrust_correction_bleed_;
}

double Engine::get_thrust_lapse(const std::string &thrust_rating, const atmosphere& atm, double mach, double altitude)
{
    // Calculate the SLST via the lib and for no penalties
    this->calculate_N1_with_penalties(0,
                                      0,
                                      atm,
                                      1.,
                                      thrust_rating,
                                      0,
                                      0);
    // Set the base thrust
    double base_thrust = get_thrust();
    // Calculate the thrust at the given condition  
    this->calculate_N1_with_penalties(altitude,
                                      mach,
                                      atm,
                                      1.,
                                      thrust_rating,
                                      0,
                                      0);
    double operating_thrust = get_thrust();
    // Output is the ratio of the thrusts
    return operating_thrust/base_thrust;
}

auto Engine::get_aircraft_fuelflow() -> double
{
    // Get the deck value and scale it directly with the thrust scale factor, since TSFC stays constant
    return this->engine_data_.get_deck_value(fuelflow, this->op_) *
      this->scale_factor_;
}

auto Engine::get_fuelflow() -> double
{
    // Get the deck value and scale it directly with the thrust scale factor, since TSFC stays constant
    return this->engine_data_.get_deck_value(fuelflow, this->op_) *
      this->scale_factor_ * 
      this->fuel_flow_factor_shaft_offtakes_;
}

auto Engine::get_tsfc() -> double
{
    // Calculates TSFC in kg/s
    double tsfc = this->engine_data_.get_deck_value(fuelflow, this->op_) /
    SI::force(this->engine_data_.get_deck_value(thrustdeck, this->op_), "kN");
    return tsfc;
}

auto Engine::get_tsfc_penalties(double delta_isa_T, double shaft_offtake, double bleed_offtake) -> double
{
    // Calculates TSFC in kg/s
    this->calculate_offtake_correction(bleed_offtake, shaft_offtake);
    double tsfc = this->engine_data_.get_deck_value(fuelflow, this->op_) /
    SI::force(this->engine_data_.get_deck_value(thrustdeck, this->op_), "kN");
    return tsfc;
}

auto Engine::get_thrust_with_lever_position(double lever_position_percent, double mach, double altitude) -> double
{
    if(lever_position_percent > 1.) {
        std::cout << "Please give the thrust lever position in fractions of 1" << std::endl;
    }
    this->op_.altitude = altitude;
    this->op_.Mach = mach;

    // Set N1 limits
    set_upper_N1_limit("takeoff");
    double max_N1 = this->op_.N;
    set_lower_N1_limit();
    double idle_N1 = this->op_.N;

    // Compute target N1 based on lever position
    double target_N1 = idle_N1 + (max_N1 - idle_N1) * lever_position_percent;

    // Update N1 to compute thrust
    this->op_.N = target_N1;

    return get_thrust();
}

auto Engine::get_engine_EGT() -> double
{
    return this->engine_data_.get_deck_value("St5_temperature", this->op_) * T_correction_ * temperature_shaft_offtake_bleed_correction_;
}

auto Engine::get_engine_EPR() -> double
{
    return this->engine_data_.get_deck_value("EPR", this->op_);
}

auto Engine::get_engine_OPR() -> double
{
    return this->engine_data_.get_deck_value("P3qP2", this->op_);
}

auto Engine::get_engine_NOx_emission_index() -> double
{
    // Gets the NOx in Kg/Kg_fuel using the GasTurb values. 
    return this->engine_data_.get_deck_value(sNOx, this->op_) * get_engine_severity_parameter() / 1000.;
}

auto Engine::get_LTO_emission_index(const LTOPhases LTO_cycle, const EngineEmissions emission) -> double
{
    switch (emission)
    {
    case EngineEmissions::HC :
        return this->engine_data_.LTO_emission_HC(LTO_cycle);
    case EngineEmissions::CO :
        return this->engine_data_.LTO_emission_CO(LTO_cycle);
    case EngineEmissions::NOx :
        return this->engine_data_.LTO_emission_NOx(LTO_cycle);
    case EngineEmissions::SN :
        return this->engine_data_.LTO_emission_SN(LTO_cycle);
    default:
        return 0.0;
    }
}

auto Engine::get_LTO_fuelflow(const LTOPhases LTO_cycle) -> double
{
    // The LTO fuel flow is now scaled with the scale factor according to the scaling of the engine fuel flow
    return this->engine_data_.LTO_fuel_flow(LTO_cycle) *
    this->scale_factor_;
}

auto Engine::get_N1_RPS() -> double
{
    // Get the N1 actual rotation value
    RPS_N1 = this->op_.N * engine_data_.N1_Nominal() / 60.0;
    return RPS_N1;
}

auto Engine::get_max_shaft_power_offtake_engine() -> double
{
    // the maximal allowed shaft power extraction scales linearly with the scale factor
    return this->max_shaft_power_extraction_ * this->scale_factor_;
}

auto Engine::get_max_bleed_offtake_at_current_operating_point() -> double
{
    // the maximal allowed bleed air offtake scales linearly with the thrust_scale_factor
    return this->max_relative_bleed * engine_data_.get_deck_value(core_mass_flow, this->op_) * this->scale_factor_;
}

auto Engine::get_dry_mass() -> double 
{
    // get the scaled dry mass of the engine according to Raymer p. 284 
    return this->dry_mass_ * pow(scale_factor_,1.1); 
}

auto Engine::get_physical_properties_stage(const StageProperties stage_property,
      const EngineStage engine_stage) -> double
      {
        // convert the input nums into two temporary strings to concatenate them later
        std::string tmp_string_stage_property{};
        std::string tmp_string_engine_stage{};

        // convert enums to strings.
        switch (stage_property)
        {
        case StageProperties::FuelToAirRatio: tmp_string_stage_property = "fa"; break;
        case StageProperties::MachNumber: tmp_string_stage_property = "Mn"; break;
        case StageProperties::TotalPressure: tmp_string_stage_property = "P"; break;
        case StageProperties::TotalTemperature: tmp_string_stage_property = "T"; break;
        case StageProperties::StaticTemperature: tmp_string_stage_property = "Ts"; break;
        case StageProperties::MassFlow: tmp_string_stage_property = "W"; break;
        default:
            std::cout << "Unknown engine stage property." << std::endl;
            return 0.0;
        }

        switch (engine_stage)
        {
        case EngineStage::St2 : tmp_string_engine_stage = "_St2_"; break;
        case EngineStage::St3 : tmp_string_engine_stage = "_St3_"; break;
        case EngineStage::St4 : tmp_string_engine_stage = "_St4_"; break;
        case EngineStage::St5 : tmp_string_engine_stage = "_St5_"; break;
        case EngineStage::St13: tmp_string_engine_stage = "_St13_"; break;
        case EngineStage::St22: tmp_string_engine_stage = "_St22_"; break;
        case EngineStage::St25: tmp_string_engine_stage = "_St25_"; break;
        case EngineStage::St45: tmp_string_engine_stage = "_St45_"; break;
        case EngineStage::St18: tmp_string_engine_stage = "_St18_"; break;
        case EngineStage::St8 : tmp_string_engine_stage = "_St8_"; break;
        default:
            std::cout << "Unknown engine stage." << std::endl;
            return 0.0;
        }

        // concatenate the tmp string tho match the .cvs name
        return engine_data_.get_deck_value(engine_data_.name() + tmp_string_engine_stage + tmp_string_stage_property,
                                           op_);
      }

// === PRIVATE FUNCTIONS ===
auto Engine::get_engine_severity_parameter() -> double
{
    return engine_data_.get_NOx_severity_parameter();
}

auto Engine::get_scaled_SLST() -> double
{
    return SI::force(this->unscaled_SLST_, "kN") * this->scale_factor_;
}

void Engine::set_lower_N1_limit() {
    // Estimation of ground/flight idle
    // FADEC sets minimum fuel flow to prevent flame-out (WF/P3)
    // Set lower N1 limit to minimum required fuel
    // (first converged N1 in fuel flow deck for current altitude and Mach number)
    this->N_lower_limit_ = op_.N = engine_data_.get_lower_operating_point(fuelflow).N;
    // N_lower_limit_ is increased from min value in deck that shows all valid operating points to first valid value
    while (engine_data_.get_deck_value(fuelflow, this->op_) < 0)
    {
        op_.N += N1_step_;
        this->N_lower_limit_ = op_.N;    // update the N_lower_limit_ variable
    }
    while (engine_data_.get_deck_value(thrustdeck, this->op_) < 0)
    {
        op_.N += N1_step_; 
        this->N_lower_limit_ = op_.N; // update the N_lower_limit_ variable
    }
    // Initialize check variables
    // Check flameout limit deck (WFqP3) if flameout danger is true
    // and ratio of fuelflow to combustion chamber inlet is below limit.
    // In this case N1 is increased
    while (this->engine_data_.get_deck_value(fraction_fuelflow_to_p3, this->op_) < this->WF_to_P3_min_) {
        op_.N += N1_step_;
        this->N_lower_limit_ = op_.N;    // update N_lower_limit_ variable
        if (this->N_lower_limit_ > this->N_upper_limit_) {
            throwError(__FILE__, __func__, __LINE__, "Idle in WFToP3-limit deck not found.");
        }
    }
}

void Engine::set_upper_N1_limit(const std::string &thrust_rating) {
    // Estimation of upper N1 value
    // FADEC sets maximum fuel flow to prevent over temperatures as a limit of temperatures, pressures, HP spool speed
    // Set upper N1 limit to maximum required fuel
    this->N_upper_limit_ = op_.N = engine_data_.get_upper_operating_point(fuelflow).N;    // Reset upper N1 limit
    // N_upper_limit_ is decreased from max value in valid operating point deck to first valid value
    while (engine_data_.get_deck_value(fuelflow, this->op_) < 0)
    {
        this->op_.N -= N1_step_;
        this->N_upper_limit_ = this->op_.N;    // update the N_upper_limit_ variable
    }
    // Temperature limit
    this->set_upper_N1_limit_temperature(thrust_rating);
    // FuelFlow/Pressure limit
    this->set_upper_N1_limit_with_overall_fuelflow_pressure_ratio(thrust_rating);
    // HP Spool speed limit
    this->set_upper_N1_limit_high_pressure_spool_speed(thrust_rating);
    if (this->N_upper_limit_ < op_.N) {
        op_.N = this->N_upper_limit_;
    }
}

void Engine::calculate_offtake_correction(const double& bleedAir, const double& shaftPower) {
    // The factors for correction were done according to the CF6 installation manual.
    // Range of values: Mach: 0-0.95; Relative corrected thirst: 0.2-1
    // create temporary operating point to feed into the function
    this->check_offtake_boundaries(op_, bleedAir, shaftPower);
    double coreMassFlow{engine_data_.get_deck_value(core_mass_flow, this->op_) * scale_factor_ / this->W_correction_}; 
    thrust_correction_bleed_ = 1. - (2. * ( bleedAir / coreMassFlow)); // Ray p. 488
    // Offtake corrections
    this->shaft_offtake_factor_ = (-3.5e-7 * this->op_.altitude + 6.75e-3) * (this->op_.Mach * this->op_.Mach)
                            + (4.7e-7 * this->op_.altitude - 1.208e-2) * this->op_.Mach
                            + 1e-8 * this->op_.altitude
                            + 5.85e-3;
    this->fuel_flow_factor_shaft_offtakes_ = (1. + shaftPower * shaft_offtake_factor_/(this->get_thrust()));
}

void Engine::set_N1_to_rating(const std::string& thrustRating, const double& derate) {
    if (thrustRating == "takeoff" || thrustRating == "maximum_continuous") {
        this->op_.N = std::max(this->N_upper_limit_, this->N_lower_limit_);
    } 
    else if (thrustRating == "climb") {
        this->op_.N = std::max(this->N_upper_limit_ * this->N_fraction_climb_rating_,
                               this->N_lower_limit_);
    } 
    else if (thrustRating == "cruise") {
        this->op_.N = std::max(this->N_upper_limit_ * this->N_fraction_cruise_rating_,
                               this->N_lower_limit_);
    } 
    else if (thrustRating == "idle") {
        this->op_.N = this->N_lower_limit_;
    } 
    else {
        std::stringstream errorMessage{};
        errorMessage << "Rating: " << thrustRating << " not known! Abort program!" << std::endl;
        throwError(__FILE__, __func__, __LINE__, errorMessage.str());
    }
}

void Engine::set_upper_N1_limit_temperature(const std::string &thrust_rating) {
    // Temperature limit for corrected temperature! flat-rated
    double TempLimit = (thrust_rating == "takeoff")
                   ? this->max_temperature_TO_
                   : this->max_temperature_continuous_thrust_;
    this->op_.N = this->N_upper_limit_;
    // N_upper_limit_ is decreased from max value in fuel deck to first valid value
    while (engine_data_.get_deck_value(temperature_limit_deck, this->op_) *
           this->T_correction_ *
           this->temperature_shaft_offtake_bleed_correction_ > TempLimit) {
        this->op_.N -= N1_step_;
        this->N_upper_limit_ = this->op_.N;    // update the N_upper_limit_ variable
        if (this->N_upper_limit_ < N_lower_limit_) {
            std::stringstream errorMessage{};
            errorMessage << "Engine N1 speed below idle speed by temp-limit! ";
            errorMessage << "Impossible operating point." << std::endl;
            throwError(__FILE__, __func__, __LINE__, errorMessage.str());
        }
    }
}

void Engine::set_upper_N1_limit_with_overall_fuelflow_pressure_ratio(const std::string &thrust_rating) {
    // FuelFlow/Pressure ratio limit
    double maximumPressureRatio = (thrust_rating == "takeoff")
                              ? this->WF_to_P3_max_TO_
                              : this->WF_to_P3_max_MCT_;
    this->op_.N = this->N_upper_limit_;
    // N_upper_limit_ is decreased from max value in fuel deck to first valid value
    while (engine_data_.get_deck_value(fraction_fuelflow_to_p3, op_) > maximumPressureRatio) {
        op_.N -= N1_step_;
        this->N_upper_limit_ = this->op_.N;    // update N_upper_limit_ variable
        if (this->N_upper_limit_ < this->N_lower_limit_) {
            std::stringstream errorMessage{};
            errorMessage << "Engine N1 speed below idle speed by compression-limit! ";
            errorMessage << "Impossible operating point." << std::endl;
            throwError(__FILE__, __func__, __LINE__, errorMessage.str());
        }
    }
}

void Engine::set_upper_N1_limit_high_pressure_spool_speed(const std::string &thrust_rating) {
    this->op_.N = this->N_upper_limit_;
    // N_upper_limit_ is decreased from max value in fuel deck to first valid value
    while (engine_data_.get_deck_value(N_limit_deck, this->op_) > this->N2_max_) {
        this->op_.N -= N1_step_;
        this->N_upper_limit_ = this->op_.N;    // Update N_upper_limit_ variable
        if (this->N_lower_limit_ > this->N_upper_limit_) {
            std::stringstream errorMessage{};
            errorMessage << "NupperLimit is below NlowerLimit. ";
            errorMessage << "No valid operation point." << std::endl;
            throwError(__FILE__, __func__, __LINE__, errorMessage.str());
        }
    }
}
