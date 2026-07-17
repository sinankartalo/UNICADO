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

#include "aircraftGeometry2/geometry/section.h"
#include "aircraftGeometry2/processing/transform.h"
#include <CGAL/number_utils.h>
#include <cmath>

namespace geom2
{
    namespace detail
    {
        /* === Functions === */
        auto get_offset(const double span, const double dihedral, const double sweep) -> geom2::Vector_3
        {
            /* Add the sweep offset */
            const double dx = span * std::tan(sweep);

            /* Add the dihedral offset */
            const double dy = span * std::sin(dihedral);

            /* Add the height offset */
            const double dz = span * std::cos(dihedral);

            /* Assemble the offset vector */
            return geom2::Vector_3{dx, dy, -dz};
        }
    }; // namespace detail

    auto Section::get_contour(const bool apply_transform) const noexcept -> Polygon_2
    {
        if (apply_transform)
        {
            auto transform = this->scale * this->rotate;
            return CGAL::transform(transform, this->contour);
        }
        return this->contour;
    }

    void Section::set_contour(const Polygon_2 &contour) noexcept
    {
        this->contour = contour;
    }

    void PolygonSection::set_width(const double width)
    {
        /* Get the unscaled width */
        const double width_unscaled = this->contour.bbox().x_span();

        /* Exit if width is nan or 0 */
        if (std::isnan(width_unscaled) or CGAL::is_zero(width_unscaled))
        {
            return;
        }

        /* Get the scaling factor for x */
        const double x_factor = width / width_unscaled;

        /* Create the transformation with the x-scale changed only*/
        const double y_factor = this->scale.m(1, 1);
        this->scale = Kernel::Aff_transformation_2(x_factor, 0, 0, y_factor, 1);
    }

    void PolygonSection::set_height(const double height)
    {
        /* Get the unscaled height */
        const double height_unscaled = this->contour.bbox().y_span();

        /* Exit if width is nan or 0 */
        if (std::isnan(height_unscaled) or CGAL::is_zero(height_unscaled))
        {
            return;
        }

        /* Get the scaling factor for x */
        const double y_factor = height / height_unscaled;

        /* Create the transformation with the y-scale changed only*/
        const double x_factor = this->scale.m(0, 0);
        this->scale = Kernel::Aff_transformation_2(x_factor, 0, 0, y_factor, 1);
    }

    void PolygonSection::set_beta_angle(const double angle)
    {
        /* Calculate the normal direction, where the angle == beta (Euler angle)*/
        this->normal = {
            0,
            -std::sin(angle),
            std::cos(angle)};
    }

    void PolygonSection::set_scale(const double scale)
    {
        /* Add the scaling transformation */
        this->scale = Kernel::Aff_transformation_2(CGAL::SCALING, scale);
    }

    AirfoilSection::AirfoilSection(const Polygon_2 &contour, const AirfoilProperties properties)
        : AirfoilSection(contour)
    {
        /* Set the properties */
        this->set_chord_length(properties.chord);
        this->set_dihedral_angle(properties.dihedral);
        this->set_twist_angle(properties.twist);
    }

    void AirfoilSection::set_chord_length(const double length)
    {
        /* Get the curren chord scale */
        double chord_scale = this->scale.m(0, 0);

        /* Catch when the scale is 0 */
        if (CGAL::is_zero(chord_scale))
        {
            return;
        }        

        /* Derive the current thickness scale */
        const double thickness_scale = this->scale.m(1, 1) / chord_scale;

        /* Set the new scale */
        chord_scale = length; // -> Assuming the airfoil coordinates are normalized!
        this->scale = Kernel::Aff_transformation_2(chord_scale, 0, 0, chord_scale*thickness_scale, 1);
    }

    void AirfoilSection::set_dihedral_angle(const double angle)
    {
        /* Calculate the normal direction, where the angle == beta (Euler angle)*/
        this->normal = {
            0,
            -std::sin(angle),
            std::cos(angle)};
    }

    void AirfoilSection::set_twist_angle(const double angle)
    {
        /* Limit the twist to +- 90 degrees */
        if (std::abs(angle) > std::numbers::pi)
        {
            throw std::invalid_argument("Twist angle must be in the range of -pi to pi.");
        }

        /* Create the rotation transformation */
        const double sin = std::sin(angle);
        const double cos = std::cos(angle);
        this->rotate = Kernel::Aff_transformation_2(CGAL::ROTATION, sin, cos);
    }

    void AirfoilSection::scale_thickness(const double factor)
    {
        /* Get the current chord scale */
        const double chord_scale = this->scale.m(0, 0);

        /* Set the new transformation */
        this->scale = Kernel::Aff_transformation_2(chord_scale, 0, 0, chord_scale*factor, 1);
    }

    auto AirfoilSection::get_chord_length() const -> double
    {
        /* Get the chord length from the scale transformation */
        return this->scale.m(0, 0);
    }

    auto AirfoilSection::get_thickness_scale() const -> double
    {
        /* Get the thickness scale from the scale transformation */
        return this->scale.m(1, 1) / this->scale.m(0, 0);
    }

    auto AirfoilSection::get_twist_angle() const -> double
    {
        /* Get the twist angle from the rotation transformation
         * => The actual angle is the negative of the atan2 value
         */
        return -std::atan2(this->rotate.m(0, 1), this->rotate.m(0, 0));
    }
}; // namespace geom2
