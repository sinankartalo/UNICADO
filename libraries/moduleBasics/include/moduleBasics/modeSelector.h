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

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_MODESELECTOR_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_MODESELECTOR_H_

#include <map>
#include <string>

/* mode definition */
using modi = unsigned int;

/**
 * \brief Basic selector for modes in control_settings
 *
 */
class Modeselector {
 public:
  /**
   * \brief Construct a new Modeselector object
   *
   * \param mode string values for mode
   */
  explicit Modeselector(const std::string mode) {
    mode_ = mode;
    m_ = selection_[mode_];
  }

  /**
   * \brief Getter method for selected modi
   *
   * \return modi
   */
  modi get_mode_numeric() { return m_; }
  std::string get_mode_string() { return mode_; }

 private:
  modi m_{0};
  std::string mode_{"mode_0"};
  std::map<std::string, modi> selection_{{"mode_0", 0}, {"mode_1", 1}, {"mode_2", 2}, {"mode_3", 3}, {"mode_4", 4},
                                            {"mode_5", 5}, {"mode_6", 6}, {"mode_7", 7}, {"mode_8", 8}, {"mode_9", 9}};
};

#endif  // MODULEBASICS_INCLUDE_MODULEBASICS_MODESELECTOR_H_
