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

/* Unit Under Test */
#include <aircraftGeometry2/geometry/builder.h>
#include <aircraftGeometry2/geometry/acxml.h>
#include "../../../coordinateSystemConversion/include/coordinateSystemConversion/coordinateSystemConversionConf.h"
#include "../../../coordinateSystemConversion/include/coordinateSystemConversion/coordinateBase.h"

/* === Types ===*/
using Entity3D = geom2::Entity3D;
using Point_2 = geom2::Point_2;
using Point_3 = geom2::Point_3;
using Direction_3 = geom2::Direction_3;
using Polygon_2 = geom2::Polygon_2;
using PolygonSection = geom2::PolygonSection;

/* === Fixtures ===*/

/* === Tests === */
/**
 * @brief Test creating a ellipse polygon section.
 */

TEST(CoordinateSystemConversion, SimpleConversionTest)
{
    namespace sc = SimpleConversion;
    
    sc::Element3D element3D(
        1.0, 0.0, 0.0,
        0.0, 2.0, 0.0,
        0.0, 0.0, 3.0);

    EXPECT_NEAR(element3D.xx(), 1.0, 1e-3);
    EXPECT_NEAR(element3D.yy(), 2.0, 1e-3);
    EXPECT_NEAR(element3D.zz(), 3.0, 1e-3);
}

TEST(CoordinateSystemConversion, SimpleConversionTest2)
{
    namespace sc = SimpleConversion;
    
    sc::Element3D element3D(1.0, 2.0, 3.0);

    element3D.CGAL2AC();

    EXPECT_NEAR(element3D.x(), -2.0, 1e-3);
    EXPECT_NEAR(element3D.y(), -3.0, 1e-3);
    EXPECT_NEAR(element3D.z(), 1.0, 1e-3);

}


TEST(GeometryBuilder, CreatePolygonEllipse)
{
    /* Create the ellipse */
    auto shape = geom2::build::ellipse(2.0, 1.0, 10);

    /* Check the resulting shape */
    ASSERT_EQ(shape.get_contour().size(), 36);
    ASSERT_TRUE(shape.get_contour().is_simple());
    EXPECT_EQ(shape.get_contour(true).bbox().x_span(), 2.0);
    EXPECT_EQ(shape.get_contour(true).bbox().y_span(), 1.0);
    EXPECT_EQ(shape.get_contour(false).bbox().x_span(), 2.0);
    EXPECT_EQ(shape.get_contour(false).bbox().y_span(), 1.0);
}

/**
 * @brief Test conversion from A/C coordinate system to global coordinate system.
 */
TEST(GeometryBuilder, Ac_to_global1)
{
    /* Create the properties */
    Point_3 location(1, -3, 2);
    const Point_3 ac_origin(0,0,0);
    Direction_3 normal(1, 0, 0);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{ name, location, normal };

    /* Conversion from A/C coordinate system to global coordinate system*/
    entity.ac_to_global(ac_origin);

    /* todo write the checker*/
        /* Check default values */
    EXPECT_EQ(entity.name, name);
    EXPECT_NEAR(entity.origin.x(), 1.000, 1e-3);
    EXPECT_NEAR(entity.origin.y(), 2.000, 1e-3);
    EXPECT_NEAR(entity.origin.z(), 3.000, 1e-3);
    EXPECT_EQ(entity.normal, normal);
}

TEST(GeometryBuilder, Ac_to_global2)
{
    /* Create the properties */
    Point_3 location(1, -3, 2);
    const Point_3 ac_origin(0, 0, 0);
    Direction_3 normal(1, -3, 2);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{ name, location, normal };

    /* Conversion from A/C coordinate system to global coordinate system*/
    entity.ac_to_global(ac_origin);

    /* todo write the checker*/
        /* Check default values */
    EXPECT_EQ(entity.name, name);
    EXPECT_NEAR(entity.origin.x(), 1.000, 1e-3);
    EXPECT_NEAR(entity.origin.y(), 2.000, 1e-3);
    EXPECT_NEAR(entity.origin.z(), 3.000, 1e-3);
    EXPECT_NEAR(entity.normal.dx(), 1.000, 1e-3);
    EXPECT_NEAR(entity.normal.dy(), 2.000, 1e-3);
    EXPECT_NEAR(entity.normal.dz(), 3.000, 1e-3);
}

TEST(GeometryBuilder, Ac_to_global3)
{
    /* Create the properties */
    Point_3 location(0, 0, 0);
    const Point_3 ac_origin(1, 2, 3);
    Direction_3 normal(1, -3, 2);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{ name, location, normal };

    /* Conversion from A/C coordinate system to global coordinate system*/
    entity.ac_to_global(ac_origin);

    /* todo write the checker*/
        /* Check default values */
    EXPECT_EQ(entity.name, name);
    EXPECT_NEAR(entity.origin.x(), 1.000, 1e-3);
    EXPECT_NEAR(entity.origin.y(), 2.000, 1e-3);
    EXPECT_NEAR(entity.origin.z(), 3.000, 1e-3);
    EXPECT_NEAR(entity.normal.dx(), 1.000, 1e-3);
    EXPECT_NEAR(entity.normal.dy(), 2.000, 1e-3);
    EXPECT_NEAR(entity.normal.dz(), 3.000, 1e-3);
}

TEST(GeometryBuilder, Ac_to_global4)
{
    /* Create the properties */
    Point_3 location(1, 2, 3);
    const Point_3 ac_origin(1, 2, 3);
    Direction_3 normal(1, -3, 2);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{ name, location, normal };

    /* Conversion from A/C coordinate system to global coordinate system*/
    entity.ac_to_global(ac_origin);

    /* todo write the checker*/
        /* Check default values */
    EXPECT_EQ(entity.name, name);
    EXPECT_NEAR(entity.origin.x(), 2.000, 1e-3);
    EXPECT_NEAR(entity.origin.y(), 5.000, 1e-3);
    EXPECT_NEAR(entity.origin.z(), 1.000, 1e-3);
    EXPECT_NEAR(entity.normal.dx(), 1.000, 1e-3);
    EXPECT_NEAR(entity.normal.dy(), 2.000, 1e-3);
    EXPECT_NEAR(entity.normal.dz(), 3.000, 1e-3);
}

/**
 * @brief Test conversion from global coordinate system to A/C coordinate system.
 */
TEST(GeometryBuilder, Global_to_ac1)
{
    /* Create the properties */
    Point_3 location(1, 2, 3);
    const Point_3 ac_origin(0, 0, 0);
    Direction_3 normal(1, 0, 0);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{ name, location, normal };

    /* Conversion from A/C coordinate system to global coordinate system*/
    entity.global_to_ac(ac_origin);

    /* todo write the checker*/
        /* Check default values */
    EXPECT_EQ(entity.name, name);
    EXPECT_NEAR(entity.origin.x(), 1.000, 1e-3);
    EXPECT_NEAR(entity.origin.y(), -3.000, 1e-3);
    EXPECT_NEAR(entity.origin.z(), 2.000, 1e-3);
    EXPECT_EQ(entity.normal, normal);
}

TEST(GeometryBuilder, Global_to_ac2)
{
    /* Create the properties */
    Point_3 location(1, 2, 3);
    const Point_3 ac_origin(0, 0, 0);
    Direction_3 normal(1, 2, 3);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{ name, location, normal };

    /* Conversion from A/C coordinate system to global coordinate system*/
    entity.global_to_ac(ac_origin);

    /* todo write the checker*/
        /* Check default values */
    EXPECT_EQ(entity.name, name);
    EXPECT_NEAR(entity.origin.x(), 1.000, 1e-3);
    EXPECT_NEAR(entity.origin.y(), -3.000, 1e-3);
    EXPECT_NEAR(entity.origin.z(), 2.000, 1e-3);
    EXPECT_NEAR(entity.normal.dx(), 1.000, 1e-3);
    EXPECT_NEAR(entity.normal.dy(), -3.000, 1e-3);
    EXPECT_NEAR(entity.normal.dz(), 2.000, 1e-3);
}

TEST(GeometryBuilder, Global_to_ac3)
{
    /* Create the properties */
    Point_3 location(0,0,0);
    const Point_3 ac_origin(1, 2, 3);
    Direction_3 normal(1, 2, 3);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{ name, location, normal };

    /* Conversion from A/C coordinate system to global coordinate system*/
    entity.global_to_ac(ac_origin);

    /* todo write the checker*/
        /* Check default values */
    EXPECT_EQ(entity.name, name);
    EXPECT_NEAR(entity.origin.x(), -1.000, 1e-3);
    EXPECT_NEAR(entity.origin.y(), 3.000, 1e-3);
    EXPECT_NEAR(entity.origin.z(), -2.000, 1e-3);
    EXPECT_NEAR(entity.normal.dx(), 1.000, 1e-3);
    EXPECT_NEAR(entity.normal.dy(), -3.000, 1e-3);
    EXPECT_NEAR(entity.normal.dz(), 2.000, 1e-3);
}