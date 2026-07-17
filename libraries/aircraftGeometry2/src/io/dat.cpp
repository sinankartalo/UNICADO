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

#include "aircraftGeometry2/io/dat.h"
#include <array>
#include <algorithm>
#include <fstream>

/* === Functions === */
namespace geom2
{
    namespace detail
    {
        /**
         * @brief Replaces all separators in a line with whitespace.
         * 
         * @param line The line to be processed.
         */
        void separators_to_whitespace(std::string *line)
        {
            /* Define the valid separators */
            constexpr std::array<char, 2> separators = {',', ';'};

            /* Replace all separators with whitespace */
            std::replace_if(
                line->begin(),
                line->end(),
                [&separators](const char &c) { return std::find(separators.begin(), separators.end(), c) != separators.end(); },
                ' ');
        };

    }; // namespace detail

    namespace io
    {
        auto read_dat_file(std::filesystem::path path) -> geom2::Polygon_2
        {
            /* Open the file and initialize the container for the points */
            std::ifstream file(path.make_preferred());
            std::vector<geom2::Point_2> points;

            /* Check whether the file exists */
            if (file.is_open())
            {
                /* Loop through each line of the file using the file stream */
                std::string line;
                while (std::getline(file, line))
                {
                    /* Make the line separated with whitespace */
                    detail::separators_to_whitespace(&line);

                    /*
                     * Use the stream operator of the string stream to extract
                     * the x and y coordinates from the line.
                     * The separator has to be whitespace at this point!
                     */
                    std::istringstream iss(line);
                    double x, y;
                    if ((iss >> x >> y))
                    {
                        points.emplace_back(x, y);
                    }
                }
            } else {
                /* Throw an exception if the file could not be opened */
                throw std::runtime_error("Could not open file: " + path.string());
            }

            /* Return the polygon with the loaded coordinates */
            return geom2::Polygon_2(points.begin(), points.end());
        }

        auto read_airfoil(std::filesystem::path path) -> geom2::Polygon_2
        {
            /* Temporary polygon for sorting the vertices */
            geom2::Polygon_2 poly_complex = read_dat_file(path);
            geom2::Polygon_2 poly_simple;

            /* Check the preconditions for a valid airfoil */
            /* Left point should be the leading edge is located at [0, y]*/
            if (std::abs(poly_complex.left_vertex()->x()) > 1e-4) {
                throw std::domain_error("The coordinates of the airfoil do not satisfy the preconditions!");
            }
            /* Right point should be located at [1,y] */
            if (std::abs(poly_complex.right_vertex()->x() - 1.0) > 1e-4) {
                throw std::domain_error("The coordinates of the airfoil do not satisfy the preconditions!");
            }

            /* Check whether the imported polygon has to be sorted */
            if (poly_complex[0].x() > 0.5) {
                /* Trailing edge first convention is used => sort that leading edge is first point */
                poly_complex.reverse_orientation();

                /* Insert the reversed top part */
                poly_simple.insert(poly_simple.vertices_end(), poly_complex.left_vertex(), poly_complex.vertices_end());

                /* Insert the reversed bottom part */
                poly_simple.insert(poly_simple.vertices_end(), poly_complex.vertices_begin() + 1, poly_complex.left_vertex());

                /* If polygon is not simple, the TE point is in there twice so remove it */
                if (!poly_simple.is_simple()) {
                    poly_simple.erase(poly_simple.vertices_end() - 1);
                }
            } else if (!poly_complex.is_simple() && poly_complex[0].x() < 0.5) {
                /* Leading edge first convention is used => Upper part is fine, but bottom part has to be reversed */
                /* Split the complex polygon into top and bottom */
                geom2::Polygon_2 poly_bottom;
                poly_simple.insert(poly_simple.vertices_end(), poly_complex.vertices_begin(), poly_complex.right_vertex());
                poly_bottom.insert(poly_bottom.vertices_end(), poly_complex.right_vertex(), poly_complex.vertices_end());

                /* Add the reversed bottom to the simple polygon */
                /* Also skip duplicate points so that the resulting polygon is simple */
                poly_bottom.reverse_orientation();
                poly_simple.insert(poly_simple.vertices_end(), poly_bottom.vertices_begin() + 1, poly_bottom.vertices_end() - 1);
            } else {
                poly_simple = poly_complex;
            }

            return poly_simple;
        }
    }; // namespace io
}; // namespace geom2
