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

#ifndef UNITCONVERSION_CONSTANTS_H_
#define UNITCONVERSION_CONSTANTS_H_

#include <cmath>

//Astronomical_Unit in Meter
//Mean Sun-Earth distance
#define AU 149597870700.

//AMU Chemical in Kilogram
//1/16 of the weighted average mass of the 3 naturally occurring neutral isotopes of oxygen.
#define AMU_CHEMICAL 1.66026000E-27

//AMU Physical in Kilogram
//1/16 of the mass of a neutral oxygen 16 atom.
#define AMU_PHYSICAL 1.65981000E-27

//Atomic Mass Unit in Kilogram
//defined to be 1/12 of the mass of carbon 12
#define ATOMIC_MASS_UNIT 1.6605402E-27

//Avogadro Constant in Particles/mol
//Number of particles in 1 mole of substance
#define AVOGADRO_CONSTANT 6.02214129E+23

//Bohr Radius in Meter
//the most probable distance between the proton and electron in a hydrogen atom in its ground state.
#define BOHR_RADIUS 5.2917721092E-11

//Boltzmann Constant k in Joule/Kelvin
//physical constant relating energy at the individual particle level with temperature
#define BOLTZMANN_CONSTANT 1.3806488E-23

//Coulomb's Constant(electric force constant) in Newton*(Meter^2)/(Coulomb^2)
//proportionality constant in equations relating electric variables
#define COULOMBS_CONSTANT 8.9875517873681764E+9

//Electron Rest Mass me in Kilogram
//the mass of a stationary electron
#define ELECTRON_REST_MASS 9.10938215E-31

//Proton Mass mp in Kilogram
//Particle mass
#define PROTON_MASS 1.672621777E-27

//Neutron  Mass mn in Kilogram
#define NEUTRON_MASS 1.674927351E-27

//Electron Electric Charge in Coulomb
//Elementary charge
#define ELECTRON_ELECTRIC_CHARGE 1.602176565E-19

//Faraday Constant F in Coulomb/mol
//the magnitude of electric charge per mole of electrons
#define FARADAY_CONSTANT 9.64853399E+4

//Fine Structure Constant
//mu0 * c * (e^2) / h
#define FINE_STRUCTURE_CONSTANT 0.00729735281

//G Force Acceleration of gravity, g in Meter / (Second^2)
//Earth Gravity
#define G_FORCE 9.80665

//Gravitational Constant in Newton * (Meter^2) / (Kilogram^2)
//an empirical physical constant involved in the calculation(s) of gravitational force between two bodies
#define GRAVITATIONAL_CONSTANT 6.674E-11

//Gas Constant R in Joule / (Kelvin * mol)
//physical constant which is featured in many fundamental equations in the physical sciences
#define GAS_CONSTANT 8.3144621

//Isentropic exponent for dry air at 20 °C (kappa)
//ratio of the heat capacity at constant pressure to heat capacity at constant volume
#define ISENTROPIC_EXPONENT_DRY_AIR 1.4

// Specific heat capacity water in J/(kg*K)
// Amount of heat that must be added to one unit of mass of a substance in order to cause an increase of one unit in temperature.
#define HEAT_CAPACITY_WATER 4184.

//Specific Gas Constant R in J / (Kelvin * Kilogram)
//The specific gas constant of a gas is given by the molar gas constant divided by the molar mass of the gas/mixture.
#define SPECIFIC_GAS_CONSTANT_DRY_AIR 287.058

//PI
//mathematical constant, the ratio of a circle's circumference to its diameter
#define PI 3.14159265358979323846

//Planck Constant h in Joule * Second
//physical constant that is the quantum of action in quantum mechanics
#define PLANCK_CONSTANT 6.62606957E-34

//Speed of Light in Meter/Second
//universal physical constant important in many areas of physics
#define SPEED_OF_LIGHT 299792458.

//Standard Temperature in Kelvin
//IUPAC established standard temperature as a temperature of 273.15 K
#define STANDARD_TEMPERATURE 273.15

//Temperature ISA: Temperature after ISA in Kelvin (ISO 2533)
//ICAO established standard temperature as a temperature of 288.15 K
#define ISA_TEMPERATURE 288.15

//Density ISA: Density after ISA in kg/(m^3) (ISO 2533)
//ICAO established standard density in level NN of 1.225 kg/(m^3)
#define ISA_DENSITY 1.225

//Pressure ISA: Pressure after ISA in N/(m^2) (ISO 2533)
//ICAO established standard pressure in level NN of 101325 N/(m^2)
#define ISA_PRESSURE 101325.

//Sutherland Constant C in Kelvin for dry air
//Sutherland constant vary with gas
#define SUTHERLAND_CONSTANT_DRY_AIR 110.4

//Viscosity ISA: Dynamic Viscosity after ISA in Pa*s
#define DYNAMIC_VISCOSITY_SEALEVEL 1.716E-5

//Accuracy constants
//Accuracy constants used for comparison of doubles (==, !=)
#define ACCURACY_LOW 1E-4
#define ACCURACY_MEDIUM 1E-7
#define ACCURACY_HIGH 1E-10

// Transition altitude ATC limit FL100 in [m]
#define TRANSITION_ALTITUDE 3048.

#endif // UNITCONVERSION_CONSTANTS_H_
