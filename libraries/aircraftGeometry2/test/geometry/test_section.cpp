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
#include <aircraftGeometry2/geometry/builder.h>
#include <numbers>

/* Unit Under Test */
#include "aircraftGeometry2/geometry/section.h"

/* === Types === */
using Point_2 = geom2::Point_2;
using Point_3 = geom2::Point_3;
using Vector_3 = geom2::Vector_3;
using Direction_3 = geom2::Direction_3;
using Polygon_2 = geom2::Polygon_2;
using PolygonSection = geom2::PolygonSection;
using AirfoilSection = geom2::AirfoilSection;
using PolygonBuilder = geom2::SectionBuilder<PolygonSection>;
using AirfoilBuilder = geom2::SectionBuilder<AirfoilSection>;

/* === Fixtures === */
class ShapeRhombus : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        shape.push_back(Point_2(-1, 0));
        shape.push_back(Point_2(-0.5, 0.5));
        shape.push_back(Point_2(0, 1));
        shape.push_back(Point_2(0.5, 0.5));
        shape.push_back(Point_2(1, 0));
        shape.push_back(Point_2(0.5, -0.5));
        shape.push_back(Point_2(0, -1));
        shape.push_back(Point_2(-0.5, -0.5));
    }

    Polygon_2 shape{};
};

class SimpleAirfoil : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        shape.push_back(Point_2(0, 0));
        shape.push_back(Point_2(0.3, 0.1));
        shape.push_back(Point_2(1, 0));
        shape.push_back(Point_2(0.3, -0.1));
        shape.push_back(Point_2(0, 0));
    }

    Polygon_2 shape{};
};

/* === Test === */
TEST(PolygonSection, DefaultConstructor)
{
    /* Create empty section */
    PolygonSection section;

    /* Check default values */
    EXPECT_EQ(section.name, "unknown");
    EXPECT_EQ(section.origin, CGAL::ORIGIN);
    EXPECT_EQ(section.normal, Direction_3(0, 0, 1));

    /* Default polygon is empty */
    EXPECT_TRUE(section.get_contour().is_empty());
}

TEST(PolygonSection, ConstructWithShape)
{
    /* Create the shape */
    Polygon_2 shape;
    shape.push_back(Point_2(0, 0));
    shape.push_back(Point_2(1, 0));
    shape.push_back(Point_2(1, 1));
    shape.push_back(Point_2(0, 1));

    /* Create the section */
    PolygonSection section(shape);

    /* Check default values */
    EXPECT_EQ(section.name, "unknown");
    EXPECT_EQ(section.origin, CGAL::ORIGIN);
    EXPECT_EQ(section.normal, Direction_3(0, 0, 1));

    /* Check polygon */
    EXPECT_EQ(section.get_contour(), shape);
    EXPECT_EQ(section.get_contour().size(), 4);
}

TEST_F(ShapeRhombus, BoundingBox)
{
    /* Create the section */
    PolygonSection section(shape);

    /* Check bounding box */
    EXPECT_EQ(section.get_contour().bbox().xmin(), -1);
    EXPECT_EQ(section.get_contour().bbox().xmax(), 1);
    EXPECT_EQ(section.get_contour().bbox().ymin(), -1);
    EXPECT_EQ(section.get_contour().bbox().ymax(), 1);
}

TEST(PolygonSection, GetNumberOfVertices)
{
    /* Create the shape */
    Polygon_2 shape;
    shape.push_back(Point_2(0, 0));
    shape.push_back(Point_2(1, 0));
    shape.push_back(Point_2(1, 1));
    shape.push_back(Point_2(0, 1));

    /* Create the section */
    PolygonSection section(shape);

    /* Check number of vertices */
    EXPECT_EQ(section.size(), 4);
}

/**
 * @brief Test inserting valid sections.
 */
TEST_F(ShapeRhombus, FactoryArrangeSections)
{
    /* Setup the builder */
    PolygonBuilder builder;
    Vector_3 offset(0, 0, 0.5);
    Vector_3 offset_y(0, 0.5, 0.5);
    builder.arrange(shape, offset, 2);
    builder.insert_back(shape, offset_y);

    /* Create the surface */
    auto sections = builder.get_result();
    ASSERT_EQ(sections.size(), 3);

    /* Check the first inserted section */
    EXPECT_EQ(sections[0].get_contour(), shape);
    EXPECT_EQ(sections[0].origin, CGAL::ORIGIN);
    EXPECT_EQ(sections[0].normal, Direction_3(0, 0, 1));

    /* Check the second inserted section */
    EXPECT_EQ(sections[1].get_contour(), shape);
    EXPECT_EQ(sections[1].origin, CGAL::ORIGIN + offset);
    EXPECT_EQ(sections[1].normal, Direction_3(0, 0, 1));

    /* Check the third inserted section */
    EXPECT_EQ(sections[2].get_contour(), shape);
    EXPECT_EQ(sections[2].origin, CGAL::ORIGIN + offset + offset_y);
    EXPECT_EQ(sections[2].normal, Direction_3(0, 0, 1));
}

/**
 * @brief Test setting the width of a section.
 */
TEST(PolygonSection, SetWidth)
{
    /* Create the shape */
    Polygon_2 shape;
    shape.push_back(Point_2(0, 0));
    shape.push_back(Point_2(1, 0));
    shape.push_back(Point_2(1, 1));
    shape.push_back(Point_2(0, 1));

    /* Create the section */
    PolygonSection section(shape);

    /* Set the width */
    section.set_width(2);

    /* Check the width */
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
    EXPECT_EQ(section.get_contour(true)[1], Point_2(2, 0));
    EXPECT_EQ(section.get_contour(true)[2], Point_2(2, 1));
    EXPECT_EQ(section.get_contour(true)[3], Point_2(0, 1));

    /* Test sections with only one point */
    shape = Polygon_2();
    shape.push_back(Point_2(0, 0));
    section = PolygonSection(shape);
    section.set_width(2);
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
}

/**
 * @brief Test setting the height of a section.
 */
TEST(PolygonSection, SetHeight)
{
    /* Create the shape */
    Polygon_2 shape;
    shape.push_back(Point_2(0, 0));
    shape.push_back(Point_2(1, 0));
    shape.push_back(Point_2(1, 1));
    shape.push_back(Point_2(0, 1));

    /* Create the section */
    PolygonSection section(shape);

    /* Set the width */
    section.set_height(3);

    /* Check the width */
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
    EXPECT_EQ(section.get_contour(true)[1], Point_2(1, 0));
    EXPECT_EQ(section.get_contour(true)[2], Point_2(1, 3));
    EXPECT_EQ(section.get_contour(true)[3], Point_2(0, 3));

    /* Test sections with only one point */
    shape = Polygon_2();
    shape.push_back(Point_2(0, 0));
    section = PolygonSection(shape);
    section.set_height(2);
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
}

/**
 * @brief Test setting the Euler angle beta of a polygon section.
 */
TEST(PolygonSection, SetBetaAngle)
{
    /* Create the shape */
    Polygon_2 shape;
    shape.push_back(Point_2(0, 0));
    shape.push_back(Point_2(1, 0));
    shape.push_back(Point_2(1, 1));
    shape.push_back(Point_2(0, 1));

    /* Create the section */
    PolygonSection section(shape);

    /* Set the beta angle */
    section.set_beta_angle(0.7854); // 45 degrees

    /* Check the result */
    EXPECT_EQ(section.normal.dx(), 0.0);
    EXPECT_NEAR(section.normal.dy(), -0.707, 1e-3);
    EXPECT_NEAR(section.normal.dz(), 0.707, 1e-3);
}

/**
 * @brief Test applying an uniform scaling to a section.
 */
TEST(PolygonSection, ApplyUniformScaling)
{
    /* Create the shape */
    Polygon_2 shape;
    shape.push_back(Point_2(0, 0));
    shape.push_back(Point_2(1, 0));
    shape.push_back(Point_2(1, 1));
    shape.push_back(Point_2(0, 1));

    /* Create the section */
    PolygonSection section(shape);

    /* Set the scale */
    section.set_scale(5);

    /* Check the width */
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
    EXPECT_EQ(section.get_contour(true)[1], Point_2(5, 0));
    EXPECT_EQ(section.get_contour(true)[2], Point_2(5, 5));
    EXPECT_EQ(section.get_contour(true)[3], Point_2(0, 5));
}

/**
 * @brief Test inserting a section defining its bounding box using the builder.
 */
// TEST_F(ShapeRhombus, FactoryInsertWithBoundingBox) {
//     /* Setup the builder */
//     geom2::PolygonBuilder builder;
//     Vector_3 offset(0, 0, 0.5);
//     CGAL::Bbox_2 bounds{-3, -2, 3, 2};
//     builder.insert_scaled(shape, offset, bounds);

//     /* Create the surface */
//     auto sections = builder.getResult();
//     ASSERT_EQ(sections.size(), 1);

//     /* Check the first inserted section */
//     EXPECT_EQ(sections[0].get_contour(true).bbox().x_span(), 6.0);
//     EXPECT_EQ(sections[0].get_contour(true).bbox().y_span(), 4.0);
// }

/**
 * @brief Test inserting a creates section with the builder.
 */
TEST_F(ShapeRhombus, FactoryInsertSection)
{
    /* Setup the builder */
    PolygonBuilder builder;

    /* Create the section to insert */
    PolygonSection section(shape);

    /* Insert the sections */
    builder.insert_back(section, Vector_3(0, 0, 0));
    builder.insert_back(section, Vector_3(0, 0, 1));

    /* Test the inserted sections */
    auto sections = builder.get_result();
    ASSERT_EQ(sections.size(), 2);
    EXPECT_EQ(sections[0].get_contour(false), shape);
    EXPECT_EQ(sections[1].get_contour(false), shape);
    EXPECT_EQ(sections[0].origin, Point_3(0, 0, 0));
    EXPECT_EQ(sections[1].origin, Point_3(0, 0, 1));
}

/**
 * @brief Test the default construction of an airfoil section.
 */
TEST(AirfoilSection, DefaultConstructor)
{
    /* Create empty section */
    AirfoilSection section;

    /* Check default values */
    EXPECT_EQ(section.name, "unknown");
    EXPECT_EQ(section.origin, CGAL::ORIGIN);
    EXPECT_EQ(section.normal, Direction_3(0, 0, 1));

    /* Default polygon is empty */
    EXPECT_TRUE(section.get_contour().is_empty());
}

/**
 * @brief Test setting the chord length of a simple airfoil.
 */
TEST_F(SimpleAirfoil, SetChordLength)
{
    /* Create the section */
    AirfoilSection section(shape);

    /* Set the chord length */
    section.set_chord_length(2);

    /* Check the result */
    auto length = section.get_chord_length();
    EXPECT_FLOAT_EQ(length, 2.0);
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
    EXPECT_EQ(section.get_contour(true)[1], Point_2(0.6, 0.2));
    EXPECT_EQ(section.get_contour(true)[2], Point_2(2, 0));
    EXPECT_EQ(section.get_contour(true)[3], Point_2(0.6, -0.2));
    EXPECT_EQ(section.get_contour(true)[4], Point_2(0, 0));

    /* Set the chord length again */
    section.set_chord_length(3.5);
    length = section.get_chord_length();
    EXPECT_FLOAT_EQ(length, 3.5);
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
    EXPECT_NEAR(section.get_contour(true)[1].hx(), 1.05, 1e-3);
    EXPECT_NEAR(section.get_contour(true)[1].hy(), 0.35, 1e-3);
    EXPECT_EQ(section.get_contour(true)[2], Point_2(3.5, 0));
    EXPECT_NEAR(section.get_contour(true)[3].hx(), 1.05, 1e-3);
    EXPECT_NEAR(section.get_contour(true)[3].hy(), -0.35, 1e-3);
    EXPECT_EQ(section.get_contour(true)[4], Point_2(0.0, 0.0));
}

/**
 * @brief Test scaling the thickness of a simple airfoil.
 */
TEST_F(SimpleAirfoil, ScaleThickness)
{
    /* Create the section */
    AirfoilSection section(shape);

    /* Set the chord length */
    section.scale_thickness(2.0);

    /* Check the result */
    ASSERT_EQ(section.get_chord_length(), 1.0);
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
    EXPECT_EQ(section.get_contour(true)[1], Point_2(0.3, 0.2));
    EXPECT_EQ(section.get_contour(true)[2], Point_2(1, 0));
    EXPECT_EQ(section.get_contour(true)[3], Point_2(0.3, -0.2));
    EXPECT_EQ(section.get_contour(true)[4], Point_2(0, 0));
}

/**
 * @brief Read the thickness scale factor from the airfoil.
 */
TEST_F(SimpleAirfoil, GetThicknessScale)
{
    /* Create the section */
    AirfoilSection section(shape);

    /* Set the chord length */
    section.scale_thickness(2.0);

    /* Check the result */
    EXPECT_EQ(section.get_thickness_scale(), 2.0);
}

/**
 * @brief Tes scaling the thickness and then setting the chord length of a simple airfoil.
 */
TEST_F(SimpleAirfoil, ScaleThicknessAndSetChordLength)
{
    /* Create the section */
    AirfoilSection section(shape);

    /* Set the chord length */
    section.scale_thickness(2.0);
    section.set_chord_length(2.0);

    /* Check the result */
    ASSERT_EQ(section.get_chord_length(), 2.0);
    EXPECT_EQ(section.get_contour(true)[0], Point_2(0, 0));
    EXPECT_EQ(section.get_contour(true)[1], Point_2(0.6, 0.4));
    EXPECT_EQ(section.get_contour(true)[2], Point_2(2, 0));
    EXPECT_EQ(section.get_contour(true)[3], Point_2(0.6, -0.4));
    EXPECT_EQ(section.get_contour(true)[4], Point_2(0, 0));
}

/**
 * @brief Test inserting an airfoil with the airfoil builder.
 */
TEST_F(SimpleAirfoil, FactoryInsertBack)
{
    /* Create the builder */
    AirfoilBuilder builder;

    /* Insert the airfoil */
    Vector_3 offset(0, 0, 0.5);
    builder.insert_back(shape, offset);

    /* Create the surface */
    auto sections = builder.get_result();
    ASSERT_EQ(sections.size(), 1);

    /* Check the first inserted section */
    EXPECT_EQ(sections[0].get_contour(), shape);
}

/**
 * @brief Test setting the dihedral angle of a simple airfoil.
 */
TEST_F(SimpleAirfoil, SetDihedralAngle)
{
    /* Create the section */
    AirfoilSection section(shape);

    /* Set the dihedral angle */
    section.set_dihedral_angle(0.7854); // 45 degrees

    /* Check the result */
    EXPECT_EQ(section.normal.dx(), 0.0);
    EXPECT_NEAR(section.normal.dy(), -0.707, 1e-3);
    EXPECT_NEAR(section.normal.dz(), 0.707, 1e-3);
}

/**
 * @brief Test setting the twist angle of a simple airfoil.
 */
TEST_F(SimpleAirfoil, SetTwistAngle)
{
    /* Create the section */
    AirfoilSection section(shape);

    /* Set the twist angle */
    section.set_twist_angle(-0.7854); // -45 degrees

    /* Check the result */
    auto point_TE = section.get_contour(true)[2];
    EXPECT_NEAR(point_TE.hx(), 0.707, 1e-3);
    EXPECT_NEAR(point_TE.hy(), -0.707, 1e-3);

    /* Try setting a twist beyond +-90 degrees*/
    EXPECT_THROW(section.set_twist_angle(std::numbers::pi + 1e-3), std::invalid_argument);
    EXPECT_THROW(section.set_twist_angle(-std::numbers::pi - 1e-3), std::invalid_argument);
}

/**
 * @brief Test reading the twist angle of a simple airfoil.
 */
TEST_F(SimpleAirfoil, GetTwistAngle)
{
    /* Create the section */
    AirfoilSection section(shape);

    /* Set the twist angle */
    section.set_twist_angle(-0.7854); // -45 degrees
    EXPECT_NEAR(section.get_twist_angle(), -0.7854, 1e-3);

    /* Set the twist angle */
    section.set_twist_angle(+0.7854); // +45 degrees
    EXPECT_NEAR(section.get_twist_angle(), +0.7854, 1e-3);

    /* Test more extreme values */
    section.set_twist_angle(std::numbers::pi);
    EXPECT_NEAR(section.get_twist_angle(), std::numbers::pi, 1e-3);
    section.set_twist_angle(-std::numbers::pi);
    EXPECT_NEAR(section.get_twist_angle(), -std::numbers::pi, 1e-3);
}
