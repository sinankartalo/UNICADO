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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_PROCESSING_TRANSFORM_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_PROCESSING_TRANSFORM_H_

/* === Includes === */
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <array>
#include <numbers>
#include <iterator>
#include <utility>
#include <vector>

#include <CGAL/Surface_mesh.h>

#include "aircraftGeometry2/geometry/entity3d.h"
#include "aircraftGeometry2/geometry/section.h"
#include "aircraftGeometry2/geometry/surface.h"

namespace geom2
{
    /* === Typedefs === */
    using Mesh = CGAL::Surface_mesh<Point_3>;

    namespace detail
    {
        /* === Constants === */
        /** @brief Convert from degrees to radians */
        constexpr double to_radians = std::numbers::pi / 180.0;

        /** @brief Convert from radians to degrees */
        constexpr double to_degrees = 180.0 / std::numbers::pi;

        /* === Functions === */
        /**
         * @brief Adds all points of a polygon section to a mesh.
         * The points get transformed from the 2D coordinate system
         * of the polygon to the parent 3D coordinate system of the
         * surface mesh.
         *
         * @param mesh The mesh to add the points to.
         * @param section The polygon section to add the points from.
         */
        void add_points_to_mesh(geom2::Mesh &mesh, const Section &section); //NOLINT (runtime/references)

        /**
         * @brief Add triangulated faces to the surface mesh which are defined
         * by already inserted points of two sections.
         *
         * @param mesh The mesh to add the faces to.
         * @param start_vertex The iterator to the first vertex of the first section.
         * @param size_of_segments Array containing the number of points of the two sections.
         * @param close_sections Whether the sections should be closed by adding faces.
         */
        void triangulate_inbetween_sections(
            geom2::Mesh &mesh, //NOLINT (runtime/references)
            geom2::Mesh::Vertex_iterator start_vertex,
            std::array<std::size_t, 2> size_of_segments,
            bool close_sections = false);

        /**
         * @brief Get the transformation matrix which converts the 2D point
         * to the 3D coordinate system of the parent entity.
         *
         * @param entity The entity which defines the parent coordinate system.
         * @param point The 2D point in the child coordinate system.
         * @return CGAL::Aff_transformation_3<Kernel> The transformation matrix.
         */
        [[nodiscard]] auto get_transform(const Entity3D &entity, const Point_2 &point) -> CGAL::Aff_transformation_3<Kernel>;

        /**
         * @brief Get the transformation matrix which converts the 3D point
         * to the 3D coordinate system of the parent entity. You can apply
         * an optional rotation around the local z-axis.
         *
         * @param entity The entity which defines the parent coordinate system.
         * @param point The 3D point in the child coordinate system.
         * @return CGAL::Aff_transformation_3<Kernel> The transformation matrix.
         */
        [[nodiscard]] auto get_transform(const Entity3D &entity, const Point_3 &point) -> CGAL::Aff_transformation_3<Kernel>;

        [[nodiscard]] auto create_rotation_x(const double alpha) -> Kernel::Aff_transformation_3;

        [[nodiscard]] auto create_rotation_y(const double alpha) -> Kernel::Aff_transformation_3;

        [[nodiscard]] auto create_rotation_z(const double alpha) -> Kernel::Aff_transformation_3;
    }; // namespace detail

    namespace transform
    {
        /* === Functions === */
        /**
         * @brief Transforms a 2D point to the 3D coordinate system of the parent entity.
         * @note This function is not part of the Python binding.
         *
         * @param entity The entity which defines the parent coordinate system.
         * @param point The 2D point in the child coordinate system.
         * @return geom2::Point_3 Returns the 2D point converted to the parent coordinate system.
         */
        [[nodiscard]] auto to_parent(const Entity3D &entity, const Point_2 &point) -> Point_3;

        /**
         * @brief Transforms a 3D point to the 3D coordinate system of the parent entity.
         * @note This function is not part of the Python binding.
         *
         * @param entity The entity which defines the parent coordinate system.
         * @param point The 2D point in the child coordinate system.
         * @return geom2::Point_3 Returns the 3D point in the parent coordinate system.
         */
        [[nodiscard]] auto to_parent(const Entity3D &entity, const Point_3 &point) -> Point_3;

        /**
         * @brief Transforms a global 3D point to the local coordinate system of the entity.
         * @note This function is not part of the Python binding.
         * 
         * @param entity The entity which defines the local coordinate system.
         * @param point The global 3D point to transform.
         * @return Point_3 The 3D point in the local coordinate system.
         */
        [[nodiscard]] auto to_local(const Entity3D &entity, const Point_3 &point) -> Point_3;

        /**
         * @brief Get the top reflection point of a section when creating the outline in a specific
         * projection direction.
         * @note This function is not part of the Python binding and is only used internally.
         *
         * @param section The section to get the top point from.
         * @param parent The parent entity of the section which defines the orientation in the global space.
         * @param direction The projection direction.
         * @return geom2::Point_3 The top point of the section which is part of the projected outline.
         */
        [[nodiscard]] auto get_reflection_point_top(
            const Section &section,
            const Entity3D &parent,
            const Direction_3 &direction) -> Point_3;

        /**
         * @brief Get the bottom reflection point of a section when creating the outline in a specific
         * projection direction.
         * @note This function is not part of the Python binding and is only used internally.
         *
         * @param section The section to get the bottom point from.
         * @param parent The parent entity of the section which defines the orientation in the global space.
         * @param direction The projection direction.
         * @return geom2::Point_3 The bottom point of the section which is part of the projected outline.
         */
        [[nodiscard]] auto get_reflection_point_bottom(
            const Section &section,
            const Entity3D &parent,
            const Direction_3 &direction) -> Point_3;

        /**
         * @brief Get the outline of a surface when viewed from the given direction.
         * @note The outline points are inserted in series, where the first half of the points
         * are the top points and the second half are the bottom points. The bottom points
         * are inserted in reverse order so that the order of points can directly be used
         * for plotting an SVG path.
         *
         * @param surface The surface to get the outline from.
         * @param direction The direction to view the surface from.
         * @return std::vector<geom2::Point_3> The outline of the surface.
         */
        template <Shape SectionType>
        [[nodiscard]] auto outline_3d(
            const MultisectionSurface<SectionType> &surface,
            const Direction_3 &direction) -> std::vector<Point_3>
        {
            /* Create a vector which can contain all the outline points */
            std::vector<Point_3> outline;
            outline.reserve(surface.sections.size() * 2);

            /* Get all the top reflection points */
            std::transform(
                surface.sections.begin(), surface.sections.end(),
                std::back_inserter(outline),
                [&surface, &direction](const auto &section)
                {
                    return get_reflection_point_top(section, surface, direction);
                });

            /* Get all the bottom reflection points in reverse order */
            std::transform(
                surface.sections.rbegin(), surface.sections.rend(),
                std::back_inserter(outline),
                [&surface, &direction](const auto &section)
                {
                    return get_reflection_point_bottom(section, surface, direction);
                });

            /* Return the outline */
            return outline;
        }

        /**
         * @brief Get the outline of a surface when viewed from the given direction and
         * project the outline points to the 2D plane defined by the projection direction.
         * @note The outline points are inserted in series, where the first half of the points
         * are the top points and the second half are the bottom points. The bottom points
         * are inserted in reverse order so that the order of points can directly be used
         * for plotting an SVG path.
         *
         * @param surface The surface to get the outline from.
         * @param direction The direction to view the surface from.
         * @return std::vector<geom2::Point_2> The outline of the surface.
         */
        template <Shape SectionType>
        [[nodiscard]] auto outline_2d(
            const MultisectionSurface<SectionType> &surface,
            const Direction_3 &direction) -> std::vector<Point_2>
        {
            /* Get the 3d outline of the surface */
            const auto outline = outline_3d(surface, direction);

            /* Create container for the 2D outline with the same size as the outline */
            std::vector<Point_2> outline_2d;
            outline_2d.reserve(outline.size());

            /* Get the plane defined by the view direction */
            const Plane_3 plane_xy{CGAL::ORIGIN, direction};
            const Plane_3 plane_yz{CGAL::ORIGIN, plane_xy.base1()};
            const Plane_3 plane_xz{CGAL::ORIGIN, plane_xy.base2()};

            /* Get the distance of the outline points to the view plane base vectors */
            std::transform(
                outline.begin(), outline.end(),
                std::back_inserter(outline_2d),
                [&plane_yz, &plane_xz](const auto &point)
                {
                    /* Get the distance of the point to the yz plane which gives the x coordinate */
                    double sign_x = 1;
                    if (plane_yz.a() * point.x() + plane_yz.b() * point.y() + plane_yz.c() * point.z() + plane_yz.d() < 0) { sign_x = -1; };
                    const double distance_x = sign_x * std::sqrt(CGAL::squared_distance(plane_yz, point));

                    /* Get the distance of the point to the xz plane which gives the y coordinate */
                    double sign_y = 1;
                    if (plane_xz.a() * point.x() + plane_xz.b() * point.y() + plane_xz.c() * point.z() + plane_xz.d() < 0) { sign_y = -1; };
                    const double distance_y = sign_y * std::sqrt(CGAL::squared_distance(plane_xz, point));

                    /* Return the 2D point */
                    return Point_2(distance_x, distance_y);
                });

            /* Return the outline */
            return outline_2d;
        }

        /**
         * @brief Transform the surface which is defined with normalized
         * coordinates to absolute coordinates using the reference surface
         * to derive the absolute dimensions.
         * @note This is mainly intended to convert the surface of control
         * devices, which are usually defined with normalized coordinates, to absolute
         * coordinates so their outline can be plotted.
         * 
         * @param surface The surface specified with normalized coordinates.
         * @param reference The reference surface to derive the absolute dimensions from.
         * @return MultisectionSurface<PolygonSection> Return a new surface with absolute coordinates.
         */
        auto to_absolute(
            const MultisectionSurface<PolygonSection> &surface,
            const MultisectionSurface<AirfoilSection> &reference) -> MultisectionSurface<PolygonSection>;

        /**
         * @brief Resample a polygon to a given number of points.
         * 
         * @param polygon The polygon to resample.
         * @param size The number of vertices the resampled polygon should have.
         * @return geom2::Polygon_2 The resampled polygon.
         * @throws std::invalid_argument If the requested size is smaller than the polygon size.
         */
        auto resample(const Polygon_2& polygon, std::size_t size) -> Polygon_2;

        /**
         * @brief Creates a triangulated surface mesh from a vector of surface sections.
         * @note This function is not part of the Python binding.
         *
         * @tparam SectionType The type of the surface sections.
         * @param sections The vector containing the surface sections.
         * @return geom2::Mesh Returns the triangulated surface mesh.
         */
        template <Shape SectionType>
        [[nodiscard]] auto to_mesh(const std::vector<SectionType> &sections) -> Mesh
        {
            /* Check whether the SectionType is derived from the Section class */
            static_assert(std::is_base_of_v<Section, SectionType>, "SectionType must be derived from Section!");

            /* Create a new mesh */
            Mesh mesh;

            /** @todo Add orientation check of the polygon sections here! */

            /* Add all points of the polygons to the 3D mesh */
            std::for_each(
                sections.begin(), sections.end(),
                [&mesh](const auto &section)
                {
                    detail::add_points_to_mesh(mesh, section);
                });

            /* Connect all faces for each section */
            geom2::Mesh::Vertex_iterator first_vertex = mesh.vertices_begin();
            for (std::size_t iSegment = 0; iSegment < sections.size() - 1; ++iSegment)
            {
                /* Get the number of points of the two sections */
                const std::array<std::size_t, 2> size_of_segments = {
                    sections[iSegment].size(),
                    sections[iSegment + 1].size()};

                /* Check whether both polygons are simple */
                const bool close_sections = 
                    sections[iSegment].get_contour().is_simple() &&
                    sections[iSegment + 1].get_contour().is_simple();

                /* Triangulate the faces between the two sections */
                detail::triangulate_inbetween_sections(mesh, first_vertex, size_of_segments, close_sections);

                /* Move the iterator to the first vertex of the next section */
                first_vertex = std::next(first_vertex, size_of_segments[0]);
            }

            /* Return the mesh */
            return mesh;
        }

        /**
         * @brief Creates a triangulated surface mesh from a multi-section surface.
         * When the section to not have a uniform size, the sections are resampled
         * to fit the number of vertices of the largest section.
         * @note This function is not part of the Python binding.
         *
         * @tparam SectionType The type of the surface sections.
         * @param surface The multi-section surface to triangulate.
         * @return geom2::Mesh Returns the triangulated surface mesh.
         */
        template <class SectionType>
        [[nodiscard]] auto to_mesh(const MultisectionSurface<SectionType> &surface) -> Mesh
        {
            /* Check whether the SectionType is derived from the Section class */
            static_assert(std::is_base_of_v<Section, SectionType>, "SectionType must be derived from Section!");

            /* Copy the sections, since they might get resampled */
            std::vector<SectionType> sections = surface.sections;

            /* Get the min and max size of the sections */
            const auto [min_size, max_size] = std::minmax_element(
                surface.sections.begin(), surface.sections.end(),
                [](const auto &lhs, const auto &rhs)
                {
                    return lhs.size() < rhs.size();
                });

            /* If the sizes are different, resample the polygons to fit the max size */
            if (min_size->size() != max_size->size())
            {
                /* Resample all sections to the max size */
                std::for_each(
                    sections.begin(), sections.end(),
                    [&max_size](auto &section)
                    {
                        section.set_contour(resample(section.get_contour(false), max_size->size()));
                    });
            }

            /* The mesh in the local coordinate system */
            auto mesh = to_mesh(sections);

            /* Transform to the parent coordinate system */
            const auto transform = detail::get_transform(surface, Point_3(0, 0, 0));
            for (auto vertex : mesh.vertices())
            {
                mesh.point(vertex) = transform(mesh.point(vertex));
            }

            /*  Return the mesh */
            return mesh;
        }
    }; // namespace transform
};     // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_PROCESSING_TRANSFORM_H_
