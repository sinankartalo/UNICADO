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

#include <gtest/gtest.h>
#include <algorithm>
#include <iterator>

/* Unit Under Test */
#include "aircraftGeometry2/geometry/surface.h"

/* === Types ===*/
using Point_2 = geom2::Point_2;
using Direction_3 = geom2::Direction_3;
using Polygon_2 = geom2::Polygon_2;

/* === Fixtures ===*/
class PolygonSquareSection : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        shape.push_back(Point_2(-1, -1));
        shape.push_back(Point_2(1, -1));
        shape.push_back(Point_2(1, 1));
        shape.push_back(Point_2(-1, 1));
    }

    Polygon_2 shape{};
};

/* === Test === */
/**
 * @brief Test the default constructor.
 */
TEST(PolygonSectionSurface, DefaultConstructor)
{
    /* Create empty surface */
    geom2::MultisectionSurface<geom2::PolygonSection> surface;

    /* Check default values */
    EXPECT_EQ(surface.name, "unknown");
    EXPECT_EQ(surface.origin, CGAL::ORIGIN);
    EXPECT_EQ(surface.normal, Direction_3(0, 0, 1));

    /* Default sections are empty */
    EXPECT_TRUE(surface.sections.empty());
}
