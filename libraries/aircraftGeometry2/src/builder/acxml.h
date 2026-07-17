/*  Copyright (C) 2010-2023 Institute of Aerospace Systems, RWTH Aachen
   University - All rights reserved. This file is part of UNICADO.

    UNICADO is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    UNICADO is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with UNICADO.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @file axxml.h
 * @author Sebastian Oberschwendtner (sebastian.oberschwendtner@tum.de)
 * @brief Builder for creating surfaces defined by the aircraft exchange format.
 * @version 2.1.0
 * @date 2023-11-21
 */

#ifndef AIRCRAFTGEOMETRY2_SRC_BUILDER_ACXML_H_
#define AIRCRAFTGEOMETRY2_SRC_BUILDER_ACXML_H_

/* === Includes === */
#include <filesystem>
#include <memory>
#include <string_view>
#include <aixml/node.h>
#include "aircraftGeometry2/geometry/builder.h"

/* === Classes === */
namespace geom2
{
    /**
     * @class AIXMLv2
     * @brief Surface builder for the aircraft exchange format version 2.
     */
    class AIXMLv2 : public SurfaceBuilder
    {
    public:
        /**
         * @brief Construct a new AIXMLv2 object from an aircraft xml file.
         * 
         * @param AcXml Pointer to the aircraft xml file.
         * @param data_dir The path to the geometry data directory.
         */
        AIXMLv2(std::shared_ptr<node> AcXml, const std::filesystem::path& data_dir);

        /* Add default destructor */
        ~AIXMLv2() override = default;

        /* Methods to build the different surface types */
        auto build_hull(std::string_view name) -> MultisectionSurface<PolygonSection> final;
        auto build_fuselage(std::string_view name) -> MultisectionSurface<PolygonSection> final;
        auto build_airfoil_surface(std::string_view name) -> MultisectionSurface<AirfoilSection> final;
        auto build_spar(std::string_view name) -> MultisectionSurface<PolygonSection> final;
        auto build_control_device(std::string_view name) -> MultisectionSurface<PolygonSection> final;
        auto build_wing(std::string_view name) -> MultisectionSurface<AirfoilSection> final;
    private:
        /**
         * @brief Extract a point from the aircraft xml file.
         * 
         * @param id The point identifier.
         * @return geom2::Point_3 The point.
         */
        [[nodiscard]] auto get_point(std::string_view id) const -> geom2::Point_3;

        /* Properties */
        std::shared_ptr<node> aircraft;                                 /**< The pointer to the aircraft xml file. */
        std::filesystem::path data_dir{};                               /**< The path to the geometry data directory. */
    };

    /**
     * @class AIXMLv3
     * @brief Surface builder for the aircraft exchange format version 3.
     */
    class AIXMLv3 : public SurfaceBuilder
    {
    public:
        /**
         * @brief Construct a new AIXMLv3 Builder from an aircraft xml file.
         * 
         * @param AcXml Pointer to the aircraft xml file.
         * @param data_dir The path to the geometry data directory.
         */
        AIXMLv3(std::shared_ptr<node> AcXml, const std::filesystem::path& data_dir);

        /* Add default destructor */
        ~AIXMLv3() override = default;

        /* Methods to build the different surface types */
        auto build_hull(std::string_view name) -> MultisectionSurface<PolygonSection> final;
        auto build_fuselage(std::string_view name) -> MultisectionSurface<PolygonSection> final;
        auto build_airfoil_surface(std::string_view name) -> MultisectionSurface<AirfoilSection> final;
        auto build_spar(std::string_view name) -> MultisectionSurface<PolygonSection> final;
        auto build_control_device(std::string_view name) -> MultisectionSurface<PolygonSection> final;
        auto build_wing(std::string_view name) -> MultisectionSurface<AirfoilSection> final;
    private:
        /**
         * @brief Extract a point from the aircraft xml file.
         * 
         * @param id The point identifier.
         * @return geom2::Point_3 The point.
         */
        [[nodiscard]] auto get_point(std::string_view id) const -> geom2::Point_3;

        /**
         * @brief Extract a direction from the aircraft xml file.
         * 
         * @param id The direction identifier.
         * @return geom2::Direction_3 The direction.
         */
        [[nodiscard]] auto get_direction(std::string_view id) const -> geom2::Direction_3;

        /* Properties */
        std::shared_ptr<node> aircraft;                                 /**< The pointer to the aircraft xml file. */
        std::filesystem::path data_dir{};                               /**< The path to the geometry data directory. */
    };
}; // namespace geom2
#endif // AIRCRAFTGEOMETRY2_SRC_BUILDER_ACXML_H_
