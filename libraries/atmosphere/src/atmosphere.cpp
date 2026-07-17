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

#include "atmosphere/atmosphere.h"

const double atmosphere::TemperatureGradient(-0.0065);//[K/m]
const double atmosphere::h_tropopause_ref(11000.);//[m]
const double atmosphere::T_tropopause_ref(ISA_TEMPERATURE+atmosphere::TemperatureGradient * atmosphere::h_tropopause_ref);//[K]

atmosphere::atmosphere() {
    T_0 = ISA_TEMPERATURE;
    p_0 = ISA_PRESSURE;
    rho_0 = ISA_DENSITY;
    h_trop = this->h_tropopause_ref;
    delta_h = 0.;
}

atmosphere::~atmosphere() {
}

void atmosphere::setAtmosphere(const double& h_ref, const double& T_ref, const double& p_ref) { // cppcheck-suppress unusedFunction
    /* Calculate values for temperature and density at sea level and altitude of tropopause */
    this->T_0 = T_ref - this->TemperatureGradient*h_ref;
    this->p_0 = ISA_PRESSURE; //Pressure profile is always referenced to ISA STANDARD PRESSURE
    this->rho_0 = ISA_DENSITY*ISA_TEMPERATURE/this->T_0;
    this->h_trop = this->h_tropopause_ref-(this->T_0-ISA_TEMPERATURE)/this->TemperatureGradient;
    this->delta_h = (-this->getTemperatureISA(h_ref)/this->TemperatureGradient)*(1-pow(p_ref/this->getPressure(h_ref),
                    -(SPECIFIC_GAS_CONSTANT_DRY_AIR*this->TemperatureGradient/G_FORCE)));//
}

double atmosphere::getAltitudeAtDensity(const double& density) const { // cppcheck-suppress unusedFunction
    if ( density > this->getDensity(this->h_trop) ) { //Below tropopause
        return this->T_0/this->TemperatureGradient*(pow((density/this->rho_0), 1/(-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient-1))-1);
    } else {
        return log(density/(this->rho_0*pow(this->T_tropopause_ref/this->T_0,
                                            (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient-1)))) /
               (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->T_tropopause_ref)+this->h_trop;
    }
}

double atmosphere::getAltitudeAtPressureRatio(const double& pressureRatio) const { // cppcheck-suppress unusedFunction
    if ( pressureRatio > this->pressureRatio(this->h_trop) ) { //Below tropopause
        return this->T_0/this->TemperatureGradient*(pow(pressureRatio, 1/(-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient))-1);
    } else {
        return log(pressureRatio/pow(this->T_tropopause_ref/this->T_0,
                                     (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient))) /
               (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->T_tropopause_ref)+this->h_trop;
    }
}
