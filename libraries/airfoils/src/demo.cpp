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

#include <string>
int main(int argc, char** argv) {
  try {
    Airfoils airfoils("../airfoils/available_airfoil_storage");
    airfoils.print_available_airfoils();
    airfoils.copy_available_airfoil("naca0012", "../airfoils/copy_airfoils_storage");
  } catch (std::string& e) {
    std::cerr << e << std::endl;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}
