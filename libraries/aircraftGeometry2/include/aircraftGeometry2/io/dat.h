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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_IO_DAT_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_IO_DAT_H_

/* === Includes === */
#include <filesystem>
#include "aircraftGeometry2/geometry/entity3d.h"
#include "aircraftGeometry2/geometry/section.h"

namespace geom2
{
    namespace io
    {
        /**
         * @brief Reads a dat-file and returns the content as a 2D polygon.
         * @throws std::runtime_error When the file cannot be opened.
         * @attention The following delimiter are supported:
         *  - whitespace (space, tab, ...)
         *  - comma `,`
         *  - semicolon `;`
         * 
         * @param path The path to the dat-file.
         * @return geom2::Polygon_2 The content of the dat-file converted to a 2D polygon.
         */
        [[nodiscard]] auto read_dat_file(std::filesystem::path path) -> geom2::Polygon_2;

        /**
         * @brief Reads a dat-file containing airfoil coordinates and
         * sorts the points in counter-clockwise order.
         * @note When the file cannot be opened, a polygon with a single
         * point at its origin is returned.
         * @attention See `geom2::io::read_dat_file` for supported delimiters.
         *
         * @param path The path to the dat-file.
         * @return geom2::Polygon_2 The content of the dat-file converted to a 2D polygon.
         * @throws std::domain_error When:
         * - the leading edge of the airfoils is not at the origin
         * - the `X` coordinates are not positive
         * - the `X` coordinates are not normalized
         */
        [[nodiscard]] auto read_airfoil(std::filesystem::path path) -> geom2::Polygon_2;

    }; // namespace io
}; // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_IO_DAT_H_
