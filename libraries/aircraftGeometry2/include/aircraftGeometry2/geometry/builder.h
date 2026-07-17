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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_BUILDER_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_BUILDER_H_

/* === Includes === */
#include "aircraftGeometry2/geometry/section.h"
#include "aircraftGeometry2/geometry/surface.h"
#include <string_view>
#include <utility>
#include <vector>

namespace geom2
{
    /* === Functions ===*/
    namespace build
    {
        /**
         * @brief Create a polygon section in the shape of an ellipse.
         * @note The polygon points will be absolute coordinates as
         * the height and width of the ellipse. The internal scaling
         * is set to 1.0 when creating the ellipse.
         * 
         * -> The height and width are the diameters of the ellipse
         * measured in the local x and y direction.
         * 
         * @details The ellipse will be represented by a polygon with the given
         * number of points per quarter. The points will be equidistant
         * in the normalized x direction.
         * The total number of points will be 4 * (points_per_quarter - 1).
         * 
         * @param width The width of the ellipse in local `X` direction.
         * @param height The height of the ellipse in local `Y` direction.
         * @param points_per_quarter The number of points per quarter of the ellipse. Default is 20.
         * @return PolygonSection The created polygon section.
         */
        [[nodiscard]] auto ellipse(const double width, const double height, const size_t points_per_quarter = 20) -> PolygonSection;
    }; // namespace build

    /**
     * @brief The type of the container that holds the control devices.
     * It is just an alias for a map which contains vectors with the
     * control devices for each section.
     * @note The map usually contains two entries "LE" and "TE"
     */
    // using ControlDeviceContainer = std::unordered_map<std::string, std::vector<MultisectionSurface<PolygonSection>>>;

    /* === Classes === */
    /**
     * @class SectionBuilder
     * @brief This builder class is used to create sections for a surface.
     * It contains methods to quickly create the basic structure of
     * multi-section surfaces.
     *
     * @tparam SectionType
     */
    template <Shape SectionType>
    class SectionBuilder
    {
        /* Check whether the SectionType is derived from the Section class */
        static_assert(std::is_base_of_v<Section, SectionType>, "SectionType must be derived from Section!");

      public:
        /* Constructor */
        /**
         * @brief Default constructor.
         */
        SectionBuilder() = default;

        /**
         * @brief Virtual destructor since this class is used as a base class.
         */
        virtual ~SectionBuilder() = default;

        /**
         * @brief Create the sections and return them as a vector.
         * @attention The sections are moved from the factory!
         * After creating them, you cannot change them with the factory anymore!
         *
         * @return std::vector<SectionType> The created sections.
         */
        [[nodiscard]] auto get_result() noexcept -> std::vector<SectionType>
        {
            return std::move(sections);
        }

        /**
         * @brief Look at the current sections without moving them.
         * @note Not available in the Python binding!
         *
         * @return const std::vector<SectionType>& The current sections.
         */
        [[nodiscard]] auto peek() const noexcept -> const std::vector<SectionType> &
        {
            return sections;
        }

        /**
         * @brief Insert a new section at the back of the surface.
         *
         * @param section The section to be inserted.
         * @param offset The offset of the section to the previous section.
         */
        void insert_back(const SectionType &section, Vector_3 offset)
        {
            /* Get previous origin */
            Point_3 origin_previous{0, 0, 0};
            if (!this->sections.empty())
            {
                origin_previous = this->sections.back().origin;
            }

            /* Add the section to the surface */
            this->sections.emplace_back(section);

            /* Apply offset */
            this->sections.back().origin = origin_previous + offset;
        }

        /**
         * @brief Insert a new section at the back of the surface.
         *
         * @param shape The shape of the section.
         * @param offset The offset of the section to the previous section.
         */
        void insert_back(const Polygon_2 &shape, Vector_3 offset)
        {
            /* Get previous origin */
            Point_3 origin_previous{0, 0, 0};
            if (!this->sections.empty())
            {
                origin_previous = this->sections.back().origin;
            }

            /* Create the polygon section and apply offsets*/
            SectionType section(shape);
            section.origin = origin_previous + offset;

            /* Add the section to the surface */
            this->sections.emplace_back(section);
        }

        /**
         * @brief Arrange count sections of the given shape with the given offset
         * and insert them to the surface.
         *
         * @param shape The shape of the section.
         * @param offset The offset of the section to the previous section.
         * @param count The number of sections to insert.
         */
        void arrange(const Polygon_2 &shape, Vector_3 offset, std::size_t count)
        {
            /* Insert count sections */
            for (int i = 0; i < count; i++)
            {
                this->sections.emplace_back(shape);
                this->sections.back().origin = CGAL::ORIGIN + (offset * i);
            }
        }

      private:
        std::vector<SectionType> sections{}; /**< Internal container which holds the surface sections. */
    };

    /**
     * @class SurfaceBuilder
     * @brief The interface class which defines the methods to build
     * the different types of surfaces.
     */
    class SurfaceBuilder
    {
      public:
        /**
         * @brief Virtual destructor since this class is used as a base class.
         */
        virtual ~SurfaceBuilder() = default;

        /* Build Methods */
        virtual auto build_hull(std::string_view name) -> MultisectionSurface<PolygonSection> = 0;
        virtual auto build_fuselage(std::string_view name) -> MultisectionSurface<PolygonSection> = 0;
        virtual auto build_airfoil_surface(std::string_view name) -> MultisectionSurface<AirfoilSection> = 0;
        virtual auto build_spar(std::string_view name) -> MultisectionSurface<PolygonSection> = 0;
        virtual auto build_control_device(std::string_view name) -> MultisectionSurface<PolygonSection> = 0;
        virtual auto build_wing(std::string_view name) -> MultisectionSurface<AirfoilSection> = 0;
    };

};     // namespace geom2
#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_BUILDER_H_
