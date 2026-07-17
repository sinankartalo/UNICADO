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

#include "energyCarriers/energyCarriers.h"
#include <stdexcept>
#include <map>

/* 
 * @brief Maps energy carrier types (std::string) to gravimetric energy densities (double [J/kg]).
 */
const std::map<std::string, double> gravimetric_energy_densities = {
    {"kerosene", 43100000.},
    {"liquid_hydrogen", 120000000.}
};

/* 
 * @brief Map energy carrier types (std::string) to CO2 emission indices (double [kg/kg]).
 */
const std::map<std::string, double> emission_indices_CO2 = {
    {"kerosene", 3.149},
    {"liquid_hydrogen", 0.0}
};

/*
 * @brief Map energy carrier types (std::string) to H2O emission indices (double [kg/kg]).
 */
const std::map<std::string, double> emission_indices_H2O = {
    {"kerosene", 1.2},
    {"liquid_hydrogen", 8.94}
};

/*
 * @brief Map energy carrier types (std::string) to SO2 emission indices (double [kg/kg]).
 */
const std::map<std::string, double> emission_indices_SO2 = {
    {"kerosene", 0.84e-3},
    {"liquid_hydrogen", 0.0}
};

/*
 * @brief Map energy carrier types (std::string) to SO4 emission indices (double [kg/kg]).
 */
const std::map<std::string, double> emission_indices_SO4 = {
    {"kerosene", 2e-4},
    {"liquid_hydrogen", 0.0}
};

/*
 * @brief Map energy carrier types (std::string) to soot emission indices (double [kg/kg]).
 */
const std::map<std::string, double> emission_indices_soot = {
    {"kerosene", 0.025e-3},
    {"liquid_hydrogen", 0.0}
};

/*
 * @brief Constructor for EnergyCarrier class.
 * @param type Energy carrier type (e.g., "kerosene").
 * @param density Density [kg/m^3].
 */
EnergyCarrier::EnergyCarrier(const std::string& type, double density)
    : type(type),
    density(density),
    gravimetric_energy_density(gravimetric_energy_densities.at(type)),
    volumetric_energy_density(density * gravimetric_energy_densities.at(type)),
    emission_index_CO2(emission_indices_CO2.at(type)),
    emission_index_H2O(emission_indices_H2O.at(type)),
    emission_index_SO2(emission_indices_SO2.at(type)),
    emission_index_SO4(emission_indices_SO4.at(type)),
    emission_index_soot(emission_indices_soot.at(type)) {
}
