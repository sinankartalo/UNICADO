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

#include "aircraftGeometry2/processing/measure.h"
#include "aircraftGeometry2/processing/transform.h"

#include <cmath>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Surface_mesh/IO/PLY.h>
#include <CGAL/Surface_sweep_2_algorithms.h>
#include <CGAL/boost/graph/iterator.h>
#include <CGAL/squared_distance_3.h>
#include <CGAL/centroid.h>

/* === Functions === */
namespace geom2
{
    namespace detail
    {
        template <Shape T>
        auto get_section_index(const std::vector<T> &sections, double position) -> std::optional<size_t>
        {
            /* Check whether the surface is extruded in negative direction */
            const bool is_extruded_negative = sections.front().origin.z() > sections.back().origin.z();

            /* Check whether the first and last section contain the position */
            if (is_extruded_negative)
            {
                /* When the surface is extruded in negative Z direction */
                if (sections.front().origin.z() < position || sections.back().origin.z() > position)
                    return std::nullopt;
            } else {
                /* When the surface is extruded in positive Z direction */
                if (sections.front().origin.z() > position || sections.back().origin.z() < position)
                    return std::nullopt;
            }

            /* Check whether position is the first or last section */
            if (sections.front().origin.z() == position)
            {
                return 0;
            }
            if (sections.back().origin.z() == position)
            {
                return sections.size() - 2;
            }

            /* Find the first section which comes after the position */
            auto section = std::find_if(
                sections.begin(),
                sections.end(), [&](const T &section)
                { return is_extruded_negative ? section.origin.z() < position : section.origin.z() > position; });

            /* Return the index of the section before the position */
            return std::distance(sections.begin(), section) - 1;
        }
        template auto get_section_index<PolygonSection>(const std::vector<PolygonSection> &sections, double position) -> std::optional<size_t>;
        template auto get_section_index<AirfoilSection>(const std::vector<AirfoilSection> &sections, double position) -> std::optional<size_t>;
        /**
         * @brief Project the given position to the other side of the symmetric surface
         * if the current position is in the opposite direction of the surface extrusion.
         * This way the given position always results in a valid range, so that
         * `get_section_index` can be used to get the correct section index.
         * 
         * @param surface The surface to check the extrusion direction.
         * @param position The raw position to adjust.
         * @return double The position projected to the symmetric position if possible.
         */
        [[nodiscard]] inline auto project_symmetric_position(const MultisectionSurface<AirfoilSection> &surface, const double position) -> double
        {
            /* Check whether the surface extrusion and the position are in the same direction */
            const bool is_opposite_direction = std::signbit(surface.sections.back().origin.z()) xor std::signbit(position);

            /* Adjust the position to the symmetric position if the surface is symmetric */
            return is_opposite_direction and surface.is_symmetric ? -position : position;
        }

        /**
         * @brief Interpolate between two points in 3D space.
         * The interpolation refers is linear and refers to the z position.
         * The x and y coordinates are interpolated linearly between the two points.
         * 
         * @param first The first point.
         * @param second The second point.
         * @param z_position The local z position to interpolate.
         * @return Point_3 The interpolated point at the given z position.
         */
        [[nodiscard]] auto interpolate_xy(const Point_3 first, const Point_3 second, const double z_position) -> Point_3
        {
            /* Calculate the distance vector between the two points */
            const Vector_3 distance = second - first;
            const double distance_factor = (z_position - first.z()) / distance.z();

            /* Return the interpolated point */
            return first + distance_factor * distance;
        }

        /**
         * @brief Get the moment of deviation of the provided triangle mesh.
         * @attention This assumes that the mesh is a triangle mesh!
         * 
         * @param mesh The triangle mesh.
         * @param axis The axis of rotation for the moment of deviation.
         * @return double [m^4] The moment of deviation in respect to the provided axis.
         */
        auto moment_of_deviation(Mesh &mesh, const Line_3 axis) -> double
        {
            /* Accumulate the moment of deviation of each mesh face */
            return std::accumulate(
                mesh.faces_begin(),
                mesh.faces_end(),
                0.0,
                [&axis, &mesh](const double deviation, const Mesh::Face_index &face)
                {
                    /* Get the face area */
                    const double area = CGAL::Polygon_mesh_processing::face_area(face, mesh);

                    /* Calculate the centroid of the face */
                    const auto vertices = CGAL::vertices_around_face(mesh.halfedge(face), mesh);
                    std::array<Point_3, 3> points{}; // Each face has 3 vertices since the mesh is a triangle mesh 
                    std::transform(
                        vertices.begin(),
                        vertices.end(),
                        points.begin(),
                        [&mesh](const Mesh::Vertex_index &vertex)
                        { return mesh.point(vertex); });
                    const Point_3 centroid = CGAL::centroid(points.begin(), points.end(), CGAL::Dimension_tag<0>{});

                    /* Get the distance vector of the face centroid to the axis */
                    const Point_3 centroid_projected = axis.projection(centroid);
                    const Vector_3 distance_vector{centroid, centroid_projected};

                    /*
                     * Get the product of all coordinates.
                     * Since the distance vector if orthogonal to the axis,
                     * one of the coordinates is always zero, so copy
                     * the non-zero coordinates.
                     */
                    std::array<double, 2> coordinates{};
                    std::copy_if(
                        distance_vector.cartesian_begin(),
                        distance_vector.cartesian_end(),
                        coordinates.begin(),
                        [](const double coordinate)
                        { return std::abs(coordinate) > 1e-4; });

                    /*
                    * Return the accumulated moment of deviation
                    * => The thickness and density of the face are assumed to be 1.0!
                    */
                    const double d_mass = area * 1.0 * 1.0;
                    return deviation + coordinates.at(0) * coordinates.at(1) * d_mass;
                });
        }

        /**
         * @brief Get the moment of inertia of the provided triangle mesh.
         * @attention This assumes that the mesh is a triangle mesh!
         * 
         * @param mesh The triangle mesh.
         * @param axis The axis of rotation for the moment of inertia.
         * @return double [m^4] The moment of inertia in respect to the provided axis.
         */
        auto moment_of_inertia(Mesh &mesh, const Line_3 axis) -> double
        {
            /* Accumulate the inertia of each mesh face */
            return std::accumulate(
                mesh.faces_begin(),
                mesh.faces_end(),
                0.0,
                [&axis, &mesh](const double inertia, const Mesh::Face_index &face)
                {
                    /* Get the face area */
                    const double area = CGAL::Polygon_mesh_processing::face_area(face, mesh);

                    /* Calculate the centroid of the face */
                    const auto vertices = CGAL::vertices_around_face(mesh.halfedge(face), mesh);
                    std::array<Point_3, 3> points{}; // Each face has 3 vertices since the mesh is a triangle mesh 
                    std::transform(
                        vertices.begin(),
                        vertices.end(),
                        points.begin(),
                        [&mesh](const Mesh::Vertex_index &vertex)
                        { return mesh.point(vertex); });
                    const Point_3 centroid = CGAL::centroid(points.begin(), points.end(), CGAL::Dimension_tag<0>{});

                    /* Calculate the squared distance between the centroid and the axis */
                    const double squared_distance = CGAL::squared_distance(centroid, axis);

                    /*
                    * Return the accumulated inertia
                    * => The thickness and density of the face are assumed to be 1.0!
                    */
                    const double d_mass = area * 1.0 * 1.0;
                    return inertia + squared_distance * d_mass;
                });
        }

        /**
         * @brief Get the inertia tensor of the provided triangle mesh.
         * The mesh can be refined before the inertia is calculated,
         * which is recommended for a more accurate result.
         * 
         * @param mesh The triangle mesh.
         * @param target_edge_length The target edge length of the mesh when refined.
         * @return CGAL::Eigen_matrix<double, 3, 3> The inertia tensor.
         */
        auto calculate_inertia_tensor(Mesh &mesh,  const double target_edge_length) -> CGAL::Eigen_matrix<double, 3, 3>
        {
            /* Get the center of gravity of the mesh */
            std::vector<Point_3> points(mesh.number_of_vertices());
            std::transform(
                mesh.vertices_begin(),
                mesh.vertices_end(),
                points.begin(),
                [&mesh](const Mesh::Vertex_index &vertex)
                { return mesh.point(vertex); });
            const Point_3 center_of_gravity = CGAL::centroid(points.begin(), points.end(), CGAL::Dimension_tag<0>{});

            /* Refine the mesh if desired */
            if ( target_edge_length > 0)
            {
                CGAL::Polygon_mesh_processing::isotropic_remeshing(
                    faces(mesh), target_edge_length, mesh);
            }

            /* The following algorithm assumes a triangle mesh */
            if (not CGAL::is_triangle_mesh(mesh))
            {
                throw std::runtime_error("The surface could not be meshed as a triangle mesh!");
            }

            /* Get the inertia tensor of the mesh */
            CGAL::Eigen_matrix<double, 3, 3> tensor{};
            tensor.set(0, 0, moment_of_inertia(mesh, {center_of_gravity, Vector_3{1, 0, 0}}));
            tensor.set(1, 1, moment_of_inertia(mesh, {center_of_gravity, Vector_3{0, 1, 0}}));
            tensor.set(2, 2, moment_of_inertia(mesh, {center_of_gravity, Vector_3{0, 0, 1}}));
            tensor.set(0, 1, moment_of_deviation(mesh, {center_of_gravity, Vector_3{0, 0, 1}}));
            tensor.set(0, 2, moment_of_deviation(mesh, {center_of_gravity, Vector_3{0, 1, 0}}));
            tensor.set(1, 2, moment_of_deviation(mesh, {center_of_gravity, Vector_3{1, 0, 0}}));
            tensor.set(1, 0, tensor(0, 1));
            tensor.set(2, 0, tensor(0, 2));
            tensor.set(2, 1, tensor(1, 2));

            /* Return the result */
            return tensor;
        }
    }; // namespace detail

    auto measure::area(const MultisectionSurface<PolygonSection> &surface) -> double
    {
        /* Initialize area */
        double area{0.0};

        /* Calculate area if enough segments are available */
        if (surface.sections.size() > 1)
        {
            Mesh mesh = transform::to_mesh(surface.sections);
            area = CGAL::Polygon_mesh_processing::area(mesh);
        }

        /*
         * For the special case when the sections only have
         * 2 vertices each -> The surface is a planar surface.
         * The mesh result would measure the top and bottom are
         * of this surface and result in twice the area of the
         * planar surface.
         * So catch this and adjust according to our expectations.
         */
        const bool is_planar = std::all_of(
                surface.sections.begin(),
                surface.sections.end(),
                [&](const PolygonSection &section)
                { return section.get_contour().size() == 2; });

        /* Return total area */
        return is_planar? area / 2 : area;
    }

    auto measure::area(const MultisectionSurface<AirfoilSection> &surface) -> double
    {
        /* Initialize area */
        double area{0.0};

        /* Calculate area if enough segments are available */
        if (surface.sections.size() > 1)
        {
            Mesh mesh = transform::to_mesh(surface.sections);
            area = CGAL::Polygon_mesh_processing::area(mesh);
        }

        /* Return total area */
        return surface.is_symmetric ? 2.0 * area : area;
    }

    auto measure::aspect_ratio(const MultisectionSurface<AirfoilSection> &surface) -> double
    {
        /* Initialize aspect ratio */
        double aspect_ratio{0.0};

        /* Calculate aspect ratio */
        const double span = measure::span(surface);
        const double area = measure::reference_area(surface);
        if (area > 0.0)
        {
            aspect_ratio = span * span / area;
        }

        /* Return the aspect ratio */
        return aspect_ratio;
    }

    auto measure::bottom(const MultisectionSurface<PolygonSection> &surface, const double position) -> Point_3
    {
        /* Initialize bottom */
        Point_3 bottom{0.0, 0.0, 0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, position);
        if (!index.has_value())
        {
            return bottom;
        }

        /* Lambda to get the bottom point of a section in local coordinates */
        auto bottom_point = [](const PolygonSection &section) -> Point_3
        {
            const auto point = *section.get_contour(true).bottom_vertex();
            return geom2::transform::to_parent(section, point);
        };

        /* Get the bottom points of the sections */
        const auto bottom_first = bottom_point(surface.sections.at(index.value()));
        const auto bottom_second = bottom_point(surface.sections.at(index.value() + 1));

        /* Return linear interpolation of the bottom points */
        return detail::interpolate_xy(bottom_first, bottom_second, position);
    }

    auto measure::center(const MultisectionSurface<PolygonSection> &surface, const double position) -> Point_3
    {
        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, position);
        if (!index.has_value())
        {
            return {0.0, 0.0, 0.0};
        }

        /*
         * Lambda to extract the center of the bounding box of the polygon section
         * and transform it to the parent coordinate system.
         */
        auto bbox_center = [](const PolygonSection &section) -> Point_3
        {
            const auto bbox = section.get_contour(true).bbox();
            return geom2::transform::to_parent(section,
                Point_2{
                (bbox.xmin() + bbox.xmax()) / 2.0,
                (bbox.ymin() + bbox.ymax()) / 2.0});
        };

        /* The first and second center and get their relative distance vector */
        const auto first_center = bbox_center(surface.sections.at(index.value()));
        const auto second_center = bbox_center(surface.sections.at(index.value() + 1));

        /* Return the center by linear interpolation */
        return detail::interpolate_xy(first_center, second_center, position);
    }

    auto measure::centroid(const MultisectionSurface<PolygonSection> &surface) -> Point_3
    {
        /* Get the centroids of each segment */
        auto centroids = measure::centroids(surface);

        /* Calculate the barycenter of the centroids */
        return CGAL::centroid(centroids.begin(), centroids.end(), CGAL::Dimension_tag<0>{});
    }

    auto measure::centroid(const MultisectionSurface<AirfoilSection> &surface) -> Point_3
    {
        /* Get the centroids of each segment */
        auto centroids = measure::centroids(surface);

        /* Calculate the barycenter of the centroids */
        Point_3 center = CGAL::centroid(centroids.begin(), centroids.end(), CGAL::Dimension_tag<0>{});
        if (surface.is_symmetric)
        {
            center = Point_3{center.x(), center.y(), 0.0};
        }

        /* Return the barycenter */
        return center;
    }

    auto measure::centroids(const MultisectionSurface<PolygonSection> &surface) -> std::vector<Point_3>
    {
        /* Initialize the vector containing the centroid points */
        std::vector<Point_3> centroids;
        centroids.reserve(surface.sections.size() - 1);

        /* Lambda to extract the centroid of the polygon segments */
        auto _get_centroid = [](const PolygonSection &first, const PolygonSection &second) -> Point_3
        {
            /* Create the points vector */
            std::vector<Point_3> points;

            /* Extract the points of the first section */
            auto shape = first.get_contour(true);
            std::for_each(
                shape.vertices_begin(),
                shape.vertices_end(),
                [&points, &first](const Point_2 &point)
                { points.emplace_back(transform::to_parent(first, point)); });

            /* Extract the points of the second section */
            shape = second.get_contour(true);
            std::for_each(
                shape.vertices_begin(),
                shape.vertices_end(),
                [&points, &second](const Point_2 &point)
                { points.emplace_back(transform::to_parent(second, point)); });

            /* Calculate the centroid of the points */
            return CGAL::centroid(points.begin(), points.end(), CGAL::Dimension_tag<0>{});
        };

        /* Extract the centroid of each segment */
        for (size_t i = 0; i < surface.sections.size() - 1; ++i)
        {
            /* Get the centroid of the current segment */
            const Point_3 centroid = _get_centroid(surface.sections.at(i), surface.sections.at(i + 1));

            /* Add the centroid to the vector */
            centroids.push_back(centroid);
        }

        /* Return the vector containing the centroid points */
        return centroids;
    }

    auto measure::centroids(const MultisectionSurface<AirfoilSection> &surface) -> std::vector<Point_3>
    {
        /* Initialize the vector containing the centroid points */
        std::vector<Point_3> centroids;
        centroids.reserve(surface.sections.size() - 1);

        /* Lambda to extract the centroid of the airfoil segments */
        auto _get_centroid = [](const AirfoilSection &first, const AirfoilSection &second) -> Point_3
        {
            /* Create the points vector */
            std::vector<Point_3> points;

            /* Extract the points of the sections */
            points.push_back(transform::to_parent(first, *first.get_contour(true).right_vertex()));
            points.push_back(transform::to_parent(first, *first.get_contour(true).left_vertex()));
            points.push_back(transform::to_parent(second, *second.get_contour(true).right_vertex()));
            points.push_back(transform::to_parent(second, *second.get_contour(true).left_vertex()));

            /* Calculate the centroid of the points */
            return CGAL::centroid(points.begin(), points.end(), CGAL::Dimension_tag<0>{});
        };

        /* Extract the centroid of each segment */
        for (size_t i = 0; i < surface.sections.size() - 1; ++i)
        {
            /* Get the centroid of the current segment */
            const Point_3 centroid = _get_centroid(surface.sections.at(i), surface.sections.at(i + 1));

            /* Add the centroid to the vector */
            centroids.push_back(centroid);
        }

        /* Return the vector containing the centroid points */
        return centroids;
    }

    auto measure::chord(const MultisectionSurface<AirfoilSection> &surface, const double position) -> double
    {
        /* Project the position to the symmetric position if the surface is symmetric */
        const double span_position = detail::project_symmetric_position(surface, position);

        /* Initialize chord */
        double chord{0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, span_position);
        if (!index.has_value())
        {
            return chord;
        }

        /* Calculate the local chord by linear interpolation between the two sections */
        const double first_chord = surface.sections.at(index.value()).get_chord_length();
        const double second_chord = surface.sections.at(index.value() + 1).get_chord_length();
        const double first_position = surface.sections.at(index.value()).origin.z();
        const double second_position = surface.sections.at(index.value() + 1).origin.z();
        chord = first_chord + (second_chord - first_chord) * (span_position - first_position) / (second_position - first_position);

        /* Return total chord */
        return chord;
    }

    auto measure::dihedral(const MultisectionSurface<AirfoilSection> &surface, const double position) -> double
    {
        /* Initialize dihedral */
        double dihedral{0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(
            surface.sections, detail::project_symmetric_position(surface, position));
        if (!index.has_value())
        {
            return dihedral;
        }

        /* Get the leading edge points of the two closed sections */
        const Point_3 first_point = surface.sections.at(index.value()).origin;
        const Point_3 second_point = surface.sections.at(index.value() + 1).origin;

        /* Calculate the projected angle of the two points with the local y axis */
        const double dy = second_point.y() - first_point.y();
        const double dz = second_point.z() - first_point.z();
        dihedral = std::atan2(dy, std::abs(dz)); // The y offset defines the sign of the angle

        /* Return total dihedral */
        return dihedral;
    }

    auto measure::height(const MultisectionSurface<PolygonSection> &surface, const double position) -> double
    {
        /* Initialize height */
        double height{0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, position);
        if (!index.has_value())
        {
            return height;
        }

        /* Calculate the local height by linear interpolation between the two sections */
        const double first_height = surface.sections.at(index.value()).get_contour(true).bbox().y_span();
        const double second_height = surface.sections.at(index.value() + 1).get_contour(true).bbox().y_span();
        const double first_position = surface.sections.at(index.value()).origin.z();
        const double second_position = surface.sections.at(index.value() + 1).origin.z();
        height = first_height + (second_height - first_height) * (position - first_position) / (second_position - first_position);

        /* Return total height */
        return height;
    }

    auto measure::height_max(const MultisectionSurface<PolygonSection> &surface) -> double
    {
        /* Find the max element in the surface */
        auto max_height = std::max_element(
            surface.sections.begin(),
            surface.sections.end(),
            [](const PolygonSection &first, const PolygonSection &second)
            { return first.get_contour(true).bbox().y_span() < second.get_contour(true).bbox().y_span(); });

        /* Return the max height if it was found */
        return max_height != surface.sections.end() ? max_height->get_contour(true).bbox().y_span() : 0.0;
    }

    auto measure::inertia(
        const MultisectionSurface<PolygonSection> &surface,
        const double refined_edge_length) -> CGAL::Eigen_matrix<double, 3, 3>
    {
        /* Mesh the surface */
        Mesh mesh = transform::to_mesh(surface);

        /* Measure the inertia of the surface mesh */
        return detail::calculate_inertia_tensor(mesh, refined_edge_length);
    }

    auto measure::inertia(
        const MultisectionSurface<AirfoilSection> &surface,
        const double refined_edge_length) -> CGAL::Eigen_matrix<double, 3, 3>
    {
        /* Mesh the surface */
        Mesh mesh = transform::to_mesh(surface);

        /* Measure the inertia of the surface mesh */
        return detail::calculate_inertia_tensor(mesh, refined_edge_length);
    }

    auto measure::left(const MultisectionSurface<PolygonSection> &surface, const double position) -> Point_3
    {
        /* Initialize left */
        Point_3 left{0.0, 0.0, 0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, position);
        if (!index.has_value())
        {
            return left;
        }

        /* Lambda to get the left point of a section in local coordinates */
        auto left_point = [](const PolygonSection &section) -> Point_3
        {
            const auto point = *section.get_contour(true).left_vertex();
            return geom2::transform::to_parent(section, point);
        };

        /* Get the left points of the sections */
        const auto left_first = left_point(surface.sections.at(index.value()));
        const auto left_second = left_point(surface.sections.at(index.value() + 1));

        /* Return linear interpolation of the left points */
        return detail::interpolate_xy(left_first, left_second, position);
    }

    auto measure::length(const MultisectionSurface<PolygonSection> &surface) -> double
    {
        /* Initialize length */
        double length{0.0};

        /* Calculate length if enough segments are available */
        if (surface.sections.size() > 1)
            length = surface.sections.back().origin.z() - surface.sections.front().origin.z();

        /* Return total length */
        return std::abs(length);
    }

    auto measure::mean_aerodynamic_chord(const MultisectionSurface<AirfoilSection> &surface) -> double
    {
        /* Initialize mean aerodynamic chord */
        double MAC{0.0};

        /* Get the reference area of one wing half if the surface is symmetric */
        const double area = surface.is_symmetric ? measure::reference_area(surface) / 2.0 : measure::reference_area(surface);

        /* Lambda to calculate the MAC of one segment weighted by the reference area */
        auto _get_MAC = [&area](const double taper, const double root_chord, const double area_segment) -> double
        {
            /* Calculate the MAC segment itself */
            const double MAC_segment = (2.0 / 3.0) * root_chord * (1.0 + taper + (taper * taper)) / (1.0 + taper);

            /* Return the weighted MAC segment */
            return MAC_segment * area_segment / area;
        };

        /* Accumulate the MAC of each segment by weighing it with its reference area */
        for (size_t i = 0; i < surface.sections.size() - 1; ++i)
        {
            /* Get the taper ratio and root chord of the current segment */
            const double taper = surface.sections.at(i + 1).get_chord_length() / surface.sections.at(i).get_chord_length();
            const double root_chord = surface.sections.at(i).get_chord_length();
            const double area_segment =
                (surface.sections.at(i).get_chord_length() + surface.sections.at(i + 1).get_chord_length()) *
                std::abs(surface.sections.at(i + 1).origin.z() - surface.sections.at(i).origin.z()) / 2.0;

            /* Calculate the MAC of the current segment */
            MAC += _get_MAC(taper, root_chord, area_segment);
        }

        /* Return the mean aerodynamic chord */
        return MAC;
    }

    auto measure::mean_aerodynamic_chord_position(const MultisectionSurface<AirfoilSection> &surface) -> double
    {
        /* Get the area of one wing half */
        const double area_winghalf = surface.is_symmetric ? measure::reference_area(surface) / 2.0 : measure::reference_area(surface);

        /* Lambda to calculate the MAC position of one segment */
        auto _get_MAC_position = [](const double taper, const double halfspan) -> double
        {
            return (halfspan / 3) * (1 + 2 * taper) / (1 + taper);
        };

        /* Accumulate the span position of each segment MAC by weighing it with its reference area */
        double span_MAC{0.0};
        for (size_t i = 0; i < surface.sections.size() - 1; ++i)
        {
            /* Get the taper ratio and root chord of the current segment */
            const double taper = surface.sections.at(i + 1).get_chord_length() / surface.sections.at(i).get_chord_length();
            const double span_segment = (surface.sections.at(i + 1).origin.z() - surface.sections.at(i).origin.z());
            const double area_segment =
                (surface.sections.at(i).get_chord_length() + surface.sections.at(i + 1).get_chord_length()) *
                std::abs(surface.sections.at(i + 1).origin.z() - surface.sections.at(i).origin.z()) / 2.0;

            /* Calculate the span position of the current segment */
            span_MAC +=
                (surface.sections.at(i).origin.z() + _get_MAC_position(taper, span_segment)) * area_segment / area_winghalf;
        }

        /* Return the mean aerodynamic chord position */
        return span_MAC;
    }

    auto measure::offset_LE(const MultisectionSurface<AirfoilSection> &surface, const double position) -> Point_3
    {
        /* Initialize offset */
        Point_3 offset{0.0, 0.0, 0.0};

        /* Get the index of the section which is before the position */
        const double span_position = detail::project_symmetric_position(surface, position);
        auto index = detail::get_section_index(surface.sections, span_position);
        if (!index.has_value())
        {
            return offset;
        }

        /* Calculate the local offset by linear interpolation between the two sections */
        const Point_3 first_offset = surface.sections.at(index.value()).origin;
        const Point_3 second_offset = surface.sections.at(index.value() + 1).origin;
        const double first_position = surface.sections.at(index.value()).origin.z();
        const double second_position = surface.sections.at(index.value() + 1).origin.z();
        offset = first_offset + (second_offset - first_offset) * (span_position - first_position) / (second_position - first_position);

        /* Return total offset */
        return {offset.x(), offset.y(), position};
    }

    auto measure::reference_area(const MultisectionSurface<AirfoilSection> &surface) -> double
    {
        /* Initialize area */
        double area{0.0};

        /* Calculate area if enough segments are available */
        if (surface.sections.size() > 1)
        {
            /* Initialize the polygon which will represent the reference area */
            Polygon_2 reference_area;

            /* Extract the leading edge points and insert them into the polygon */
            std::for_each(
                surface.sections.begin(),
                surface.sections.end(),
                [&](const AirfoilSection &section)
                { reference_area.push_back(Point_2{section.origin.x(), section.origin.z()}); });

            /* Extract the trailing edge point in reverse order */
            std::for_each(
                surface.sections.rbegin(),
                surface.sections.rend(),
                [&](const AirfoilSection &section)
                {
                    /* Get the 3D point of the trailing edge */
                    const Point_3 TE = transform::to_parent(section, *section.get_contour(true).right_vertex());

                    /* Insert the 2D point into the polygon */
                    reference_area.push_back(Point_2{TE.x(), TE.z()}); });

            /*
             * Check if the resulting polygon is simple, i.e. the airfoil sections
             * are ordered in increasing local Z direction.
             */
            if (reference_area.is_simple())
            {
                /* Calculate the absolute area of the reference area */
                area = std::abs(reference_area.area());
            }
        }

        /* Return total area */
        return surface.is_symmetric ? 2.0 * area : area;
    }

    auto measure::right(const MultisectionSurface<PolygonSection> &surface, const double position) -> Point_3
    {
        /* Initialize right */
        Point_3 right{0.0, 0.0, 0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, position);
        if (!index.has_value())
        {
            return right;
        }

        /* Lambda to get the right point of a section in local coordinates */
        auto right_point = [](const PolygonSection &section) -> Point_3
        {
            const auto point = *section.get_contour(true).right_vertex();
            return geom2::transform::to_parent(section, point);
        };

        /* Get the right points of the sections */
        const auto right_first = right_point(surface.sections.at(index.value()));
        const auto right_second = right_point(surface.sections.at(index.value() + 1));

        /* Return linear interpolation of the right points */
        return detail::interpolate_xy(right_first, right_second, position);
    }

    auto measure::span(const MultisectionSurface<AirfoilSection> &surface) -> double
    {
        /* Initialize span */
        double span{0.0};

        /* Calculate span if enough segments are available */
        if (surface.sections.size() > 1)
            span = surface.sections.back().origin.z() - surface.sections.front().origin.z();

        /* Return total span */
        span = std::abs(span);
        return surface.is_symmetric ? 2 * span : span;
    }

    auto measure::sweep(const MultisectionSurface<AirfoilSection> &surface, const double position, const double chord_offset) -> double
    {
        /* Initialize sweep */
        double sweep{0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(
            surface.sections, detail::project_symmetric_position(surface, position));
        if (!index.has_value())
        {
            return sweep;
        }

        /* Get the points at chord_offset of the sections */
        const AirfoilSection first_section = surface.sections.at(index.value());
        const Point_3 first_point{chord_offset * first_section.get_chord_length() + first_section.origin.x(), 0.0, first_section.origin.z()};
        const AirfoilSection second_section = surface.sections.at(index.value() + 1);
        const Point_3 second_point{chord_offset * second_section.get_chord_length() + second_section.origin.x(), 0.0, second_section.origin.z()};

        /* Calculate the projected angle of the two points with the local z axis */
        const double dx = second_point.x() - first_point.x();
        const double dz = second_point.z() - first_point.z();
        // sweep = std::atan(dx / dz);
        sweep = std::atan2(dx, std::abs(dz)); // The z offset defines the sign of the angle

        /*
         * This function measures the angle in y-direction
         * and not in z-direction as assumed by the std::atan function.
         * Therefore, the sign of the angle has to be corrected.
         */
        sweep *= -1.0; 

        /* Return total sweep */
        return sweep;
    }

    auto measure::taper_ratio(const MultisectionSurface<AirfoilSection> &surface) -> double
    {
        /* Initialize taper ratio */
        double taper_ratio{0.0};

        /* Calculate taper ratio */
        const double root_chord = measure::chord(surface, 0.0);
        const double tip_chord = surface.sections.back().get_chord_length();
        if (root_chord > 0.0)
        {
            taper_ratio = tip_chord / root_chord;
        }

        /* Return the taper ratio */
        return taper_ratio;
    }

    auto measure::thickness(const AirfoilSection &section, const double x_position) -> double
    {
        /* Access the polygon of the airfoil */
        const Polygon_2 polygon = section.get_contour(false);

        /* Get the y distance of top and bottom */
        const auto [top, bottom] = top_and_bottom(polygon, x_position);
        const double y_span = top.y() - bottom.y();

        /* Calculate the distance between the intersection points */
        return y_span * section.get_chord_length() * section.get_thickness_scale();
    }

    auto measure::thickness_max(const AirfoilSection &section) -> double
    {
        /* Loop through the upper half of the polygon points and find the maximum thickness position */
        Polygon_2 polygon = section.get_contour(false);
        auto vertex_max = std::max_element(
            polygon.vertices_begin(),
            polygon.right_vertex(),
            [&section](const Point_2 &lhs, const Point_2 &rhs)
            { return measure::thickness(section, lhs.x()) < measure::thickness(section, rhs.x()); });

        /* Return the maximum thickness at the found position */
        return measure::thickness(section, vertex_max->x());
    }

    auto measure::top(const MultisectionSurface<PolygonSection> &surface, const double position) -> Point_3
    {
        /* Initialize top */
        Point_3 top{0.0, 0.0, 0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, position);
        if (!index.has_value())
        {
            return top;
        }

        /* Lambda to get the top point of a section in local coordinates */
        auto top_point = [](const PolygonSection &section) -> Point_3
        {
            const auto point = *section.get_contour(true).top_vertex();
            return geom2::transform::to_parent(section, point);
        };

        /* Get the top points of the sections */
        const auto top_first = top_point(surface.sections.at(index.value()));
        const auto top_second = top_point(surface.sections.at(index.value() + 1));

        /* Return linear interpolation of the top points */
        return detail::interpolate_xy(top_first, top_second, position);
    }

    auto measure::top_and_bottom(const Polygon_2 &shape, const double x_position) -> std::array<Point_2, 2>
    {
        /* Types for computing the intersection
         *
         * => The compute_intersection_points function requires the
         * Exact_predicates_exact_constructions_kernel for the computation.
         * This kernel is used here locally to not increase the complexity
         * of the other library objects.
         */
        using K = CGAL::Exact_predicates_exact_constructions_kernel;
        using Traits_2 = CGAL::Arr_segment_traits_2<K>;
        using Segment_2 = Traits_2::Curve_2;

        /* Initialize the result */
        double top{0.0};
        double bottom{0.0};

        /* Create segment line which should intersect the airfoil */
        Segment_2 x_line{
            K::Point_2{x_position, 1.1 * shape.bbox().ymax() + 0.1},
            K::Point_2{x_position, 1.1 * shape.bbox().ymin() - 0.1}};

        /* Get the number of polygon edges for preallocating the segments vector */
        const auto n_edges = std::distance(shape.edges_begin(), shape.edges_end());

        /* Create and fill the container with the line elements */
        std::vector<Segment_2> segments(n_edges + 1); // + 1 for the intersecting line
        std::transform(
            shape.edges_begin(),
            shape.edges_end(),
            segments.begin(),
            [](const auto &edge) {
                const auto source = edge.source();
                const auto target = edge.target();
                return Segment_2{
                    K::Point_2{source.x(), source.y()},
                    K::Point_2{target.x(), target.y()}};
            });

        /* Add the intersecting line at the end */
        segments.back() = x_line;

        /* Get the intersection points */
        std::vector<K::Point_2> points;
        CGAL::compute_intersection_points(segments.begin(), segments.end(), std::back_inserter(points));

        /* Check whether the intersection points are valid
         * and get the y coordinates of the top and bottom.
         * The result of the previous computation are
         * reported in an increasing xy-lexicographical order.
         */
        if (points.size() == 2)
        {
            top = CGAL::to_double(points[1].y());
            bottom = CGAL::to_double(points[0].y());
        }

        /* Return the result */
        return {Point_2{x_position, top}, Point_2{x_position, bottom}};
    }

    auto measure::twist(const MultisectionSurface<AirfoilSection> &surface, const double position) -> double
    {
        /* Project the position to the symmetric position if the surface is symmetric */
        const double span_position = detail::project_symmetric_position(surface, position);

        /* Initialize twist */
        double twist{0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, span_position);
        if (!index.has_value())
        {
            return twist;
        }

        /* Calculate the local twist by linear interpolation between the two sections */
        const double first_twist = surface.sections.at(index.value()).get_twist_angle();
        const double second_twist = surface.sections.at(index.value() + 1).get_twist_angle();
        const double first_position = surface.sections.at(index.value()).origin.z();
        const double second_position = surface.sections.at(index.value() + 1).origin.z();
        twist = first_twist + (second_twist - first_twist) * (span_position - first_position) / (second_position - first_position);

        /* Return the interpolated twist */
        return twist;
    }

    auto measure::volume(const MultisectionSurface<PolygonSection> &surface) -> double
    {
        /* Initialize volume */
        double volume{0.0};

        /* Calculate volume if enough segments are available */
        if (surface.sections.size() > 1)
        {
            Mesh mesh = transform::to_mesh(surface.sections);

            /* Close the holes of the first and last section */
            auto border_edge_first = mesh.halfedge(*mesh.vertices_begin());
            auto border_edge_last = mesh.halfedge(*(mesh.vertices_end() - 1));
            CGAL::Polygon_mesh_processing::triangulate_hole(mesh, border_edge_first);
            CGAL::Polygon_mesh_processing::triangulate_hole(mesh, border_edge_last);

            /* Compute the bounding volume */
            volume = CGAL::Polygon_mesh_processing::volume(mesh);
        }

        /* Return total volume */
        return volume;
    }

    auto measure::volume(const MultisectionSurface<AirfoilSection> &surface) -> double
    {
        /* Initialize volume */
        double volume{0.0};

        /* Calculate volume if enough segments are available */
        if (surface.sections.size() > 1)
        {
            Mesh mesh = transform::to_mesh(surface.sections);

            /* Close the holes of the first and last section */
            auto border_edge_first = mesh.halfedge(*mesh.vertices_begin());
            auto border_edge_last = mesh.halfedge(*(mesh.vertices_end() - 1));
            CGAL::Polygon_mesh_processing::triangulate_hole(mesh, border_edge_first);
            CGAL::Polygon_mesh_processing::triangulate_hole(mesh, border_edge_last);

            /* Compute the bounding volume */
            volume = CGAL::Polygon_mesh_processing::volume(mesh);
        }

        /* Return total volume */
        return surface.is_symmetric ? 2.0 * volume : volume;
    }

    auto measure::width(const MultisectionSurface<PolygonSection> &surface, const double position) -> double
    {
        /* Initialize width */
        double width{0.0};

        /* Get the index of the section which is before the position */
        auto index = detail::get_section_index(surface.sections, position);
        if (!index.has_value())
        {
            return width;
        }

        /* Calculate the local width by linear interpolation between the two sections */
        const double first_width = surface.sections.at(index.value()).get_contour(true).bbox().x_span();
        const double second_width = surface.sections.at(index.value() + 1).get_contour(true).bbox().x_span();
        const double first_position = surface.sections.at(index.value()).origin.z();
        const double second_position = surface.sections.at(index.value() + 1).origin.z();
        width = first_width + (second_width - first_width) * (position - first_position) / (second_position - first_position);

        /* Return total width */
        return width;
    }

    auto measure::width_max(const MultisectionSurface<PolygonSection> &surface) -> double
    {
        /* Find the max element in the surface */
        auto max_width = std::max_element(
            surface.sections.begin(),
            surface.sections.end(),
            [](const PolygonSection &first, const PolygonSection &second)
            { return first.get_contour(true).bbox().x_span() < second.get_contour(true).bbox().x_span(); });

        /* Return the max width if it was found */
        return max_width != surface.sections.end() ? max_width->get_contour(true).bbox().x_span() : 0.0;
    }
}; // namespace geom2
