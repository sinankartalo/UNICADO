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
#include <pybind11/stl.h>

#include "energyCarriers/energyCarriers.h"

namespace py = pybind11;

/* === Python bindings === */
/**
 * @brief Python bindings for the EnergyCarrier C++ library.
 */
PYBIND11_MODULE(py11energyCarriers, m) {
    // Set module docstring
    m.doc() = "Python bindings for the EnergyCarrier C++ library";

    // Bind the EnergyCarrier class
    py::class_<EnergyCarrier>(m, "EnergyCarrier")
        .def(py::init<const std::string&, double>(),
             py::arg("type"), py::arg("density"),
             "Constructor for EnergyCarrier class. Requires fuel type and density.")

        // Bind public attributes
        .def_readonly("type", &EnergyCarrier::type, "The energy carrier type (e.g., 'kerosene').")
        .def_readonly("density", &EnergyCarrier::density, "The density of the energy carrier in kg/m^3.")
        .def_readonly("gravimetric_energy_density", &EnergyCarrier::gravimetric_energy_density,
                      "The gravimetric energy density of the energy carrier in J/kg.")
        .def_readonly("volumetric_energy_density", &EnergyCarrier::volumetric_energy_density,
                      "The volumetric energy density of the energy carrier in J/m^3.")
        .def_readonly("emission_index_CO2", &EnergyCarrier::emission_index_CO2,
                      "The CO2 emission index of the energy carrier in kg/kg.")
        .def_readonly("emission_index_H2O", &EnergyCarrier::emission_index_H2O,
                      "The H2O emission index of the energy carrier in kg/kg.")
        .def_readonly("emission_index_SO2", &EnergyCarrier::emission_index_SO2,
                      "The SO2 emission index of the energy carrier in kg/kg.")
        .def_readonly("emission_index_SO4", &EnergyCarrier::emission_index_SO4,
                      "The SO4 emission index of the energy carrier in kg/kg.")
        .def_readonly("emission_index_soot", &EnergyCarrier::emission_index_soot,
                      "The soot emission index of the energy carrier in kg/kg.");
}
