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


TEST(AircraftAero, TestEngine)
{
    std::filesystem::path TN_deck{ CMAKE_TEST_STUBS_DIR };
    TN_deck /= "engine/PW1127G-JM/PW1127G-JM_FN.csv";
    std::filesystem::path Fuel_deck{ CMAKE_TEST_STUBS_DIR };
    Fuel_deck /= "engine/PW1127G-JM/PW1127G-JM_WF.csv";

    std::ifstream file(TN_deck);
    std::vector<double> mach_vec;
    std::vector<double> altitude_vec;
    std::vector<double> thrust_setting_vec;
    std::vector<types::PropertyType> thrust_value_vec;

    std::string line;
    std::vector<double> mach_numbers;
    double thrust_setting = 0.0;
    int data_row_count = 0;
    const int BLOCK_SIZE = 29;

    auto is_header = [](const std::vector<std::string>& tokens) {
        (void)tokens;
        return false; 
        };
    (void)is_header;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ';'))
            tokens.push_back(token);

        if (tokens.size() < 2) continue;

        bool is_header_row = (data_row_count % (BLOCK_SIZE + 1) == 0);

        if (is_header_row) {
            thrust_setting = std::stod(tokens[0]);
            mach_numbers.clear();
            for (size_t i = 1; i < tokens.size(); ++i)
                mach_numbers.push_back(std::stod(tokens[i]));
        }
        else {
            double altitude = std::stod(tokens[0]);
            for (size_t i = 1; i < tokens.size() && (i - 1) < mach_numbers.size(); ++i) {
                mach_vec.push_back(mach_numbers[i - 1]);
                altitude_vec.push_back(altitude);
                thrust_setting_vec.push_back(thrust_setting);
                types::PropertyType dummy;
                dummy["TN"] = std::stod(tokens[i]);
                thrust_value_vec.push_back(dummy);
            }
        }
        ++data_row_count;
    }
    types::PropertyType lhs;
    lhs["mach"] = mach_vec;
    lhs["altitude"] = altitude_vec;
    lhs["setting"] = thrust_setting_vec;
    std::vector<types::key> variables = { "mach", "altitude","setting" };
    linear::interpolation engine(lhs, thrust_value_vec, variables);
    EXPECT_EQ(engine.Delaunay_triangulation.number_of_vertices(), 9280);
}

TEST(AircraftAero, TestEngineMS)
{
	std::filesystem::path TN_deck{ CMAKE_TEST_STUBS_DIR };
	TN_deck /= "engine/V2527-A5/V2527-A5_FN.csv";
	std::filesystem::path Fuel_deck{ CMAKE_TEST_STUBS_DIR };
	Fuel_deck /= "engine/V2527-A5/V2527-A5_WF.csv";

    std::ifstream file(TN_deck);
    std::vector<double> mach_vec;
    std::vector<double> altitude_vec;
    std::vector<double> thrust_setting_vec;
    std::vector<types::PropertyType> thrust_value_vec;

    std::string line;
    std::vector<double> mach_numbers;
    double thrust_setting = 0.0;
    int data_row_count = 0;
    const int BLOCK_SIZE = 29;

    auto is_header = [](const std::vector<std::string>& tokens) {
        (void)tokens;
        return false; // logic handled by counter below
        };
    (void)is_header;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ';'))
            tokens.push_back(token);

        if (tokens.size() < 2) continue;

        bool is_header_row = (data_row_count % (BLOCK_SIZE + 1) == 0);

        if (is_header_row) {
            // col[0] = thrust setting, col[1..] = Mach numbers
            thrust_setting = std::stod(tokens[0]);
            mach_numbers.clear();
            for (size_t i = 1; i < tokens.size(); ++i)
                mach_numbers.push_back(std::stod(tokens[i]));
        }
        else {
            // col[0] = altitude, col[1..] = thrust values
            double altitude = std::stod(tokens[0]);
            for (size_t i = 1; i < tokens.size() && (i - 1) < mach_numbers.size(); ++i) {
                mach_vec.push_back(mach_numbers[i - 1]);
                altitude_vec.push_back(altitude);
                thrust_setting_vec.push_back(thrust_setting);
                types::PropertyType dummy;
                dummy["TN"] = std::stod(tokens[i]);
                thrust_value_vec.push_back(dummy);
            }
        }
        ++data_row_count;
    }
    types::PropertyType lhs;
    lhs["mach"] = mach_vec;
    lhs["altitude"] = altitude_vec;
    lhs["setting"] = thrust_setting_vec;
    std::vector<types::key> variables = { "mach", "altitude","setting" };
    linear::interpolation engine(lhs, thrust_value_vec, variables);
    types::PropertyType conditions;
    conditions["mach"] = 0.5;
    conditions["altitude"] = 10000.;
    conditions["setting"] = 0.7;
    engine.conditions = conditions;
    const types::key dum = "TN";
    auto T = engine(dum, conditions);
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> mach_dist(0.1, 0.9);
    std::uniform_real_distribution<double> alt_dist(1000., 13000.0);
    std::uniform_real_distribution<double> setting_dist(0.4, 1.0);
    std::cout <<"Thrust = " << T << "\n";
    
    std::vector<double> machs = {};
    std::vector<double> alts = {};
    std::vector<double> sets = {};
    for (int i = 0; i < 200000; ++i) {
        machs.push_back(mach_dist(rng));
        alts.push_back(alt_dist(rng));
        sets.push_back(setting_dist(rng));
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 200000; ++i) {
        conditions["mach"] = machs[i];
        conditions["altitude"] = alts[i];
        conditions["setting"] = sets[i];
        T = engine(dum, conditions);
        std::cout << "M = " << machs[i] << " h = " << alts[i] << " setting = " << sets[i] << " Thrust = " << T << "\n";
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto total_us = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Total time : " << total_ms << " ms\n";
    std::cout << "Per call   : " << total_us / 200000.0 << " ms\n";
}

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
    auto condition = aerodynamics::Flight_Condition(5000., 0.47);
    auto rho = condition.density;
    auto v = condition.u;
    double D = 3.96;
    double RPM = 1000.;
    std::cout << "J = " << v / ((RPM / 60.) * D) << "\n";
    types::PropertyType conditions;
    conditions["inclination"] = 25.;
    conditions["j"] = v/((RPM/60.)*D);
    const types::key dum = "CT";
    auto Ct = engine(dum, conditions);
    std::cout << "CT = " << Ct << " T = " << Ct * rho * std::pow((RPM / 60.), 2.) * std::pow(D, 4.) << "\n";
}