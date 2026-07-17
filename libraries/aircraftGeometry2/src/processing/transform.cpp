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

#include "aircraftGeometry2/processing/transform.h"
#include "aircraftGeometry2/processing/measure.h"
#include <cmath>
#include <exception>
#include <CGAL/centroid.h>

namespace geom2
{
    namespace detail
    {
        /* === Functions === */
        void add_points_to_mesh(geom2::Mesh &mesh, const Section &section) //NOLINT (runtime/reference)
        {
            /* Get the contour with applied transformation */
            const Polygon_2 poly = section.get_contour(true);

            /* Add all points of the polygons to the 3D mesh */
            for (const auto &vertex : poly.vertices())
            {
                /* Convert to parent coordinate space */
                // 2D to 3D (section to parent coordinate system)
                Point_3 point = geom2::transform::to_parent(section, vertex);
                // 3D to 2D (to global coordinate system)
                // point = geom2::transform::toParent(section, point);

                /* Add the point to the mesh */
                mesh.add_vertex(point);
            }
        }

        void triangulate_inbetween_sections(
            geom2::Mesh &mesh, //NOLINT (runtime/reference)
            const geom2::Mesh::Vertex_iterator start_vertex,
            const std::array<std::size_t, 2> size_of_segments,
            const bool close_sections)
        {
            /** @todo Check whether the mesh actually contains valid vertices */
            /** @todo How to deal with sections which only have 1 or 2 vertices? */
            /* Get the minimum number of points of the two sections */
            const std::size_t number_of_points = std::min(size_of_segments[0], size_of_segments[1]);

            /* Iterators to the points of the first and second section */
            geom2::Mesh::vertex_iterator points_first = start_vertex;
            geom2::Mesh::vertex_iterator points_second = points_first + size_of_segments[0];

            /* Vertex handlers */
            CGAL::SM_Vertex_index u, v, w, x;

            /* Add faces */
            for (std::size_t n = 0; n < number_of_points - 1; ++n)
            {
                u = *(points_first + n);
                v = *(points_first + n + 1);
                w = *(points_second + n);
                x = *(points_second + n + 1);
                mesh.add_face(u, v, w);
                mesh.add_face(w, v, x);
            }

            /* Append extra faces when the two sections have different number of vertices*/
            if (size_of_segments[0] > size_of_segments[1])
            {
                /* Add faces for the remaining points of the first section */
                for (std::size_t n = number_of_points - 1; n < size_of_segments[0] - 1; ++n)
                {
                    u = *(points_first + n);
                    v = *(points_first + n + 1);
                    w = *(points_second + number_of_points - 1);
                    mesh.add_face(u, v, w);
                }
            } else if (size_of_segments[0] < size_of_segments[1]) {
                /* Add faces for the remaining points of the second section */
                for (std::size_t n = number_of_points - 1; n < size_of_segments[1] - 1; ++n)
                {
                    u = *(points_first + number_of_points - 1);
                    v = *(points_second + n + 1);
                    w = *(points_second + n);
                    mesh.add_face(u, v, w);
                }
            }

            /* Append the last set of faces which wrap around to the beginning */
            if (close_sections)
            {
                u = *(points_first + size_of_segments[0] - 1);
                v = *(points_first);
                w = *(points_second + size_of_segments[1] - 1);
                x = *(points_second);
                mesh.add_face(u, v, w);
                mesh.add_face(w, v, x);
            }
        }

        /**
         * @brief Create transformation to rotate around x with angle alpha.
         * @param alpha The angle to rotate around x. [rad]
         * @return Kernel::Aff_transformation_3 The transformation matrix.
         */
        auto create_rotation_x(const double alpha) -> Kernel::Aff_transformation_3
        {
            /*
            The Rotation matrix looks like:
            | 1   0   0 |
            | 0 cos -sin|
            | 0 sin  cos|
            */

            /* Get the sin and cos components */
            const double cos_alpha = std::cos(alpha);
            const double sin_alpha = std::sin(alpha);

            /* Return the transformation matrix */
            return Kernel::Aff_transformation_3(
                1, 0, 0,
                0, cos_alpha, -sin_alpha,
                0, sin_alpha, cos_alpha);
        }

        /**
         * @brief Create transformation to rotate around y with angle alpha.
         * @param alpha The angle to rotate around y. [rad]
         * @return Kernel::Aff_transformation_3 The transformation matrix.
         */
        auto create_rotation_y(const double alpha) -> Kernel::Aff_transformation_3
        {
            /*
            The Rotation matrix looks like:
            | cos  0 sin|
            |   0  1   0|
            |-sin  0 cos|
            */

            /* Get the sin and cos components */
            const double cos_alpha = std::cos(alpha);
            const double sin_alpha = std::sin(alpha);

            /* Return the transformation matrix */
            return Kernel::Aff_transformation_3(
                cos_alpha, 0, sin_alpha,
                0, 1, 0,
                -sin_alpha, 0, cos_alpha);
        }

        /**
         * @brief Create transformation to rotate around z with angle alpha.
         * @param alpha The angle to rotate around z. [rad]
         * @return Kernel::Aff_transformation_3 The transformation matrix.
         */
        auto create_rotation_z(const double alpha) -> Kernel::Aff_transformation_3
        {
            /*
            The Rotation matrix looks like:
            | cos -sin  0|
            | sin  cos  0|
            |   0   0   1|
            */

            /* Get the sin and cos components */
            const double cos_alpha = std::cos(alpha);
            const double sin_alpha = std::sin(alpha);

            /* Return the transformation matrix */
            return Kernel::Aff_transformation_3(
                cos_alpha, -sin_alpha, 0,
                sin_alpha, cos_alpha, 0,
                0, 0, 1);
        }
        auto get_transform(const Entity3D &entity, const Point_2 &point) -> CGAL::Aff_transformation_3<Kernel>
        {
            /* Get the distance of the origin of the entity */
            Vector_3 offset{CGAL::ORIGIN, entity.origin};

            /* Project the normal into the YZ plane */
            Vector_3 normal_projected{0.0, entity.normal.dy(), entity.normal.dz()};

            /* Get the Euler angle beta between entity normal and global Z */
            Vector_3 z_axis{0, 0, 1};
            double beta = CGAL::approximate_angle(normal_projected, z_axis) * to_radians;

            /* Get the sign of the angle */
            if (entity.normal.dy() < 0)
            {
                beta *= -1.0;
            }

            /* Create the transform to rotate around x */
            Kernel::Aff_transformation_3 rotation = detail::create_rotation_x(-beta);

            /* Get the translation from the origin to the parent origin */
            Kernel::Aff_transformation_3 translation = Kernel::Aff_transformation_3(CGAL::TRANSLATION, offset);

            /* return the transformation matrix */
            return translation * rotation;
        }

        auto get_transform(const Entity3D &entity, const Point_3 &point) -> CGAL::Aff_transformation_3<Kernel>
        {
            /* Get the distance of the origin of the entity */
            Vector_3 offset{CGAL::ORIGIN, entity.origin};

            /* Project the normal into the XY and YZ plane */
            Vector_3 normal_xy{entity.normal.dx(), entity.normal.dy(), 0.0};
            // Vector_3 normal_yz{0.0, entity.normal.dy(), CGAL::abs(entity.normal.dz())};

            /* Get the Euler angle beta between entity normal and global Z */
            Vector_3 z_axis{0, 0, 1};
            Vector_3 y_axis{0, 1, 0};
            double alpha = CGAL::approximate_angle(normal_xy, y_axis) * to_radians;
            double beta = CGAL::approximate_angle(entity.normal.vector(), z_axis) * to_radians;

            /* Get the sign of the angles */
            if (entity.normal.dy() > 0)
            {
                beta *= -1.0;
            }
            if (entity.normal.dy() < 0)
            {
                alpha -= std::numbers::pi;
            }
            if (entity.normal.dx() < 0)
            {
                beta *= -1.0;
            }

            /* Create the transform to rotate back around x */
            Kernel::Aff_transformation_3 rotation_beta = detail::create_rotation_x(beta);

            /* Create the transform to rotate back around y */
            Kernel::Aff_transformation_3 rotation_alpha = detail::create_rotation_z(alpha);

            /* Create the transform to rotate around z */
            Kernel::Aff_transformation_3 rotation_gamma = detail::create_rotation_z(entity.rotation_z);

            /* Get the translation from the origin to the parent origin */
            Kernel::Aff_transformation_3 translation = Kernel::Aff_transformation_3(CGAL::TRANSLATION, offset);

            /* Return the transformation matrix */
            return translation * rotation_alpha * rotation_beta * rotation_gamma;
        }

        /**
         * @brief Create a vector of equidistant points along a segment.
         * 
         * @param segment The segment to create the points along.
         * @param count The number of points to create in between the endpoints of the segment.
         * @return std::vector<geom2::Point_2> Vector with the equidistant points along the segment.
         */
        auto midpoint_n(const Kernel::Segment_2 &segment, std::size_t count) -> std::vector<Point_2>
        {
            /* Create vector with count size */
            std::vector<Point_2> points;
            points.reserve(count);

            /* Create n equidistant points along segment */
            for (std::size_t i = 0; i < count; ++i)
            {
                /* Interpolate X and Y coordinate */
                const double x = segment.source().x() + (segment.target().x() - segment.source().x()) * (i + 1) / (count + 1);
                const double y = segment.source().y() + (segment.target().y() - segment.source().y()) * (i + 1) / (count + 1);
                points.emplace_back(Point_2{x, y});
            }

            /* Return the vector with the points */
            return points;
        }
    } // namespace detail

    auto transform::to_parent(const Entity3D &entity, const Point_2 &point) -> Point_3
    {
        /* Get the transformation matrix */
        auto transform = detail::get_transform(entity, point);

        /* Apply the transformations to the point */
        Point_3 point_transformed(point.x(), point.y(), 0);

        /* Transform the point to the parent coordinate system */
        return transform(point_transformed);
    }

    auto transform::to_parent(const Entity3D &entity, const Point_3 &point) -> Point_3
    {
        /* Get the transformation matrix */
        auto transform = detail::get_transform(entity, point);

        /* Transform the point to the parent coordinate system */
        return transform(point);
    }

    auto transform::to_local(const Entity3D &entity, const Point_3 &point) -> Point_3
    {
        /* Get the transformation matrix */
        auto transform = detail::get_transform(entity, point);

        /* Invert the transformation matrix */
        transform = transform.inverse();

        /* Transform the point to the local coordinate system */
        return transform(point);
    }

    auto transform::get_reflection_point_top(
        const Section &section,
        const Entity3D &parent,
        const Direction_3 &direction) -> Point_3
    {
        /* Get the outline of the section */
        const Polygon_2 outline = section.get_contour(true);

        /* Create a vector of 3D Points with the size of the polygon */
        std::vector<Point_3> points{};
        points.reserve(outline.size());

        /* Convert all polygon points to 3D and insert into the vector */
        for (const auto &point : outline.vertices())
        {
            points.emplace_back(to_parent(parent, to_parent(section, point)));
        }

        /* Get the centroid of the 3D points */
        const Point_3 center = CGAL::centroid(points.begin(), points.end(), CGAL::Dimension_tag<0>());

        /* Get the plane which is defined by the parent normal and the view direction */
        using Plane_3 = Kernel::Plane_3;
        const Vector_3 plane_normal = CGAL::cross_product(direction.vector(), parent.normal.vector());
        const Plane_3 plane{center, -plane_normal}; // The cross product returns the opposite direction!
        /**
         * @todo Is the negative plane normal due to a wrong expectation
         * how the view direction should be defined?
         */

        /* Get the point which has the maximum positive distance from this plane */
        const Point_3 top_point = *std::max_element(
            points.begin(), points.end(),
            [&plane](const Point_3 &a, const Point_3 &b)
            {
                double coeff_a = 1;
                double coeff_b = 1;
                if (plane.a() * a.x() + plane.b() * a.y() + plane.c() * a.z() + plane.d() < 0) { coeff_a = -1; };
                if (plane.a() * b.x() + plane.b() * b.y() + plane.c() * b.z() + plane.d() < 0) { coeff_b = -1; };
                /* Check whether a has a smaller distance to the plane than b */
                bool a_is_less_than_b = coeff_a * CGAL::squared_distance(a, plane) < coeff_b * CGAL::squared_distance(b, plane);

                return a_is_less_than_b;
            });

        /* Return the top point */
        return top_point;
    }

    auto transform::get_reflection_point_bottom(
        const Section &section,
        const Entity3D &parent,
        const Direction_3 &direction) -> Point_3
    {
        /* Get the outline of the section */
        const Polygon_2 outline = section.get_contour(true);

        /* Create a vector of 3D Points with the size of the polygon */
        std::vector<Point_3> points{};
        points.reserve(outline.size());

        /* Convert all polygon points to 3D and insert into the vector */
        for (const auto &point : outline.vertices())
        {
            points.emplace_back(to_parent(parent, to_parent(section, point)));
        }

        /* Get the centroid of the 3D points */
        const Point_3 center = CGAL::centroid(points.begin(), points.end(), CGAL::Dimension_tag<0>());

        /* Get the plane which is defined by the parent normal and the view direction */
        using Plane_3 = Kernel::Plane_3;
        const Vector_3 plane_normal = CGAL::cross_product(direction.vector(), parent.normal.vector());
        const Plane_3 plane{center, -plane_normal};
        /**
         * @todo Is the negative plane normal due to a wrong expectation
         * how the view direction should be defined?
         */

        /* Get the point which has the maximum negative distance from this plane */
        const Point_3 bottom_point = *std::max_element(
            points.begin(), points.end(),
            [&plane](const Point_3 &a, const Point_3 &b)
            {
                double coeff_a = 1;
                double coeff_b = 1;
                if (plane.a() * a.x() + plane.b() * a.y() + plane.c() * a.z() + plane.d() < 0) { coeff_a = -1; };
                if (plane.a() * b.x() + plane.b() * b.y() + plane.c() * b.z() + plane.d() < 0) { coeff_b = -1; };
                /* Check whether a has a greater distance to the plane than b */
                bool a_is_less_than_b = coeff_a * CGAL::squared_distance(a, plane) > coeff_b * CGAL::squared_distance(b, plane);

                return a_is_less_than_b;
            });

        /* Return the top point */
        return bottom_point;
    }

    auto transform::to_absolute(
        const MultisectionSurface<PolygonSection> &surface,
        const MultisectionSurface<AirfoilSection> &reference) -> MultisectionSurface<PolygonSection>
    {
        /* Get the span reference dimension of only one wing half */
        const double span_reference = reference.is_symmetric ?
            measure::span(reference) / 2 : measure::span(reference);

        /* Create a new surface with same base properties as the normalized surface */
        /** 
         * @todo How to deal with the normal and origin here? Should we assume the factory
         * sets this correctly after the aircraft XML is refactored and includes
         * both information? Or should this function always copy the normal and origin
         * from the reference surface?
         */
        MultisectionSurface<PolygonSection> surface_absolute{};
        surface_absolute.origin = surface.origin;
        surface_absolute.normal = surface.normal;

        /* Iterate over all sections */
        /**
         * @todo Since this function is mainly intended to transform control devices,
         * does it make sense to include a check here whether the resulting surface
         * is actually extruded along a kink of the reference surface? As a result,
         * this function would need to insert additional sections in the resulting
         * surface to accommodate for the kink(s).
         */
        for (const auto &section : surface.sections)
        {
            /* Get the absolute span wise position of the section */
            const double z_absolute = section.origin.z() * span_reference;

            /* Get the chord length at the current position of the section */
            const double chord_local = measure::chord(reference, z_absolute);

            /* Get the leading edge offset at the local Z position */
            const auto offset_LE = measure::offset_LE(reference, z_absolute);

            /* Create a polygon with the absolute coordinates in chord direction */
            Polygon_2 polygon_absolute{};
            polygon_absolute.push_back(
                Point_2{
                    section.get_contour(true).bbox().xmin() * chord_local + offset_LE.x(),
                    0.0});
            polygon_absolute.push_back(
                Point_2{
                    section.get_contour(true).bbox().xmax() * chord_local + offset_LE.x(),
                    0.0});
            surface_absolute.sections.emplace_back(polygon_absolute);

            /* Set the absolute origin in Z direction */
            surface_absolute.sections.back().origin = Point_3{0.0, 0.0, z_absolute};
        }

        /* Return the absolute surface */
        return surface_absolute;
    }

    auto transform::resample(const Polygon_2& polygon, const std::size_t size) -> Polygon_2
    {
        /* Throw when the size would down-sample the polygon */
        if (size < polygon.size())
        {
            throw std::invalid_argument("geom2::transform::resample(): The polygon cannot be down sampled!");
        }

        /* Return the initial polygon when it has just one vertex */
        if (polygon.size() < 2)
        {
            return polygon;
        }

        /* Get the number of points to insert */
        const auto n_points = static_cast<double>(size - polygon.size());

        /* Compute the normalized interval where points should be inserted */
        const double point_rate = (polygon.size() - 1) / n_points;

        /* Initialize the output polygon with the requested size */
        Polygon_2 polygon_resampled{};
        polygon_resampled.resize(size);

        /* Loop through the number of points and interpolate if necessary */
        std::size_t index_offset{0};
        double next_sample = (1.0 * point_rate);
        for (std::size_t index = 0; index < polygon.size(); ++index)
        {
            /* Insert the existing points */
            polygon_resampled[index + index_offset] = polygon[index];

            /* 
             * When the point rate at which the vertices are resampled
             * allows for additional points, insert them.
             */
            if (next_sample <= (index + 1) && index_offset < n_points && index < polygon.size() - 1)
            {
                /* Compute the points to be inserted */
                Kernel::Segment_2 segment{polygon[index], polygon[index + 1]};
                auto points = detail::midpoint_n(segment, std::floor((index + 1) / point_rate) - index_offset);

                /* Insert all the points and update the next sample position */
                for (const auto &point : points)
                {
                    polygon_resampled[index + (++index_offset)] = point;
                    next_sample += point_rate;
                }
            }
        }

        /* Return the resampled polygon */
        return polygon_resampled;
    }
}; // namespace geom2
