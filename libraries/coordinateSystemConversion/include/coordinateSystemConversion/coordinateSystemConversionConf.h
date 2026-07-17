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

#ifndef COORDINATESYSTEMCONVERSION_COORDINATESYSTEMCONVERSIONCONF_H_
#define COORDINATESYSTEMCONVERSION_COORDINATESYSTEMCONVERSIONCONF_H_

#include <cmath>
#include <CGAL/Simple_cartesian.h>
#include "coordinateBase.h"
#include <vector>
#include <numbers>

namespace SimpleConversion
{
    using Kernel = CGAL::Simple_cartesian<double>;

    struct Element3D {
        
        Element3D() = default;

        Kernel::Aff_transformation_3 tensor3D = Kernel::Aff_transformation_3(
            0, 0, 0,
            0, 0, 0,
            0, 0, 0);

        Kernel::Point_3 point3D = Kernel::Point_3(0, 0, 0);

        /* Initialize the element*/
        Element3D(
            const double xx, const double xy, const double xz, 
            const double yx, const double yy, const double yz, 
            const double zx, const double zy, const double zz)
        {
            /* The element can contain a tensor or a point/direction */
            tensor3D = Kernel::Aff_transformation_3(xx, xy, xz, yx, yy, yz, zx, zy, zz);
            point3D = Kernel::Point_3(xx, yy, zz);
        };

        Element3D(const double x, const double y, const double z)
        {
            tensor3D = Kernel::Aff_transformation_3(
                x, 0.0, 0.0,
                0.0, y, 0.0,
                0.0, 0.0, z);
            point3D = Kernel::Point_3(x, y, z);
        };

        virtual ~Element3D() = default;

        /* Getter functions */
        [[nodiscard]] auto xx() const noexcept -> const double { return  tensor3D.m(0, 0); }
        [[nodiscard]] auto xy() const noexcept -> const double { return  tensor3D.m(0, 1); }
        [[nodiscard]] auto xz() const noexcept -> const double { return  tensor3D.m(0, 2); }
        [[nodiscard]] auto yx() const noexcept -> const double { return  tensor3D.m(1, 0); }
        [[nodiscard]] auto yy() const noexcept -> const double { return  tensor3D.m(1, 1); }
        [[nodiscard]] auto yz() const noexcept -> const double { return  tensor3D.m(1, 2); }
        [[nodiscard]] auto zx() const noexcept -> const double { return  tensor3D.m(2, 0); }
        [[nodiscard]] auto zy() const noexcept -> const double { return  tensor3D.m(2, 1); }
        [[nodiscard]] auto zz() const noexcept -> const double { return  tensor3D.m(2, 2); }
        [[nodiscard]] auto x() const noexcept -> const double& { return  point3D.x(); }
        [[nodiscard]] auto y() const noexcept -> const double& { return  point3D.y(); }
        [[nodiscard]] auto z() const noexcept -> const double& { return  point3D.z(); }

        /* Conversion from CGAL Coordinate System to AC Coordinate System */
        auto CGAL2AC() {
            Kernel::Aff_transformation_3 rotate_x = Kernel::Aff_transformation_3(
                1, 0, 0,
                0, std::cos(-std::numbers::pi / 2.0), -std::sin(-std::numbers::pi / 2.0),
                0, std::sin(-std::numbers::pi / 2.0), std::cos(-std::numbers::pi / 2.0));

            Kernel::Aff_transformation_3 rotate_z = Kernel::Aff_transformation_3(
                std::cos(-std::numbers::pi / 2.0), -std::sin(-std::numbers::pi / 2.0), 0,
                std::sin(-std::numbers::pi / 2.0), std::cos(-std::numbers::pi / 2.0), 0,
                0, 0, 1);

            Kernel::Aff_transformation_3 transform_matrix = rotate_x * rotate_z;

            Kernel::Aff_transformation_3 transposed_transform_matrix = Kernel::Aff_transformation_3(
                transform_matrix.m(0, 0), transform_matrix.m(1, 0), transform_matrix.m(2, 0),
                transform_matrix.m(0, 1), transform_matrix.m(1, 1), transform_matrix.m(2, 1),
                transform_matrix.m(0, 2), transform_matrix.m(1, 2), transform_matrix.m(2, 2));

            tensor3D = transform_matrix * tensor3D * transposed_transform_matrix;
            point3D = transform_matrix(point3D);
        };

        /* Conversion from AC Coordinate System to CGAL Coordinate System */
        auto AC2CGAL() {
            Kernel::Aff_transformation_3 rotate_x = Kernel::Aff_transformation_3(
                1, 0, 0,
                0, std::cos(-std::numbers::pi / 2.0), -std::sin(-std::numbers::pi / 2.0),
                0, std::sin(-std::numbers::pi / 2.0), std::cos(-std::numbers::pi / 2.0));

            Kernel::Aff_transformation_3 rotate_z = Kernel::Aff_transformation_3(
                std::cos(-std::numbers::pi / 2.0), -std::sin(-std::numbers::pi / 2.0), 0,
                std::sin(-std::numbers::pi / 2.0), std::cos(-std::numbers::pi / 2.0), 0,
                0, 0, 1);

            Kernel::Aff_transformation_3 transform_matrix = rotate_x * rotate_z;

            transform_matrix = transform_matrix.inverse();

            Kernel::Aff_transformation_3 transposed_transform_matrix = Kernel::Aff_transformation_3(
                transform_matrix.m(0, 0), transform_matrix.m(1, 0), transform_matrix.m(2, 0),
                transform_matrix.m(0, 1), transform_matrix.m(1, 1), transform_matrix.m(2, 1),
                transform_matrix.m(0, 2), transform_matrix.m(1, 2), transform_matrix.m(2, 2));

            tensor3D = transform_matrix * tensor3D * transposed_transform_matrix;
            point3D = transform_matrix(point3D);
        };

        /* Conversion from using specific angles */
        auto CS12CS2(const double theta_x, const double theta_y, const double theta_z) {
            Kernel::Aff_transformation_3 rotate_x = Kernel::Aff_transformation_3(
                1, 0, 0,
                0, std::cos(theta_x * std::numbers::pi / 180), -std::sin(theta_x * std::numbers::pi / 180),
                0, std::sin(theta_x * std::numbers::pi / 180), std::cos(theta_x * std::numbers::pi / 180));

            Kernel::Aff_transformation_3 rotate_y = Kernel::Aff_transformation_3(
                std::cos(theta_y * std::numbers::pi / 180), 0, std::sin(theta_y * std::numbers::pi / 180),
                0, 1, 0,
                -std::sin(theta_y * std::numbers::pi / 180), 0, std::cos(theta_y * std::numbers::pi / 180));

            Kernel::Aff_transformation_3 rotate_z = Kernel::Aff_transformation_3(
                std::cos(theta_z * std::numbers::pi / 180), -std::sin(theta_z * std::numbers::pi / 180), 0,
                std::sin(theta_z * std::numbers::pi / 180), std::cos(theta_z * std::numbers::pi / 180), 0,
                0, 0, 1);

            Kernel::Aff_transformation_3 transform_matrix = rotate_x * rotate_y * rotate_z;

            Kernel::Aff_transformation_3 transposed_transform_matrix = Kernel::Aff_transformation_3(
                transform_matrix.m(0, 0), transform_matrix.m(1, 0), transform_matrix.m(2, 0),
                transform_matrix.m(0, 1), transform_matrix.m(1, 1), transform_matrix.m(2, 1),
                transform_matrix.m(0, 2), transform_matrix.m(1, 2), transform_matrix.m(2, 2));

            tensor3D = transform_matrix * tensor3D * transposed_transform_matrix;
            point3D = transform_matrix(point3D);
        };
    };

    //auto CGAL2AC(const std::vector<double> &element3D) -> std::vector<double>;

    //auto AC2CGAL(const std::vector<double> &element3D) -> std::vector<double>;
}

#endif // COORDINATESYSTEMCONVERSION_COORDINATESYSTEMCONVERSIONCONF_H_