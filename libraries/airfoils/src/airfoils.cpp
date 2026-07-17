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

#include "airfoils/airfoils.h"

#include <standardFiles/functions.h>

#include <format>
#include <iostream>

Airfoils::Airfoils(const std::filesystem::path& path_to_airfoil_directory) {
  if (!std::filesystem::exists(path_to_airfoil_directory)) {
    throwError(__FILE__, __func__, __LINE__,
               std::format("Directory {} does not exist!", path_to_airfoil_directory.string()));
  }
  add_directory_airfoils(path_to_airfoil_directory);
}

void Airfoils::print_available_airfoils() {
  for (const auto& item : available_airfoils) {
    std::cout << std::format("Airfoil ... {:<20.50} ... path {:<50.100}", item.first, item.second.string())
              << std::endl;
  }
}

geom2::Polygon_2 Airfoils::get_airfoil(const std::string& airfoil) {
  geom2::Polygon_2 airfoil_data;

  auto airfoil_data_it = available_airfoils.find(airfoil);
  if (airfoil_data_it == available_airfoils.end()) {
    throwError(__FILE__, __func__, __LINE__, std::format("Airfoil {} not available ...", airfoil));
  }
  return geom2::io::read_airfoil(airfoil_data_it->second);
}

void Airfoils::copy_available_airfoil(const std::string& airfoil, const std::filesystem::path& target_directory) {
  auto airfoil_it = available_airfoils.find(airfoil);
  if (!std::filesystem::exists(target_directory)) {
    throwError(__FILE__, __func__, __LINE__,
               std::format("Target directory {} does not exist", target_directory.string()));
  }
  if (airfoil_it == available_airfoils.end()) {
    throwError(__FILE__, __func__, __LINE__, std::format("Airfoil {} is not available", airfoil));
  }
  std::filesystem::path absolute_target_directory = std::filesystem::absolute(target_directory);
  std::filesystem::path absolute_airfoils_path = std::filesystem::absolute(airfoil_it->second);
  if (absolute_target_directory.string().compare(absolute_airfoils_path.parent_path().string()) == 0) {
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->out << std::format("Copying airfoil skipped - airfoil {} is already in {}", airfoil,
                                        target_directory.string())
                         << std::endl;
    } else {
      std::cout << std::format("Copying airfoil skipped - airfoil {} is already in {}", airfoil,
                               target_directory.string())
                << std::endl;
    }
    return;
  }
  try {
    std::filesystem::copy(absolute_airfoils_path, absolute_target_directory,
                          std::filesystem::copy_options::update_existing);
  } catch (std::exception& e) {
    throwError(__FILE__, __func__, __LINE__, e.what());
  }
}

void Airfoils::add_directory_airfoils(const std::filesystem::path& path_to_airfoil_directory) {
  if (!std::filesystem::exists(path_to_airfoil_directory)) {
    throw std::invalid_argument(std::format("Directory {} does not exist!", path_to_airfoil_directory.string()));
    // throwError(__FILE__,__func__,__LINE__,std::format("Directory {} does not exist!",
    // path_to_airfoil_directory.string()));
  }
  for (const auto& entry : std::filesystem::directory_iterator(path_to_airfoil_directory)) {
    add_airfoil(entry.path());
  }
}

void Airfoils::add_airfoil(const std::filesystem::path& airfoil_path) {
  if (!std::filesystem::exists(airfoil_path)) {
    throwError(__FILE__, __func__, __LINE__, std::format("Airfoil {} does not exist", airfoil_path.string()));
  }
  if (std::filesystem::is_regular_file(airfoil_path) && airfoil_path.extension() == ".dat") {
    std::string airfoil = airfoil_path.filename().stem().string();
    available_airfoils.try_emplace(airfoil, airfoil_path);
  }
}
