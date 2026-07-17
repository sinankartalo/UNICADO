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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_AIRFOIL_SURFACE_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_AIRFOIL_SURFACE_H_

/* === Includes === */
#include <string_view>
#include "aircraftGeometry2/geometry/section.h"
#include "aircraftGeometry2/geometry/surface.h"
#include "aircraftGeometry2/geometry/factory.h"

namespace geom2
{

    /* === Classes === */
    /**
     * @class AirfoilSurfaceFactory
     * @brief Factory for airfoil surface geometry as defined in the aircraft xml file.
     * @details This factory is mainly intended for auxiliary surfaces like empennages or pylons.
     * It does **NOT** respect the following airfoil properties:
     * - *scale_thickness*: Always defaults to `1.0`
     * - *is_symmetric*: Always defaults to `false`
     * 
     * => Use geom2::WingFactory if these parameters are in use!
     */
    class AirfoilSurfaceFactory : public Factory<MultisectionSurface<AirfoilSection>>
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
        AirfoilSurfaceFactory(InputType input, const std::filesystem::path& data_dir) : Factory(input, data_dir) {}

        /**
         * @brief Call the builder of the factory to create a surface.
         * @note The extrusion direction of this surface is in the **negative**  `Z` direction!
         * @attention This function is mainly intended for auxiliary surfaces like empennages or pylons.
         * It does **NOT** respect the following airfoil properties:
         * - *scale_thickness*: Always defaults to `1.0`
         * - *is_symmetric*: Always defaults to `false`
         * 
         * => Use geom2::WingFactory if these parameters are in use!
         * 
         * @param name The identifier of the surface.
         * @return geom2::MultisectionSurface<geom2::AirfoilSection> The created surface.
         */
        auto create(std::string_view name) -> MultisectionSurface<AirfoilSection> final
        {
            return this->builder->build_airfoil_surface(name);
        }
    };

    /**
     * @class SparFactory
     * @brief Factory for spar geometry as defined in the aircraft xml file.
     */
    class SparFactory : public Factory<MultisectionSurface<PolygonSection>>
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
        SparFactory(InputType input, const std::filesystem::path& data_dir) : Factory(input, data_dir) {}

        /**
         * @brief Call the builder of the factory to create a surface.
         * @note The extrusion direction of this surface is in the **negative**  `Z` direction!
         * 
         * @param name The identifier of the surface.
         * @return geom2::MultisectionSurface<geom2::PolygonSection> The created surface.
         */
        auto create(std::string_view name) -> MultisectionSurface<PolygonSection> final
        {
            return this->builder->build_spar(name);
        }
    };

    /**
     * @class ControlDeviceFactory
     * @brief Factory for control device geometry as defined in the aircraft xml file.
     */
    class ControlDeviceFactory : public Factory<MultisectionSurface<PolygonSection>>
    {
    public:
        /**
         * @brief Construct a new Factory object
         * @note The extrusion direction of this surface is in the **negative**  `Z` direction!
         * 
         * @tparam InputType The input format of the factory.
         * @param input The input data for the factory.
         * @param data_dir The path to the geometry data directory.
         */
        template <typename InputType>
        ControlDeviceFactory(InputType input, const std::filesystem::path& data_dir) : Factory(input, data_dir) {}

        /**
         * @brief Call the builder of the factory to create a surface.
         * 
         * @param name The identifier of the surface.
         * @return geom2::ControlDeviceContainer Container which contains all control devices of the entity.
         */
        auto create(std::string_view name) -> MultisectionSurface<PolygonSection> final
        {
            return this->builder->build_control_device(name);
        }
    };

    /**
     * @class WingFactory
     * @brief Factory for wing geometry as defined in the aircraft xml file.
     */
    class WingFactory : public Factory<MultisectionSurface<AirfoilSection>>
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
        WingFactory(InputType input, const std::filesystem::path& data_dir) : Factory(input, data_dir) {}

        /**
         * @brief Call the builder of the factory to create a surface.
         * @note The extrusion direction of this surface is in the **negative**  `Z` direction!
         * 
         * @param name The identifier of the surface.
         * @return geom2::MultisectionSurface<geom2::AirfoilSection> The created surface.
         */
        auto create(std::string_view name) -> MultisectionSurface<AirfoilSection> final
        {
            return this->builder->build_wing(name);
        }
    };

}; // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_AIRFOIL_SURFACE_H_
