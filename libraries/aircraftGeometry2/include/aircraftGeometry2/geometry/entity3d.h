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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_ENTITY3D_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_ENTITY3D_H_

/* === Includes === */
#include <string>
#include <string_view>
#include <CGAL/Simple_cartesian.h>
#include <numbers>

namespace geom2
{
    /* === Types ===*/
    // using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
    using Kernel = CGAL::Simple_cartesian<double>;
    using Point_2 = Kernel::Point_2;
    using Point_3 = Kernel::Point_3;
    using Direction_3 = Kernel::Direction_3;
    using Vector_3 = Kernel::Vector_3;
    using Line_3 = Kernel::Line_3;
    using Plane_3 = Kernel::Plane_3;
    using Bbox_2 = CGAL::Bbox_2;

    /* === Classes === */
    /**
     * @class Entity3D
     * @brief Defines the base properties of a 3D entity.
     */
    struct Entity3D
    {
        /* Methods */
        /**
         * @brief Default constructs a new Entity 3D object
         */
        Entity3D() = default;

        /**
         * @brief Construct a new Entity 3D object
         *
         * @param origin The origin point of the entity.
         */
        explicit Entity3D(Point_3 origin) : origin(origin) {}

        /**
         * @brief Construct a new Entity 3D object
         *
         * @param name The name of the entity.
         * @param origin The origin point of the entity.
         * @param normal The reference direction of the entity.
         */
        Entity3D(std::string_view name, Point_3 origin, Direction_3 normal)
            : name(name), origin(origin), normal(normal)
        {
        }

        /**
         * @brief Virtual default destructor for the entity since it is a base class.
         */
        virtual ~Entity3D() = default;

        /* Members */
        std::string name{"unknown"};  /**< Name of the entity */
        Point_3 origin{CGAL::ORIGIN}; /**< Origin of the entity */
        Direction_3 normal{0, 0, 1};  /**< Reference direction of the entity */
        double rotation_z{0.0};       /**< Rotation around the local z-axis [rad]*/

        public :
            auto ac_to_global(const Point_3 ac_origin) {
                /* Rotation of the aircraft coordinate system: x points in x,  y points in -z, z points in y,
                origin is at the aircraft xml defined pos --> 90 deg rotation wrt x axis */
                Kernel::Aff_transformation_3 direction_transform_x = Kernel::Aff_transformation_3(
                    1, 0, 0,
                    0, std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0),
                    0, std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0));

                Kernel::Aff_transformation_3 direction_transform_z = Kernel::Aff_transformation_3(
                    std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0), 0,
                    std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0), 0,
                    0, 0, 1);

                Kernel::Aff_transformation_3 direction_transform = direction_transform_x * direction_transform_z;

                /* Translation of the aircraft coordinate system: ac coordinate system has the origin at the reference point*/
                Vector_3 offset{ ac_origin, CGAL::ORIGIN };
                Kernel::Aff_transformation_3 translation = Kernel::Aff_transformation_3(CGAL::TRANSLATION, offset);

                /* Get the resulting transofmration matrix*/
                Kernel::Aff_transformation_3 transform = direction_transform * translation;

                /* Inverse of the global to aircraft coordinate system*/
                transform = transform.inverse();
                direction_transform = direction_transform.inverse();

                /* The normal direction and position of entity is transformed into the global coordinate system */
                origin = transform(origin);
                normal = direction_transform(normal);
            };

            auto global_to_ac(const Point_3 ac_origin) {
                /* Rotation of the aircraft coordinate system: x points in x,  y points in -z, z points in y,
                origin is at the aircraft xml defined pos --> 90 deg rotation wrt x axis */
                Kernel::Aff_transformation_3 direction_transform_x = Kernel::Aff_transformation_3(
                    1, 0, 0,
                    0, std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0),
                    0, std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0));

                Kernel::Aff_transformation_3 direction_transform_z = Kernel::Aff_transformation_3(
                    std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0), 0,
                    std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0), 0,
                    0, 0, 1);

                Kernel::Aff_transformation_3 direction_transform = direction_transform_x * direction_transform_z;

                Vector_3 offset{ ac_origin, CGAL::ORIGIN };
                Kernel::Aff_transformation_3 translation = Kernel::Aff_transformation_3(CGAL::TRANSLATION, offset);

                /* Get the resulting transofmration matrix*/
                Kernel::Aff_transformation_3 transform = direction_transform * translation;

                /* The normal direction and position of entity is transformed into the aircraft coordinate system */
                origin = transform(origin);
                normal = direction_transform(normal);
            };

            auto get_origin_ac_coordinate(const Point_3 ac_origin) const {
                /* Rotation of the aircraft coordinate system: x points in x,  y points in -z, z points in y,
                origin is at the aircraft xml defined pos --> 90 deg rotation wrt x axis */
                Kernel::Aff_transformation_3 direction_transform_x = Kernel::Aff_transformation_3(
                    1, 0, 0,
                    0, std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0),
                    0, std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0));

                Kernel::Aff_transformation_3 direction_transform_z = Kernel::Aff_transformation_3(
                    std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0), 0,
                    std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0), 0,
                    0, 0, 1);

                Kernel::Aff_transformation_3 direction_transform = direction_transform_x * direction_transform_z;

                Vector_3 offset{ ac_origin, CGAL::ORIGIN };
                Kernel::Aff_transformation_3 translation = Kernel::Aff_transformation_3(CGAL::TRANSLATION, offset);

                /* Get the resulting transofmration matrix*/
                Kernel::Aff_transformation_3 transform = direction_transform * translation;

                /* The normal direction and position of entity is transformed into the aircraft coordinate system */
                return transform(origin);
            };

            auto get_direction_ac_coordinate() const {
                /* Rotation of the aircraft coordinate system: x points in x,  y points in -z, z points in y,
                origin is at the aircraft xml defined pos --> 90 deg rotation wrt x axis */
                Kernel::Aff_transformation_3 direction_transform_x = Kernel::Aff_transformation_3(
                    1, 0, 0,
                    0, std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0),
                    0, std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0));

                Kernel::Aff_transformation_3 direction_transform_z = Kernel::Aff_transformation_3(
                    std::cos(std::numbers::pi / 2.0), -std::sin(std::numbers::pi / 2.0), 0,
                    std::sin(std::numbers::pi / 2.0), std::cos(std::numbers::pi / 2.0), 0,
                    0, 0, 1);

                Kernel::Aff_transformation_3 direction_transform = direction_transform_x * direction_transform_z;

                /* The normal direction and position of entity is transformed into the aircraft coordinate system */
                return direction_transform(normal);
            };
    };
}; // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_ENTITY3D_H_
