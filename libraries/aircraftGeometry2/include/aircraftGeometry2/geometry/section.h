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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_SECTION_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_SECTION_H_

/* === Includes === */
#include <vector>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polygon_2.h>
#include "entity3d.h"

namespace geom2
{
    /* === Functions === */
    namespace detail
    {
        /**
         * @brief Calculate the offset vector when inserting an airfoil section which
         * has a dihedral and a sweep angle at span position. This function converts
         * those parameters to an offset vector.
         * @attention The span direction is automatically converted to a negative `Z`
         * coordinate system as this is the convention used in this library.
         * 
         * @param span The offset span position. NOT the absolute span position!
         * @param dihedral The dihedral angle of the airfoil section. [rad]
         * @param sweep The sweep angle of the airfoil section. [rad]
         * @return geom2::Vector_3 The resulting offset vector.
         */
        [[nodiscard]] auto get_offset(double span, double dihedral, double sweep) -> geom2::Vector_3;
    }; // namespace detail

    /* === Types === */
    using Polygon_2 = CGAL::Polygon_2<Kernel>;

    /* === Classes === */
    /**
     * @class Section
     * @brief This class defines the base properties of a section.
     * It is the base geometry class used for the surface sections.
     */
    class Section : public Entity3D
    {
    public:
        /**
         * @brief Default constructor for a section.
         */
        Section() = default;

        /**
         * @brief Construct a new Section object with a given contour.
         * 
         * @param contour The contour of the section.
         */
        explicit Section(const Polygon_2 &contour) : contour(contour) {}

        /**
         * @brief Default destructor for a section.
         */
        ~Section() override = default;

        /**
         * @brief Get the Contour polygon of the section.
         * @param apply_transform If true, the contour polygon is transformed by the section's transformation.
         *
         * @return Polygon_2 The contour polygon of the section.
         */
        [[nodiscard]] auto get_contour(bool apply_transform = false) const noexcept -> Polygon_2;

        /**
         * @brief Set the contour of the polygon section.
         * @attention You should make sure that the inserted polygon is normalized,
         * otherwise the existing transformation produce wrong results!
        */
        void set_contour(const Polygon_2 &contour) noexcept;

        /**
         * @brief Get the number of vertices of the section.
         *
         * @return std::size_t The number of vertices of the section.
         */
        [[nodiscard]] auto size() const noexcept -> std::size_t { return contour.size(); }

    protected:
        Polygon_2 contour{};                                                               /**< The contour of the section without any transformation applied. */
        Kernel::Aff_transformation_2 scale{Kernel::Aff_transformation_2(CGAL::IDENTITY)};  /**< The scaling of the section. */
        Kernel::Aff_transformation_2 rotate{Kernel::Aff_transformation_2(CGAL::IDENTITY)}; /**< The rotation of the section. */
    };

    /* === Concepts === */

    /**
     * @brief Concept which defines a valid shape which has a contour and is derived from Section.
     *
     * @tparam T The type which represents the shape
     */
    template <typename T>
    concept Shape = requires(T t) {
        {
            t.get_contour()
        } -> std::same_as<Polygon_2>;
        std::is_base_of_v<Section, T>;
        // { t.origin } -> std::same_as<Point_3>;
    };

    /**
     * @class PolygonSection
     * @brief Class for section geometry using polygons.
     */
    class PolygonSection : public Section
    {
    public:
        /* Constructor */
        /**
         * @brief Default constructor for a polygon section.
         */
        PolygonSection() = default;

        /**
         * @brief Construct a new Polygon Section object with a given contour.
         * 
         * @param contour The contour of the section.
         */
        explicit PolygonSection(const Polygon_2 &contour) : Section(contour) {}

        /**
         * @brief Default destructor for a polygon section.
         */
        ~PolygonSection() override = default;

        /**
         * @brief Set the width of the section.
         * Sets the internal scaling transformation accordingly.
         *
         * @param width The width of the section.
         */
        void set_width(double width);

        /**
         * @brief Set the height of the section.
         * Sets the internal scaling transformation accordingly.
         *
         * @param height The height of the section.
         */
        void set_height(double height);

        /**
         * @brief Set the rotation around the local `X` axis of the section.
         * The rotation happens around the local origin of the 2d section.
         *
         * @param angle The rotation angle around the local `X` axis of the section. [rad]
         */
        void set_beta_angle(double angle);

        /**
         * @brief Set the uniform scaling of the section.
         * @attention This overwrites the width and height scaling!
         *
         * @param scale The uniform scaling of the section.
         */
        void set_scale(double scale);
    };

    /**
     * @class AirfoilSection
     * @brief Class for section geometry using airfoils.
     * 
     */
    class AirfoilSection : public Section
    {
    public:
        /**
         * @struct AirfoilProperties
         * @brief Helper struct to group the airfoil properties.
         */
        struct AirfoilProperties
        {
            double chord{0.0};    /**< The chord length of the airfoil section. */
            double dihedral{0.0}; /**< The dihedral angle of the airfoil section. [rad] */
            double twist{0.0};    /**< The twist angle of the airfoil section. [rad] */
        };

        /* Constructor */
        /**
         * @brief Default constructor for an airfoil section.
         */
        AirfoilSection() = default;

        /**
         * @brief Construct a new Airfoil Section object with a given contour.
         * 
         * @param contour The contour of the section.
         */
        explicit AirfoilSection(const Polygon_2 &contour) : Section(contour) {}

        /**
         * @brief Construct a new Airfoil Section object with a given contour and properties.
         * 
         * @param contour The contour of the section.
         * @param properties The properties of the airfoil section.
         */
        AirfoilSection(const Polygon_2 &contour, AirfoilProperties properties);

        /**
         * @brief Default destructor for an airfoil section.
         */
        ~AirfoilSection() override = default;

        /**
         * @brief Set the chord length of the airfoil section.
         *
         * @param length The chord length of the airfoil section.
         */
        void set_chord_length(double length);

        /**
         * @brief Set the dihedral angle of the airfoil section.
         * The dihedral is defined as a rotation around the local
         * `X` axis of the airfoil. The sign convention is defined
         * accordingly.
         *
         * @param angle The dihedral angle of the airfoil section. [rad]
         */
        void set_dihedral_angle(double angle);

        /**
         * @brief Set the twist angle of the airfoil section.
         * The twist is defined as a rotation around the local
         * `Z` axis of the airfoil. The sign convention is defined
         * accordingly.
         * @attention The twist angle has to be in the range [-pi, pi]!
         * @throws std::invalid_argument If the twist angle is out of range.
         *
         * @param angle The twist angle of the airfoil section. [rad]
         */
        void set_twist_angle(double angle);

        /**
         * @brief Scale the thickness of the airfoil section.
         * @details The thickness of the airfoil is scaled by this factor,
         * meaning a factor 2.0 doubles the thickness of the airfoil. It is
         * effectively a scaling of the `Y` axis of the airfoil.
         * @attention This changes the thickness to chord ratio of the airfoil!
         * The chord length is not changed.
         * 
         * @param factor The scaling factor of the thickness.
         */
        void scale_thickness(double factor);

        /**
         * @brief Get the absolute chord length of the airfoil section.
         *
         * @return double The absolute chord length.
         */
        [[nodiscard]] auto get_chord_length() const -> double;

        /**
         * @brief Get the current thickness scaling factor of the airfoil section.
         *
         * @return double The thickness scaling factor.
         */
        [[nodiscard]] auto get_thickness_scale() const -> double;

        /**
         * @brief Get the current twist angle of the airfoil section.
         *
         * @return double The twist angle of the airfoil section. [rad]
         */
        [[nodiscard]] auto get_twist_angle() const -> double;
    };
}; // namespace geom2
#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_SECTION_H_
