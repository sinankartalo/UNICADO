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

#include <pybind11/pybind11.h>

#include "unitConversion/constants.h"

class Constants {
 public:
  static constexpr double au = AU;                                                       // Example constant: Astronomical Unit in kilometers
  static constexpr double amu_chemical = AMU_CHEMICAL;                                   // AMU Chemical in Kilogram
  static constexpr double amu_physical = AMU_PHYSICAL;                                   // AMU Physical in Kilogram
  static constexpr double atomic_mass_unit = ATOMIC_MASS_UNIT;                           // Atomic Mass Unit in Kilogram
  static constexpr double avogadro_constant = AVOGADRO_CONSTANT;                         // Avogadro Constant in Particles/mol
  static constexpr double bohr_radius = BOHR_RADIUS;                                     // Bohr Radius in Meter
  static constexpr double boltzmann_constant = BOLTZMANN_CONSTANT;                       // Boltzmann Constant in Joule/Kelvin
  static constexpr double coulombs_constant = COULOMBS_CONSTANT;                         // Coulomb's Constant in Newton*(Meter^2)/(Coulomb^2)
  static constexpr double electron_rest_mass = ELECTRON_REST_MASS;                       // Electron Rest Mass in Kilogram
  static constexpr double proton_mass = PROTON_MASS;                                     // Proton Mass in Kilogram
  static constexpr double neutron_mass = NEUTRON_MASS;                                   // Neutron Mass in Kilogram
  static constexpr double electron_electric_charge = ELECTRON_ELECTRIC_CHARGE;           // Electron Electric Charge in Coulomb
  static constexpr double faraday_constant = FARADAY_CONSTANT;                           // Faraday Constant in Coulomb/mol
  static constexpr double fine_structure_constant = FINE_STRUCTURE_CONSTANT;             // Fine Structure Constant
  static constexpr double g_force = G_FORCE;                                             // G Force in Meter/Second^2
  static constexpr double gravitational_constant = GRAVITATIONAL_CONSTANT;               // Gravitational Constant in Newton * (Meter^2) / (Kilogram^2)
  static constexpr double gas_constant = GAS_CONSTANT;                                   // Gas Constant in Joule / (Kelvin * mol)
  static constexpr double heat_capacity_water = HEAT_CAPACITY_WATER;                     // Specific heat capacity for water in J/(kg·K)
  static constexpr double isentropic_exponent_dry_air = ISENTROPIC_EXPONENT_DRY_AIR;     // Isentropic exponent for dry air at 20 °C
  static constexpr double specific_gas_constant_dry_air = SPECIFIC_GAS_CONSTANT_DRY_AIR; // Specific gas constant for dry air in J/(kg·K)
  static constexpr double pi = PI;                                                       // Pi
  static constexpr double planck_constant = PLANCK_CONSTANT;                             // Planck Constant in Joule * Second
  static constexpr double speed_of_light = SPEED_OF_LIGHT;                               // Speed of Light in Meter/Second
  static constexpr double standard_temperature = STANDARD_TEMPERATURE;                   // Standard Temperature in Kelvin
  static constexpr double isa_temperature = ISA_TEMPERATURE;                             // ISA Temperature in Kelvin (ISO 2533)
  static constexpr double isa_density = ISA_DENSITY;                                     // ISA Density in kg/m^3 (ISO 2533)
  static constexpr double isa_pressure = ISA_PRESSURE;                                   // ISA Pressure in N/m^2 (ISO 2533)
  static constexpr double sutherland_constant_dry_air = SUTHERLAND_CONSTANT_DRY_AIR;     // Sutherland Constant for dry air in Kelvin
  static constexpr double dynamic_viscosity_sealevel = DYNAMIC_VISCOSITY_SEALEVEL;       // Dynamic Viscosity at sea level in Pa·s
  static constexpr double accuracy_low = ACCURACY_LOW;                                   // Accuracy constant - low
  static constexpr double accuracy_medium = ACCURACY_MEDIUM;                             // Accuracy constant - medium
  static constexpr double accuracy_high = ACCURACY_HIGH;                                 // Accuracy constant - high
  static constexpr double transition_altitude = TRANSITION_ALTITUDE;                     // Transition altitude ATC limit FL100 in meters
};

namespace py = pybind11;

/* === Python bindings === */
PYBIND11_MODULE(py11unitConversion, m) {
  m.doc() = "Python bindings for unitConversion.";
  py::class_<Constants>(m, "constants")
      .def_property_readonly_static(
          "AU", [](py::object /* self */) { return Constants::au; }, "Astronomical Unit in kilometers")
      .def_property_readonly_static(
          "AMU_CHEMICAL", [](py::object /* self */) { return Constants::amu_chemical; }, "AMU Chemical in Kilogram")
      .def_property_readonly_static(
          "AMU_PHYSICAL", [](py::object /* self */) { return Constants::amu_physical; }, "AMU Physical in Kilogram")
      .def_property_readonly_static(
          "ATOMIC_MASS_UNIT", [](py::object /* self */) { return Constants::atomic_mass_unit; },
          "Atomic Mass Unit in Kilogram")
      .def_property_readonly_static(
          "AVOGADRO_CONSTANT", [](py::object /* self */) { return Constants::avogadro_constant; },
          "Avogadro Constant in Particles/mol")
      .def_property_readonly_static(
          "BOHR_RADIUS", [](py::object /* self */) { return Constants::bohr_radius; }, "Bohr Radius in Meter")
      .def_property_readonly_static(
          "BOLTZMANN_CONSTANT", [](py::object /* self */) { return Constants::boltzmann_constant; },
          "Boltzmann Constant in Joule/Kelvin")
      .def_property_readonly_static(
          "COULOMBS_CONSTANT", [](py::object /* self */) { return Constants::coulombs_constant; },
          "Coulomb's Constant in Newton*(Meter^2)/(Coulomb^2)")
      .def_property_readonly_static(
          "ELECTRON_REST_MASS", [](py::object /* self */) { return Constants::electron_rest_mass; },
          "Electron Rest Mass in Kilogram")
      .def_property_readonly_static(
          "PROTON_MASS", [](py::object /* self */) { return Constants::proton_mass; }, "Proton Mass in Kilogram")
      .def_property_readonly_static(
          "NEUTRON_MASS", [](py::object /* self */) { return Constants::neutron_mass; }, "Neutron Mass in Kilogram")
      .def_property_readonly_static(
          "ELECTRON_ELECTRIC_CHARGE", [](py::object /* self */) { return Constants::electron_electric_charge; },
          "Electron Electric Charge in Coulomb")
      .def_property_readonly_static(
          "FARADAY_CONSTANT", [](py::object /* self */) { return Constants::faraday_constant; },
          "Faraday Constant in Coulomb/mol")
      .def_property_readonly_static(
          "FINE_STRUCTURE_CONSTANT", [](py::object /* self */) { return Constants::fine_structure_constant; },
          "Fine Structure Constant")
      .def_property_readonly_static(
          "G_FORCE", [](py::object /* self */) { return Constants::g_force; }, "G Force in Meter/Second^2")
      .def_property_readonly_static(
          "GRAVITATIONAL_CONSTANT", [](py::object /* self */) { return Constants::gravitational_constant; },
          "Gravitational Constant in Newton * (Meter^2) / (Kilogram^2)")
      .def_property_readonly_static(
          "GAS_CONSTANT", [](py::object /* self */) { return Constants::gas_constant; },
          "Gas Constant in Joule / (Kelvin * mol)")
      .def_property_readonly_static(
          "HEAT_CAPACITY_WATER", [](py::object /* self */) { return Constants::heat_capacity_water; },
          "Specific heat capacity for water in J/(kg·K)")
      .def_property_readonly_static(
          "ISENTROPIC_EXPONENT_DRY_AIR", [](py::object /* self */) { return Constants::isentropic_exponent_dry_air; },
          "Isentropic exponent for dry air at 20 °C")
      .def_property_readonly_static(
          "SPECIFIC_GAS_CONSTANT_DRY_AIR",
          [](py::object /* self */) { return Constants::specific_gas_constant_dry_air; },
          "Specific gas constant for dry air in J/(kg·K)")
      .def_property_readonly_static(
          "PI", [](py::object /* self */) { return Constants::pi; }, "Pi")
      .def_property_readonly_static(
          "PLANCK_CONSTANT", [](py::object /* self */) { return Constants::planck_constant; },
          "Planck Constant in Joule * Second")
      .def_property_readonly_static(
          "SPEED_OF_LIGHT", [](py::object /* self */) { return Constants::speed_of_light; },
          "Speed of Light in Meter/Second")
      .def_property_readonly_static(
          "STANDARD_TEMPERATURE", [](py::object /* self */) { return Constants::standard_temperature; },
          "Standard Temperature in Kelvin")
      .def_property_readonly_static(
          "ISA_TEMPERATURE", [](py::object /* self */) { return Constants::isa_temperature; },
          "ISA Temperature in Kelvin (ISO 2533)")
      .def_property_readonly_static(
          "ISA_DENSITY", [](py::object /* self */) { return Constants::isa_density; },
          "ISA Density in kg/m^3 (ISO 2533)")
      .def_property_readonly_static(
          "ISA_PRESSURE", [](py::object /* self */) { return Constants::isa_pressure; },
          "ISA Pressure in N/m^2 (ISO 2533)")
      .def_property_readonly_static(
          "SUTHERLAND_CONSTANT_DRY_AIR", [](py::object /* self */) { return Constants::sutherland_constant_dry_air; },
          "Sutherland Constant for dry air in Kelvin")
      .def_property_readonly_static(
          "DYNAMIC_VISCOSITY_SEALEVEL", [](py::object /* self */) { return Constants::dynamic_viscosity_sealevel; },
          "Dynamic Viscosity at sea level in Pa·s")
      .def_property_readonly_static(
          "ACCURACY_LOW", [](py::object /* self */) { return Constants::accuracy_low; }, "Accuracy constant - low")
      .def_property_readonly_static(
          "ACCURACY_MEDIUM", [](py::object /* self */) { return Constants::accuracy_medium; },
          "Accuracy constant - medium")
      .def_property_readonly_static(
          "ACCURACY_HIGH", [](py::object /* self */) { return Constants::accuracy_high; }, "Accuracy constant - high")
      .def_property_readonly_static(
          "TRANSITION_ALTITUDE", [](py::object /* self */) { return Constants::transition_altitude; },
          "Transition altitude ATC limit FL100 in meters");
}
