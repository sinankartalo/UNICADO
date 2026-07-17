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

#ifndef ATMOSPHERE_ATMOSPHERE_H_
#define ATMOSPHERE_ATMOSPHERE_H_

#include <unitConversion/constants.h>
#include <cmath>

#ifdef BUILD_ATMOSPHERE_SHARED
    #ifdef _WIN32
        #define ATMOSPHEREDLLEXPORT __declspec(dllexport)
    #else
        #define ATMOSPHEREDLLEXPORT __attribute__ ((visibility ("default")))
    #endif
#elif defined(IMPORT_ATMOSPHERE_SHARED)
    #ifdef _WIN32
        #define ATMOSPHEREDLLEXPORT __declspec(dllimport)
    #else
        #define ATMOSPHEREDLLEXPORT
    #endif
#else
    #define ATMOSPHEREDLLEXPORT
#endif

/** \brief Class for the description of the atmosphere
 */
class ATMOSPHEREDLLEXPORT atmosphere {
 public:
    atmosphere(); /**< Constructor */
    virtual ~atmosphere(); /**< Destructor */

    /* Member functions */
    /** \brief function sets values for temperature, pressure and density at sea level as well as the altitude of tropopause
    *   \details the values at sea level are adapted to the given reference values (pressure level is kept constant to ISA Standard Pressure)
    *   \param h_ref reference altitude [m]
    *   \param T_ref reference temperature at reference altitude [K]
    *   \param p_ref reference pressure at reference altitude [N/m^2]
    */
    void setAtmosphere(const double& h_ref, const double& T_ref, const double& p_ref);

    inline double getTemperature(const double& h) const;
    inline double getTemperatureISA(const double& h) const;
    inline double getDensity(const double& h) const;
    inline double getPressure(const double& h) const;
    inline double getPressureISA(const double& h) const;
    inline double getSpeedOfSound(const double& h) const;
    inline double getSpeedOfSoundISA(const double& h) const;
    /** \brief function returns the dynamic viscosity
    *   \param h altitude in meter
    *   \return dynamicViscosity [kg/(m*s)]
    */
    inline double getViscosity(const double& h) const;
    inline double temperatureRatio(const double& h) const;
    inline double densityRatio(const double& h) const;
    inline double pressureRatio(const double& h) const;

    /** \brief function calculates the density height
    *   \details function returns current altitude in accordance to a given density
    *   \param density current density [kg/m^3]
    *   \return double: altitude where the current density is given [m]
    */
    double getAltitudeAtDensity(const double& density) const;
    /** \brief function calculates the pressure height
    *   \details function returns current altitude in accordance to a given pressure ratio
    *   \param pressureRatio current pressure ratio [-]
    *   \return double: altitude where the current pressure ratio is given [m]
    */
    double getAltitudeAtPressureRatio(const double& pressureRatio) const;


    /* Calculated values */
    double h_trop; /**< Altitude of tropopause [m] */
    double delta_h; /**< Correction of altitude for local altimeter settings (in Germany below 5000 ft, else ISA Standard and FL) [m] */
    double rho_0; /**< Density at sea level [kg/m^3] */
    double T_0; /**< Temperature at sea level [K] */
    double p_0; /**< Pressure at sea level [N/m^2] */

 private:
    /* Constants */
    static const double h_tropopause_ref; /**< Reference height of tropopause (11 km) [m] */
    static const double T_tropopause_ref; /**< Temperature of tropopause in 11 km [K] */
    static const double TemperatureGradient; /**< Temperature gradient with altitude (valid until tropopause) */

    /**
        h1(km)      h2(km)     dT/dh (K/km)  Source: http://www.pdas.com/coesa.html
            0          11         -6.5
            11         20          0.0
            20         32          1.0
            32         47          2.8
            47         51          0.0
            51         71         -2.8
            71         84.852     -2.0
    **/
};

/* Implementation of inline functions */
inline double atmosphere::getTemperature(const double& h) const {
    return (h < this->h_trop) ? this->T_0+this->TemperatureGradient*h : this->T_tropopause_ref;
}

inline double atmosphere::getTemperatureISA(const double& h) const {
    return (h < this->h_trop) ? ISA_TEMPERATURE+this->TemperatureGradient*h : this->T_tropopause_ref;
}

inline double atmosphere::getDensity(const double& h) const {
    return (h < this->h_trop) ?
           this->rho_0*pow(1+(this->TemperatureGradient*h/this->T_0), (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient-1)) :
           this->rho_0*pow(this->T_tropopause_ref/this->T_0, (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient-1))*exp(
               -G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->T_tropopause_ref*(h-this->h_trop));
}

inline double atmosphere::getPressure(const double& h) const {
    return (h < this->h_trop) ?
           this->p_0*pow(1+(this->TemperatureGradient*h/this->T_0), (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient)) :
           this->p_0*pow(this->T_tropopause_ref/this->T_0, (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient))*exp(
               -G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->T_tropopause_ref*(h-this->h_trop));
}

inline double atmosphere::getPressureISA(const double& h) const {
    return (h < this->h_trop) ?
           ISA_PRESSURE*pow(1+(this->TemperatureGradient*h/ISA_TEMPERATURE), (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient)) :
           ISA_PRESSURE*pow(this->T_tropopause_ref/ISA_TEMPERATURE,
                            (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient))*exp(-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->T_tropopause_ref*(h-this->h_trop));
}

inline double atmosphere::getSpeedOfSound(const double& h) const {
    return (h < this->h_trop) ?
           sqrt((this->T_0+(TemperatureGradient*h))*ISENTROPIC_EXPONENT_DRY_AIR*SPECIFIC_GAS_CONSTANT_DRY_AIR) :
           sqrt(this->T_tropopause_ref*ISENTROPIC_EXPONENT_DRY_AIR*SPECIFIC_GAS_CONSTANT_DRY_AIR);
}

inline double atmosphere::getSpeedOfSoundISA(const double& h) const {
    return (h < 11000.) ?
           sqrt((ISA_TEMPERATURE+(TemperatureGradient*h))*ISENTROPIC_EXPONENT_DRY_AIR*SPECIFIC_GAS_CONSTANT_DRY_AIR) :
           sqrt(this->T_tropopause_ref*ISENTROPIC_EXPONENT_DRY_AIR*SPECIFIC_GAS_CONSTANT_DRY_AIR);
}

inline double atmosphere::getViscosity(const double& h) const {
    double beta(DYNAMIC_VISCOSITY_SEALEVEL*(STANDARD_TEMPERATURE+SUTHERLAND_CONSTANT_DRY_AIR)/pow(STANDARD_TEMPERATURE, 1.5));
    double T(this->T_0+this->TemperatureGradient*h);

    return (h < this->h_trop) ?
           beta * pow(T, 1.5)/(T+SUTHERLAND_CONSTANT_DRY_AIR) :
           beta * pow(T_tropopause_ref, 1.5)/(T_tropopause_ref+SUTHERLAND_CONSTANT_DRY_AIR);
}

inline double atmosphere::temperatureRatio(const double& h) const {
    return (h < this->h_trop) ? 1+(TemperatureGradient*h/this->T_0) : this->T_tropopause_ref/this->T_0;
}

inline double atmosphere::densityRatio(const double& h) const {
    return (h < this->h_trop) ?
           pow(1+(this->TemperatureGradient*h/this->T_0), (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient-1)) :
           pow(this->T_tropopause_ref/this->T_0, (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient-1)) *
            exp(-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->T_tropopause_ref * (h-this->h_trop));
}

inline double atmosphere::pressureRatio(const double& h) const {
    return (h < this->h_trop) ?
           pow(1+(this->TemperatureGradient*h/this->T_0), (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient)) :
           pow(this->T_tropopause_ref/this->T_0, (-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->TemperatureGradient)) *
           exp(-G_FORCE/SPECIFIC_GAS_CONSTANT_DRY_AIR/this->T_tropopause_ref * (h-this->h_trop));
}

#endif // ATMOSPHERE_ATMOSPHERE_H_
