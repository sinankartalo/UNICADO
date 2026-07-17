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

#include "SI_units.h"
#include <iostream>
#include <unitConversion/unitConversion.h>

/* === Functions === */
namespace SI
{
    auto force(const double value, const std::string_view unit) -> double
    {
        /* Return when unit is already the preferred one */
        if (unit == "N") {
            return value;
        }

        /* Convert to the preferred unit */
        double factor{1.0};
        if (unit == "kN") {
            factor = 1000.0;
        }
        else
        {
            std::cerr << "Unknown unit '" << unit << "' for force. Assuming [N].\n";
        }
        return factor * value;
    }

    auto length(const double value, const std::string_view unit) -> double
    {
        /* Return when unit is already the preferred one */
        if (unit == "m") {
            return value;
        }

        /* Convert to the preferred unit */
        double factor{1.0};
        if (unit == "ft") {
            factor = convertLength(FOOT, METER, 1.0);
        }
        else
        {
            std::cerr << "Unknown unit '" << unit << "' for length. Assuming [m].\n";
        }
        return factor * value;
    }

    auto mass(const double value, const std::string_view unit) -> double
    {
        /* Return when unit is already the preferred one */
        if (unit == "kg") {
            return value;
        }

        /* Convert to the preferred unit */
        double factor{1.0};
        if (unit == "lbs") {
            factor = convertMass(POUND, GRAM, 1) / 1000; // convert to kg
        }
        else
        {
            std::cerr << "Unknown unit '" << unit << "' for mass. Assuming [kg].\n";
        }
        return factor * value;
    }

    auto temperature(const double value, const std::string_view unit) -> double
    {
        /* Return when unit is already the preferred one */
        if (unit == "K") {
            return value;
        }

        /* Convert to the preferred unit */
        double factor{1.0};
        if (unit == "C") {
            factor = convertTemperature(CELCIUS, KELVIN, 1); // convert to K
        }
        else if (unit == "F")
        {
            factor = convertTemperature(FAHRENHEIT, KELVIN, 1); // convert to K
        }
        else
        {
            std::cerr << "Unknown unit '" << unit << "' for temperature. Assuming [K].\n";
        }
        return factor * value;
    }

    auto power(const double value, const std::string_view unit) -> double
    {
        /* Return when unit is already the preferred one */
        if (unit == "W") {
            return value;
        }

        /* Convert to the preferred unit */
        double factor{1.0};
        if (unit == "kW") {
            factor = 1000; // convert to W
        }
        else
        {
            std::cerr << "Unknown unit '" << unit << "' for temperature. Assuming [K].\n";
        }
        return factor * value;
    }
} // namespace SI

