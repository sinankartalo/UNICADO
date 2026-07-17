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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_FUSELAGE_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_FUSELAGE_H_

/* === Includes === */
#include <string_view>
#include "aircraftGeometry2/geometry/section.h"
#include "aircraftGeometry2/geometry/surface.h"
#include "aircraftGeometry2/geometry/factory.h"

namespace geom2
{
    /* === Classes === */
    /**
     * @class FuselageFactory
     * @brief Factory for fuselage geometry as defined in the aircraft xml file.
     */
    class FuselageFactory : public Factory<MultisectionSurface<PolygonSection>>
    {
    public:
        /**
         * @brief Construct a new Factory object
         * 
         * @tparam InputType The input format of the factory.
         * @param input The input data for the factory.
         * @param data_dir The path to the geometry data directory.
         */
        template <typename InputType>
        FuselageFactory(InputType input, const std::filesystem::path& data_dir)
        : Factory(input, data_dir) {}

        /**
         * @brief Call the builder of the factory to create a surface.
         * 
         * @param name The identifier of the surface.
         * @return geom2::MultisectionSurface<geom2::PolygonSection> The created surface.
         */
        auto create(std::string_view name) -> MultisectionSurface<PolygonSection> final
        {
            return this->builder->build_fuselage(name);
        }
    };
}; // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_FUSELAGE_H_
