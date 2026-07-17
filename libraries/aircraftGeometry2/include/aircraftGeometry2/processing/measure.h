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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_PROCESSING_MEASURE_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_PROCESSING_MEASURE_H_

/* === Includes === */
#include <array>
#include <optional>
#include <vector>
#include <CGAL/Eigen_matrix.h>
#include "aircraftGeometry2/geometry/entity3d.h"
#include "aircraftGeometry2/geometry/section.h"
#include "aircraftGeometry2/geometry/surface.h"

namespace geom2
{
    namespace detail
    {
        /**
         * @brief Returns the index of the last section which is located before the given position.
         *
         * @tparam T The shape type of the sections.
         * @param sections Vector containing the sections.
         * @param position The position in local Z direction where the section index should be determined.
         * @return std::optional<size_t> The index of the section.
         * If the position is not located between the first and last section `std::nullopt` is returned.
         */
        template <Shape T>
        auto get_section_index(const std::vector<T> &sections, double position) -> std::optional<size_t>;
    }; // namespace detail

    namespace measure
    {
        /**
         * @brief Calculates the wetted surface area of a polygon section surface.
         * @note If the sections are not closed, this function will close them
         * by connecting the last and first point of each section. Therefore,
         * the resulting surface will be water-tight regardless of the input!
         * However, this is not the case when all of the sections have two vertices each.
         * In this case, we assume a planar surface and calculate the area accordingly.
         * @attention This calculation is costly!
         *
         * @param surface The surface to calculate the area of.
         * @return double The wetted surface area of the surface.
         */
        [[nodiscard]] auto area(const MultisectionSurface<PolygonSection> &surface) -> double;

        /**
         * @brief Calculates the wetted surface area of a airfoil section surface.
         * @note If the sections are not closed, this function will close them
         * by connecting the last and first point of each section. Therefore,
         * the resulting surface will be water-tight regardless of the input!
         * @attention This calculation is costly!
         *
         * @param surface The surface to calculate the area of.
         * @return double The wetted surface area of the surface.
         */
        [[nodiscard]] auto area(const MultisectionSurface<AirfoilSection> &surface) -> double;

        /**
         * @brief Measure the aspect ratio of the surface defined by the multi-section surface.
         * @attention This functions respects the `is_symmetric` flag of the multi-section surface
         * and calculates the aspect ratio accordingly!
         * 
         * @param surface The multi-section surface to measure the aspect ratio of.
         * @return double The aspect ratio of the surface.
         */
        [[nodiscard]] auto aspect_ratio(const MultisectionSurface<AirfoilSection> &surface) -> double;

        /**
         * @brief Measure the bottom point of a multi-section surface at a given position.
         * The bottom point is the point with the minimum local `Y` coordinate at the given
         * position.
         * 
         * @param surface The surface to measure the bottom point of.
         * @param position The position along the reference direction (local `Z` axis)
         * to measure the bottom point at.
         * @return Point_3 The bottom point of the surface at the given position in the local
         * coordinates of the surface.
         */
        [[nodiscard]] auto bottom(const MultisectionSurface<PolygonSection> &surface, double position) -> Point_3;

        /**
         * @brief Measure the bounding box center of a multi-section surface
         * at a given position.
         * @note The result is the center of the bounding box of the surface
         * and not necessarily the center of the surface itself! The bounding
         * box is a rectangular box that completely encloses the cross section of
         * surface at the given position. So if the cross section of the surface
         * is not symmetric to origin, the center of the bounding box is not
         * the center of the surface!
         * 
         * @param surface The surface to measure the bounding box center of.
         * @param position The position along the reference direction (local `Z` axis)
         * to measure the center at.
         * @return Point_3 The center of the bounding box of the surface at the given position
         * in the local coordinates of the surface.
         */
        [[nodiscard]] auto center(const MultisectionSurface<PolygonSection> &surface, double position) -> Point_3;

        /**
         * @anchor measure_barycenter_polygon
         * @brief Measure the geometric center of mass of a multi-section surface with 
         * geom2::PolygonSection sections.
         * First the centroids of each segment are calculated with @ref measure_centroids_polygon .
         * Then the barycenter of the centroids is calculated.
         * 
         * @param surface The surface to measure the center of mass of.
         * @return Point_3 The center of mass of the surface.
         */
        [[nodiscard]] auto centroid(const MultisectionSurface<PolygonSection> &surface) -> Point_3;

        /**
         * @brief Measure the geometric center of mass of a multi-section surface with 
         * geom2::AirfoilSection sections.
         * @attention The local `Z` coordinate of the center of mass is set to 0.0 when the
         * surface is symmetric!
         * 
         * @see @ref measure_barycenter_polygon "measure::centroid(const MultisectionSurface<PolygonSection> &surface)"
         * 
         * @param surface The surface to measure the center of mass of.
         * @return Point_3 The center of mass of the surface.
         */
        [[nodiscard]] auto centroid(const MultisectionSurface<AirfoilSection> &surface) -> Point_3;

        /**
         * @anchor measure_centroids_polygon
         * @brief Measure the geometric center of mass of a multi-section surface with 
         * geom2::PolygonSection sections. The center of mass is calculated by including
         * all coordinate points of each section. The result contains one point for each
         * segment of the surface. The size of the result is therefore `surface.sections.size() - 1`.
         * 
         * @param surface The surface to measure the centers of mass of.
         * @return std::vector<Point_3> Returns a vector of points, where each point is
         * the center of mass of one segment of the surface.
         */
        [[nodiscard]] auto centroids(const MultisectionSurface<PolygonSection> &surface) -> std::vector<Point_3>;

        /**
         * @brief Measure the geometric center of mass of a multi-section surface with 
         * geom2::AirfoilSection sections. In contrast when measuring the centroids of
         * PolygonSection, this function computes the geometric center of the planar
         * trapezoidal segment the leading and trailing edge of the airfoil section
         * define. The result contains one point for each segment of the surface. The
         * size of the result is therefore `surface.sections.size() - 1`.
         * 
         * @see @ref measure_centroids_polygon "measure::centroids(const MultisectionSurface<PolygonSection> &surface)"
         * 
         * @param surface The surface to measure the centers of mass of.
         * @return std::vector<Point_3> Returns a vector of points, where each point is
         * the center of mass of one segment of the surface.
         */
        [[nodiscard]] auto centroids(const MultisectionSurface<AirfoilSection> &surface) -> std::vector<Point_3>;

        /**
         * @brief Measure the chord length of an airfoil surface at a given span position.
         * @note The span refers to the local Z direction of the surface. The symmetric flag
         * of the surface is respected when calculating the chord length.
         *
         * @param surface The surface to measure the chord length of.
         * @param position The position along the span direction (local Z axis) to measure the chord length at.
         * @return double The chord length of the surface at the given position.
         */
        [[nodiscard]] auto chord(const MultisectionSurface<AirfoilSection> &surface, double position) -> double;

        /**
         * @brief Measure the dihedral angle of an airfoil surface at a given span position.
         * @note The dihedral is currently measured at the leading edge of the wing. The
         * symmetric flag of the surface is respected when calculating the dihedral angle.
         *
         * @param surface The airfoil surface to measure the dihedral angle of.
         * @param position The span position along the span direction (local `Z` axis) to measure the dihedral angle at.
         * @return double The dihedral angle of the airfoil surface at the given position. [rad]
         */
        [[nodiscard]] auto dihedral(const MultisectionSurface<AirfoilSection> &surface, double position) -> double;

        /**
         * @brief Measure the height of a multi-section surface at a given position.
         * @note The height refers to the local Y direction of the surface.
         *
         * @param surface The surface to measure the height of.
         * @param position The position along the reference direction (local Z axis) to measure the height at.
         * @return double The height of the surface at the given position.
         */
        [[nodiscard]] auto height(const MultisectionSurface<PolygonSection> &surface, double position) -> double;

        /**
         * @brief Measure the maximum height of a multi-section surface.
         * 
         * @param surface The surface to measure the maximum height of.
         * @return double The maximum height of the surface in local `Y` direction.
         */
        [[nodiscard]] auto height_max(const MultisectionSurface<PolygonSection> &surface) -> double;

        /**
         * @anchor measure_inertia
         * @brief Measure the inertia tensor of a multi-section surface in respect to the
         * center of gravity of the surface.
         * @attention This calculation is costly and a numerical approximation! The result
         * is more accurate the lower the refined_edge_length is! It is also exponentially
         * slower the lower the refined_edge_length is!
         * @note The numerical approximation of the inertia makes the following assumptions:
         * - The surface is a thin-walled structure and not(!) a solid body.
         * - The surface has uniform thickness and density.
         * - The wall thickness is small compared to the outline dimensions of the surface.
         * 
         * This function converts the surface into a triangulated surface mesh. Each of the triangles
         * of the surface mesh is assumed to be a point mass. The inertia of the surface is then
         * the sum of the squared distances of each point mass to the given axis times its assumed mass.
         * The mass is calculated by assuming a unity density and a uniform thickness of the surface.
         * The moment of deviation are calculated with the same approach.
         * 
         * This function optionally refines the surface mesh before calculating the inertia. But watch out:
         * The shorter the edge length of the triangles, the more accurate the result will be, but the
         * longer the calculation will take!
         * 
         * &rArr; The result therefore needs to be scaled by the wall thickness and the density of the surface!
         * Since we assume a thin-walled structure, this linear scaling is valid.
         * 
         * &rArr; The final result is therefore:
         * `I = wall_thickness * density * geom2::measure::inertia(surface)`
         * 
         * @param surface The surface to measure the inertia of.
         * @param refined_edge_length The target edge size when refining the mesh.
         * Defaults to `0.4`. `-1` disables the refinement.
         * @throws std::runtime_error If the meshing of the surface fails.
         * @return CGAL::Eigen_matrix<double, 3, 3> [m^4] The inertia tensor
         * of the surface in respect to the center of gravity of the surface.
         */
        [[nodiscard]] auto inertia(const MultisectionSurface<PolygonSection> &surface,
            double refined_edge_length = 0.4) -> CGAL::Eigen_matrix<double, 3, 3>;
        
        /**
         * @brief Measure the inertia tensor of a multi-section surface in respect to the 
         * center of gravity of the surface.
         * @attention This does **not** respect the is_symmetric flag of the surface! It always
         * assumes this flag is false!
         * 
         * @see @ref measure_inertia
         * "measure::inertia(const MultisectionSurface<PolygonSection> &surface, double refined_edge_length = 0.4)"
         * 
         * @param surface The surface to measure the inertia of.
         * @param refined_edge_length The target edge size when refining the mesh.
         * Defaults to `0.4`. `-1` disables the refinement.
         * @return CGAL::Eigen_matrix<double, 3, 3> [m^4] The inertia tensor
         * of the surface in respect to the center of gravity of the surface.
         */
        [[nodiscard]] auto inertia(const MultisectionSurface<AirfoilSection> &surface,
            double refined_edge_length = 0.4) -> CGAL::Eigen_matrix<double, 3, 3>;

        /**
         * @brief Measure the left point of a multi-section surface at a given position.
         * The left point is the point with the minimum local `X` coordinate at the given
         * position.
         * 
         * @param surface The surface to measure the left point of.
         * @param position The position along the reference direction (local `Z` axis)
         * to measure the left point at.
         * @return Point_3 The left point of the surface at the given position in the local
         * coordinates of the surface.
         */
        [[nodiscard]] auto left(const MultisectionSurface<PolygonSection> &surface, double position) -> Point_3;

        /**
         * @brief Measure the total length of a multi-section surface.
         * @note The length refers to the local Z direction of the surface.
         *
         * @param surface The surface to measure the length of.
         * @return double The length of the surface.
         */
        [[nodiscard]] auto length(const MultisectionSurface<PolygonSection> &surface) -> double;

        /**
         * @brief Measure the mean aerodynamic chord of an airfoil surface.
         *
         * @param surface The airfoil surface to measure the mean aerodynamic chord of.
         * @return double The mean aerodynamic chord of the surface.
         */
        [[nodiscard]] auto mean_aerodynamic_chord(const MultisectionSurface<AirfoilSection> &surface) -> double;

        /**
         * @brief Measure the mean aerodynamic chord spanwise position of an airfoil surface.
         * The position is measured along the local 'Z' axis of the surface.
         * @attention This function assumes, that the given surface is only half of the wing
         * and that the first section is located at the origin of the surface. Otherwise,
         * the result of this measurement is not valid!
         *
         * @param surface The airfoil surface to measure the mean aerodynamic chord position of.
         * @return double The mean aerodynamic chord position of the surface in spanwise direction.
         */
        [[nodiscard]] auto mean_aerodynamic_chord_position(const MultisectionSurface<AirfoilSection> &surface) -> double;

        /**
         * @brief Get the offset of the leading edge of an airfoil section
         * at the given span position.
         * @note This function assumes, that the leading edge is located
         * at the origin of the airfoil section. The symmetric flag of the
         * surface is respected when calculating the leading edge offset.
         *
         * @param surface The airfoil surface to measure the offset of the leading edge.
         * @param position The span position along the span direction (local Z axis) to measure the offset at.
         * @return geom2::Point_3 The position of the leading edge in the local coordinates of the airfoil section.
         */
        [[nodiscard]] auto offset_LE(const MultisectionSurface<AirfoilSection> &surface, double position) -> Point_3;

        /**
         * @brief Calculate the reference area of the wing surface.
         * The reference area is the area of the wing surface projected onto the local
         * `XZ` plane.
         *
         * @param surface The surface to calculate the reference area of.
         * @return double The reference area of the surface.
         */
        [[nodiscard]] auto reference_area(const MultisectionSurface<AirfoilSection> &surface) -> double;

        /**
         * @brief Measure the right point of a multi-section surface at a given position.
         * The right point is the point with the maximum local `X` coordinate at the given
         * position.
         * 
         * @param surface The surface to measure the right point of.
         * @param position The position along the reference direction (local `Z` axis)
         * to measure the right point at.
         * @return Point_3 The right point of the surface at the given position in the local
         * coordinates of the surface.
         */
        [[nodiscard]] auto right(const MultisectionSurface<PolygonSection> &surface, double position) -> Point_3;

        /**
         * @brief Measure the span of an airfoil surface.
         * @note The span refers to the local Z direction of the surface.
         * @attention If the surface only contains half a wing, the result of this
         * measurement is only half the span of the wing.
         *
         * @param surface The surface to measure the span of.
         * @return double The span of the surface.
         */
        [[nodiscard]] auto span(const MultisectionSurface<AirfoilSection> &surface) -> double;

        /**
         * @brief Measure the sweep angle of an airfoil surface at a given span position.
         * The chord offset can be used to set where the sweep angle is measured in chord direction.
         * A chord offset of 0.0 means that the sweep angle is measured at the leading edge.
         * A chord offset of 0.25 means that the sweep angle is measured at 25% of the chord length.
         * A chord offset of 1.0 means that the sweep angle of the trailing edge is measured.
         * @note The symmetric flag of the surface is respected when calculating the sweep angle.
         *
         * @param surface The airfoil surface to measure the sweep angle of.
         * @param position The span position along the span direction (local `Z` axis) to measure the sweep angle at.
         * @param chord_offset The chord offset in the local `X` direction to measure the sweep angle at.
         * @return double The sweep angle of the airfoil surface at the given position. [rad]
         */
        [[nodiscard]] auto sweep(const MultisectionSurface<AirfoilSection> &surface, double position, double chord_offset) -> double;

        /**
         * @brief Measure the taper ratio of an airfoil surface.
         * 
         * @param surface The airfoil surface to measure the taper ratio of.
         * @return double Returns the taper ratio of the airfoil surface.
         */
        [[nodiscard]] auto taper_ratio(const MultisectionSurface<AirfoilSection> &surface) -> double;

        /**
         * @brief Measure the absolute thickness of an airfoil at a local
         * x position. The thickness is calculated from the de-rotated airfoil
         * if the airfoil has a twist applied.
         *
         * @param section The airfoil section to measure the thickness of.
         * @param x_position The local x position to measure the thickness at.
         * @return double The absolute thickness of the airfoil at the given position.
         */
        [[nodiscard]] auto thickness(const AirfoilSection &section, double x_position) -> double;

        /**
         * @brief Measure the maximum thickness of an airfoil section.
         * The thickness is calculated from the de-rotated airfoil
         * if the airfoil has a twist applied.
         * 
         * @param section The airfoil section to measure the maximum thickness of.
         * @return double The maximum thickness of the airfoil section as an absolute value.
         */
        [[nodiscard]] auto thickness_max(const AirfoilSection &section) -> double;

        /**
         * @brief Measure the top point of a multi-section surface at a given position.
         * The top point is the point with the maximum local `Y` coordinate at the given
         * position.
         * 
         * @param surface The surface to measure the top point of.
         * @param position The position along the reference direction (local `Z` axis)
         * to measure the top point at.
         * @return Point_3 The top point of the surface at the given position in the local
         * coordinates of the surface.
         */
        [[nodiscard]] auto top(const MultisectionSurface<PolygonSection> &surface, double position) -> Point_3;

        /**
         * @brief Get the top and bottom point of a polygon at a given x position.
         * This function assumes that the polygon is convex and that the x position
         * is within the polygon's x span.
         * @note This function is an internal helper function, but might be useful
         * for other purposes as well.
         *
         * @param shape The polygon to get the top and bottom point of.
         * @param x_position The local x position to get the top and bottom point at.
         * @return std::array<Point_2, 2> Returns [top, bottom] of the polygon as geom2::Point_2 at the given x position.
         */
        [[nodiscard]] auto top_and_bottom(const Polygon_2 &shape, double x_position) -> std::array<Point_2, 2>;

        /**
         * @brief Measure the twist angle of an airfoil surface at a given span position.
         * @note The symmetric flag of the surface is respected when calculating the twist angle.
         *
         * @param surface The airfoil surface to measure the twist angle of.
         * @param position The span position along the span direction (local `Z` axis) to measure the twist angle at.
         * @return double The twist angle of the airfoil surface at the given position. [rad]
         */
        [[nodiscard]] auto twist(const MultisectionSurface<AirfoilSection> &surface, double position) -> double;

        /**
         * @brief Measure the enclosed volume of a multi-section surface.
         * The open ends at the beginning and end of the surface are closed
         * using a plane face.
         * @attention This calculation is costly!
         *
         * @param surface The surface to measure the volume of.
         * @return double The volume of the surface.
         */
        [[nodiscard]] auto volume(const MultisectionSurface<PolygonSection> &surface) -> double;
        [[nodiscard]] auto volume(const MultisectionSurface<AirfoilSection> &surface) -> double;

        /**
         * @brief Measure the width of a multi-section surface at a given position.
         * @note The width refers to the local X direction of the surface.
         *
         * @param surface The surface to measure the width of.
         * @param position The position along the reference direction (local Z axis) to measure the width at.
         * @return double The width of the surface at the given position.
         */
        [[nodiscard]] auto width(const MultisectionSurface<PolygonSection> &surface, double position) -> double;

        /**
         * @brief Measure the maximum width of a multi-section surface.
         * 
         * @param surface The surface to measure the maximum width of.
         * @return double The maximum width of the surface in local `X` direction.
         */
        [[nodiscard]] auto width_max(const MultisectionSurface<PolygonSection> &surface) -> double;
    }; // namespace measure
};     // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_PROCESSING_MEASURE_H_
