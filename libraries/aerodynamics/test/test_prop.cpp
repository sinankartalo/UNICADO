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

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <random>
#include <chrono>
#include <gtest/gtest.h>
#include <aixml/node.h>
#include <aixml/endnode.h>
#include "aerodynamics/aerodynamics_v3.h"
#include "interpolation/data_types.h"

TEST(AircraftAero, TestProp)
{
    std::filesystem::path TN_deck{ CMAKE_TEST_STUBS_DIR };
    TN_deck /= "engine/propeller/prop.csv";

    std::ifstream file(TN_deck);
    std::vector<double> inclination_vec;
    std::vector<double> j_vec;
    std::vector<types::PropertyType> prop_vec;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ','))
            tokens.push_back(token);
        if (tokens.size() < 5) continue;

        double inclination, j, ct, cp, eta;
        try {
            inclination = std::stod(tokens[0]);
            j = std::stod(tokens[1]);
            ct = std::stod(tokens[2]);
            cp = std::stod(tokens[3]);
            eta = std::stod(tokens[4]);
        }
        catch (const std::invalid_argument&) {
            continue;
        }

        inclination_vec.push_back(inclination);
        j_vec.push_back(j);

        types::PropertyType dummy;
        dummy["CT"] = ct;
        dummy["CP"] = cp;
        dummy["eta"] = eta;
        prop_vec.push_back(dummy);
    }
    types::PropertyType lhs;
    lhs["inclination"] = inclination_vec;
    lhs["j"] = j_vec;
    std::vector<types::key> variables = { "inclination", "j" };
    linear::interpolation engine(lhs, prop_vec, variables);
    auto condition = aerodynamics::Flight_Condition(5000., 0.3);
    auto rho = condition.density;
    auto v = condition.u;
    double D = 3.96;
    double RPM = 1200.;
    std::cout << "J = " << v / ((RPM / 60.) * D) << "\n";
    types::PropertyType conditions;
    conditions["inclination"] = 45.;
    conditions["j"] = v/((RPM/60.)*D);
    const types::key dum = "CT";
    auto Ct = engine(dum, conditions);
    std::cout << "CT = " << Ct << " T = " << Ct * rho * std::pow((RPM / 60.), 2.) * std::pow(D, 4.) << "\n";
}