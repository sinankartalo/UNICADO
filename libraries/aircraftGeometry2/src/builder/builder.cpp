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

#include "aircraftGeometry2/geometry/builder.h"
#include <algorithm>
#include <cmath>
#include <numeric>

/* === Functions === */
namespace geom2
{
    namespace build
    {
        auto ellipse(const double width, const double height, const size_t points_per_quarter) -> PolygonSection
        {
            /* Initial values */
            Polygon_2 shape;
            shape.resize(4 * (points_per_quarter - 1));
            const double ratio = height / width;

            /* Create the equidistant normalized x coordinates of the ellipse */
            std::vector<double> x(2 * points_per_quarter - 1);
            for (size_t i = 0; i < (2 * points_per_quarter - 1); ++i)
            {
                x.at(i) = -1.0 + (i / static_cast<double>(( points_per_quarter - 1)));
            }

            /* Compute the absolute normalized y coordinates of the ellipse */
            std::vector<double> y_abs(x);
            std::transform(x.begin(), x.end(), y_abs.begin(),
                           [&ratio](const double x) -> double
                           { return ratio * std::sqrt(1.0 - std::pow(x, 2.0)); });

            /* Insert the upper half of the ellipse in the polygon */
            std::transform(x.begin(), x.end() - 1, y_abs.begin(), shape.vertices_begin(),
                           [](const double x, const double y) -> Point_2 { return Point_2(x, y); });

            /* Insert the lower half of the ellipse in the polygon */
            std::transform(x.rbegin(), x.rend() - 1, y_abs.rbegin(), shape.vertices_begin() + ( 2*(points_per_quarter-1)),
                           [](const double x, const double y) -> Point_2 { return Point_2(x, -y); });

            /* Transform the normalized coordinates to absolute values */
            const Kernel::Aff_transformation_2 scale{
                Kernel::Aff_transformation_2(CGAL::SCALING, width/2)};

            /* Create the polygon section */
            PolygonSection section{CGAL::transform(scale, shape)};

            /* Return the scaled section */
            return section;
        };
    }; // namespace build
};     // namespace geom2
