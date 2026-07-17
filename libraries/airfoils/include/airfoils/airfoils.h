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

#ifndef AIRFOILS_INCLUDE_AIRFOILS_AIRFOILS_H_
#define AIRFOILS_INCLUDE_AIRFOILS_AIRFOILS_H_

#include <aircraftGeometry2/io/dat.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class Airfoils {
 public:
  /**
   * \brief Construct a new Airfoils:: Airfoils object
   *
   * \param path_to_airfoil_directory
   */
  explicit Airfoils(const std::filesystem::path& path_to_airfoil_directory);

  /**
   * \brief Destroy the Airfoils object
   *
   */
  ~Airfoils() = default;

  /**
   * \brief Add directory airfoils to the available airfoils list
   *
   * \param path_to_airfoil_directory
   * \throws invalid_argument if directory does not exist
   */
  void add_directory_airfoils(const std::filesystem::path& path_to_airfoil_directory);

  /**
   * \brief Adds airfoil to available airfoils list
   *
   * \param airfoil_path airfoil path
   * \throws invalid_argument if airfoil does not exists
   */
  void add_airfoil(const std::filesystem::path& airfoil_path);

  /**
   * \brief Prints available airfoils - key and path
   *
   */
  void print_available_airfoils();

  /**
   * \brief Copy airfoil from available airfoils to an existing target directory
   *
   * \param airfoil airfoil key
   * \param target_directory path to target directory
   */
  void copy_available_airfoil(const std::string& airfoil, const std::filesystem::path& target_directory);

  /**
   * \brief Get airfoil shape
   *
   * \param airfoil airfoil key string from available airfoils
   * \throws invalid_argument if airfoil is not in available airfoils
   * \return geom2::Polygon_2 shape of the airfoil geometry
   */
  geom2::Polygon_2 get_airfoil(const std::string& airfoil);

 private:
  std::map<std::string, const std::filesystem::path>
      available_airfoils;  //<std::map of available airfoils and their specific path. first entry - airfoil name, second
                           //entry path to the airfoil.
};

#endif  // AIRFOILS_INCLUDE_AIRFOILS_AIRFOILS_H_
