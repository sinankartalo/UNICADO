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

#include <cmath>
#include <CGAL/Simple_cartesian.h>
#include <vector>
#include <numbers>
#include "coordinateSystemConversion/coordinateBase.h"


//namespace SimpleConversion
//{
//    using Kernel = CGAL::Simple_cartesian<double>;
//
//    auto CGAL2AC(const std::vector<double> &element3D) -> std::vector<double> 
//    {
//        Kernel::Aff_transformation_3 rotate_x = Kernel::Aff_transformation_3(
//            1, 0, 0,
//            0, std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0),
//            0, std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0));
//
//        Kernel::Aff_transformation_3 rotate_z = Kernel::Aff_transformation_3(
//            std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0), 0,
//            std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0), 0,
//            0, 0, 1);
//
//        Kernel::Aff_transformation_3 transform = rotate_x * rotate_z;
//
//        return transform(element3D);    
//    }
//
//    auto AC2CGAL(const std::vector<double> &element3D) -> std::vector<double>
//    {
//        Kernel::Aff_transformation_3 rotate_x = Kernel::Aff_transformation_3(
//            1, 0, 0,
//            0, std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0),
//            0, std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0));
//
//        Kernel::Aff_transformation_3 rotate_z = Kernel::Aff_transformation_3(
//            std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0), 0,
//            std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0), 0,
//            0, 0, 1);
//
//        Kernel::Aff_transformation_3 transform = rotate_x * rotate_z;
//
//        transform = transform.inverse();
//
//        return transform(element3D);
//    }
//}