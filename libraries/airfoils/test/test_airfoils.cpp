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

#include <airfoils/airfoils.h>
#include <gtest/gtest.h>

#include <filesystem>

std::filesystem::path executable_path(".");
std::filesystem::path test_directory("../../../airfoils/test");
std::filesystem::path test_airfoil_directory = test_directory.string() + "/test_airfoil_directory";
std::filesystem::path test_copy_directory = test_directory.string() + "/test_copy_directory";

TEST(airfoilsAddDirectory, addDirectorySuccess) { EXPECT_NO_THROW(Airfoils(test_airfoil_directory.string())); }

TEST(airfoilsAddDirectory, addDirectoryFailure) { EXPECT_ANY_THROW(Airfoils("bla")); }

/**
 * \brief Airfoil add testcase success
 *
 */
TEST(airfoilsAddAirfoil, addAirfoilSuccess) {
  Airfoils airfoils(test_airfoil_directory.string());
  std::filesystem::path airfoil_path = test_airfoil_directory.string() + "/F15_failure.dat";
  EXPECT_NO_THROW(airfoils.add_airfoil(airfoil_path));
}

/**
 * \brief Airfoil add testcase failure
 *
 */
TEST(airfoilsAddAirfoil, addAirfoilFailure) {
  Airfoils airfoils(test_airfoil_directory.string());

  EXPECT_ANY_THROW(airfoils.add_airfoil("bla"));
}

/**
 * \brief Copy airfoil success
 *
 */
TEST(airfoilsCopyAirfoil, copyAirfoilSuccess) {
  Airfoils airfoils(test_airfoil_directory.string());
  std::string f15_failure_airfoil_key = "F15_failure";
  airfoils.copy_available_airfoil(f15_failure_airfoil_key, test_copy_directory);
  std::filesystem::path copied_path = test_copy_directory.string() + "/" + f15_failure_airfoil_key + ".dat";
  EXPECT_TRUE(std::filesystem::exists(copied_path));
}

/**
 * \brief Get Airfoil Success
 *
 */
TEST(airfoilsGetAirfoil, getAirfoilSuccess) {
  Airfoils airfoils(test_airfoil_directory.string());
  std::string n0012_airfoil_key = "n0012";
  auto n0012_airfoil = airfoils.get_airfoil(n0012_airfoil_key);
  EXPECT_EQ(n0012_airfoil.vertices().size(), 200);
}

/**
 * \brief Get Airfoil Failure -> not in list
 *
 */
TEST(airfoilsGetAirfoil, getAirfoilNotInListFailure) {
  Airfoils airfoils(test_airfoil_directory.string());
  std::string false_airfoil_key = "n0";
  EXPECT_ANY_THROW(airfoils.get_airfoil(false_airfoil_key));
}

/**
 * \brief Get Airfoil Failure -> no valid points
 *
 */
TEST(airfoilsGetAirfoil, getAirfoilInListFailure) {
  Airfoils airfoils(test_airfoil_directory.string());
  std::string correct_airfoil_key = "naca0012_failure";
  EXPECT_ANY_THROW(airfoils.get_airfoil(correct_airfoil_key));
}
