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

#ifndef SRC_SI_UNITS_H_
#define SRC_SI_UNITS_H_

/* === Includes === */
#include <string_view>

/* === Functions === */
namespace SI {
    /**
     * @brief Convert a force value to the preferred SI unit [N].
     * Valid input units are:
     * - [N]
     * - [kN]
     * 
     * @param value The value to convert.
     * @param unit The unit of the value.
     * @return double The converted value in the preferred unit.
     */
    auto force(const double value, const std::string_view unit) -> double;

    /**
     * @brief Convert a length value to the preferred SI unit [m].
     * Valid input units are:
     * - [m]
     * - [ft]
     * 
     * @param value The value to convert.
     * @param unit The unit of the value.
     * @return double The converted value in the preferred unit.
     */
    auto length(const double value, const std::string_view unit) -> double;

    /**
     * @brief Convert a mass value to the preferred SI unit [kg].
     * Valid input units are:
     * - [kg]
     * - [lbs]
     * 
     * @param value The value to convert.
     * @param unit The unit of the value.
     * @return double The converted value in the preferred unit.
     */
    auto mass(const double value, const std::string_view unit) -> double;

    /**
     * @brief Convert a temperature value to the preferred SI unit [K].
     * Valid input units are:
     * - [K]
     * - [C]
     * - [F]
     * @param value The value to convert.
     * @param unit The unit of the value.
     * @return double The converted value in the preferred unit.
     */
    auto temperature(const double value, const std::string_view unit) -> double;

    /**
     * @brief Convert a power value to the preferred SI unit [W].
     * Valid input units are:
     * - [kW]
     *
     * @param value The value to convert.
     * @param unit The unit of the value.
     * @return double The converted value in the preferred unit.
     */
    auto power(const double value, const std::string_view unit) -> double;
}; // namespace SI

#endif  // SRC_SI_UNITS_H_

