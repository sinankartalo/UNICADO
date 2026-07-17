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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_FACTORY_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_FACTORY_H_

/* === Includes === */
#include <filesystem>
#include <memory>
#include <string_view>
#include <aixml/node.h>
#include "aircraftGeometry2/geometry/builder.h"

namespace geom2
{
    /* === Classes === */
    /**
     * @class Factory
     * @brief Factory base class which is used to create geometries.
     * The factory is the interface of different input formats to
     * the geometry classes. The factory has custom constructor
     * overloads for each input format and defines its builder
     * strategy accordingly.
     * 
     * @tparam Surface The surface type that is created by the factory.
     */
    template<typename Surface>
    class Factory
    {
    public:
        /* No default Constructor */
        Factory() = delete;

        /**
         * @brief Virtual destructor as the class is used as a base class.
         */
        virtual ~Factory() = default;

        /**
         * @brief Construct a new Factory object which uses the 
         * AIXML format to create the geometry.
         * @note The AIXML format version is deduced automatically.
         * 
         * @param input The root node of the AIXML input file.
         * @param data_dir The path to the data directory where to find external files.
         */
        explicit Factory(std::shared_ptr<node> input, const std::filesystem::path &data_dir);

        /**
         * @brief Create the surface geometry.
         * 
         * @param name The identifier of the surface.
         * @return Surface The created surface.
         */
        virtual auto create(std::string_view name) -> Surface = 0;

    protected:
        std::unique_ptr<SurfaceBuilder> builder; /**< The pointer to the builder strategy */
    };
}; // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_FACTORY_H_
