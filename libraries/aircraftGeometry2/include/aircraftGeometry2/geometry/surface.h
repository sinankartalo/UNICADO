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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_SURFACE_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_SURFACE_H_

/* === Includes === */
#include <vector>

#include "entity3d.h"
#include "section.h"

namespace geom2
{
    /* === Classes === */
    /**
     * @class MultisectionSurface
     * @brief Defines multi-section surface geometry.
     * It is basically a vector of sections, where the type of the sections is
     * defined by the template parameter.
     * @attention When using the `is_symmetric` flag, the first section must be
     * located at the origin of the surface coordinate system! Also, this flag
     * is currently intended for airfoil sections only!
     * 
     * @tparam SectionType The type of the sections.
     */
    template <typename SectionType>
    struct MultisectionSurface : public Entity3D
    {
        /* Members */
        std::vector<SectionType> sections{}; /**< The sections of the surface. */
        bool is_symmetric{false};            /**< Whether the surface is symmetric to the local `XY` plane at the surface origin. */
    };
}; // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_GEOMETRY_SURFACE_H_
