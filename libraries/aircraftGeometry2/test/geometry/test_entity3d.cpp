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
#include <CGAL/Origin.h>

/* Unit Under Test */
#include "aircraftGeometry2/geometry/entity3d.h"

/* === Types ===*/
using Entity3D = geom2::Entity3D;
using Point_3 = geom2::Point_3;
using Direction_3 = geom2::Direction_3;

/* === Tests ===*/
/**
 *  @brief Test the default constructor.
 */
TEST(Entity3D, DefaultConstructor)
{
    /* Create default entity */
    Entity3D entity;

    /* Check default values */
    EXPECT_EQ(entity.name, "unknown");
    EXPECT_EQ(entity.origin, CGAL::ORIGIN);
    EXPECT_EQ(entity.normal, Direction_3(0, 0, 1));
    EXPECT_EQ(entity.rotation_z, 0.0);
}

/**
 *  @brief Test the constructor at a location.
 */
TEST(Entity3D, ConstructAtLocation)
{
    /* Create the location */
    Point_3 location(1, 2, 3);

    /* Create default entity */
    Entity3D entity(location);

    /* Check default values */
    EXPECT_EQ(entity.name, "unknown");
    EXPECT_EQ(entity.origin, location);
    EXPECT_EQ(entity.normal, Direction_3(0, 0, 1));
}

/**
 * @brief Test the constructor with custom values.
 */
TEST(Entity3D, ConstructCustom)
{
    /* Create the properties */
    Point_3 location(1, 2, 3);
    Direction_3 normal(1, 0, 0);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Check default values */
    EXPECT_EQ(entity.name, name);
    EXPECT_EQ(entity.origin, location);
    EXPECT_EQ(entity.normal, normal);

    /* Changing the input string should not affect the name*/
    name = "test2";
    EXPECT_EQ(entity.name, "test");
}
