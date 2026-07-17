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

#ifndef ENERGYCARRIERS_INCLUDE_ENERGYCARRIERS_H_
#define ENERGYCARRIERS_INCLUDE_ENERGYCARRIERS_H_

#include <string>
/**
 * @class EnergyCarrier
 * @brief Class to access energy carrier specific parameter.
 * @details The aircraft xml specifies the energy carrier type and density.
 * Additional parameter such as the gravimetric energy density and emission indices
 * are constant for each energy carrier type. The volumetric energy density is
 * scaled acc to the density.
 * 
 * NOTE: To construct the object, define energy carrier type and density by using the
 * getter in the moduleBasics/runtimeIO.h ! 
 */
class EnergyCarrier {
 public:
    const std::string type;                   /* Energy carrier type (e.g., "kerosene") */
    const double density;                     /* Density [kg/m^3] */
    const double gravimetric_energy_density;  /* Gravimetric energy density [J/kg] */
    const double volumetric_energy_density;   /* Volumetric energy density [J/m^3] */
    const double emission_index_CO2;          /* Emission index of CO2 [kg/kg] */
    const double emission_index_H2O;          /* Emission index of H2O [kg/kg] */
    const double emission_index_SO2;          /* Emission index of SO2 [kg/kg] */
    const double emission_index_SO4;          /* Emission index of SO4 [kg/kg] */
    const double emission_index_soot;         /* Emission index of soot [kg/kg] */

    /* Constructor with type and density */
    EnergyCarrier(const std::string& type, double density);
};

#endif  // ENERGYCARRIERS_INCLUDE_ENERGYCARRIERS_H_
