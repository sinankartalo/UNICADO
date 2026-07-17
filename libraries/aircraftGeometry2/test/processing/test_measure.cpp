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
#include "aircraftGeometry2/processing/transform.h"

/* Unit Under Test */
#include "aircraftGeometry2/processing/measure.h"

/* === Types ===*/
using Point_2 = geom2::Point_2;
using Polygon_2 = geom2::Polygon_2;
using Vector_3 = geom2::Vector_3;
using PolygonSection = geom2::PolygonSection;
using AirfoilSection = geom2::AirfoilSection;
using PolygonBuilder = geom2::SectionBuilder<PolygonSection>;
using AirfoilBuilder = geom2::SectionBuilder<AirfoilSection>;

/* === Fixtures ===*/
class MeasurePolygon : public ::testing::Test
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

class MeasureCone : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        auto shape = geom2::build::ellipse(2.0, 1.0);
        
        /* Create the cone */
        cone.sections.emplace_back(shape);
        cone.sections.emplace_back(shape);
        cone.sections.back().origin = {0, 0.25, 3};
        cone.sections.back().set_width(1.0);
        cone.sections.back().set_height(0.5);
    }

    geom2::MultisectionSurface<PolygonSection> cone{};
};

class MeasurePyramid : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        Polygon_2 tip;
        Polygon_2 shape;
        tip.push_back(Point_2(0, 0));
        shape.push_back(Point_2(-1, -1));
        shape.push_back(Point_2(1, -1));
        shape.push_back(Point_2(1, 1));
        shape.push_back(Point_2(-1, 1));
        
        /* Create the pyramid */
        pyramid.sections.emplace_back(tip);
        pyramid.sections.emplace_back(shape);
        pyramid.sections.back().origin = {0, 0, 2};
    }

    geom2::MultisectionSurface<PolygonSection> pyramid{};
};

class MeasureAirfoil : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        shape.push_back(Point_2(0, 0));
        shape.push_back(Point_2(0.5, 0.2));
        shape.push_back(Point_2(1.0, 0.0));
        shape.push_back(Point_2(0.5, 0.0));
    }

    Polygon_2 shape{};
};

class MeasurePlanarWing : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        Polygon_2 shape{};
        shape.push_back(Point_2(0, 0));
        shape.push_back(Point_2(0.5, 0.2));
        shape.push_back(Point_2(1.0, 0.0));
        shape.push_back(Point_2(0.5, 0.0));

        /* Create the airfoil */
        AirfoilSection airfoil{shape};

        /* Create a simple wing surface with 3 sections */
        wing.sections.emplace_back(airfoil);
        wing.sections.emplace_back(airfoil);
        wing.sections.emplace_back(airfoil);
        wing.sections[0].origin = geom2::Point_3(0, 0, 0);
        wing.sections[0].set_chord_length(3.0);
        wing.sections[1].origin = geom2::Point_3(0, 0, -2);
        wing.sections[1].set_chord_length(1.0);
        wing.sections[2].origin = geom2::Point_3(0, 0, -4);
        wing.sections[2].set_chord_length(1.0);
    }

    geom2::MultisectionSurface<AirfoilSection> wing{};
};

class MeasureTwistedWing : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        Polygon_2 shape{};
        shape.push_back(Point_2(0, 0));
        shape.push_back(Point_2(0.5, 0.2));
        shape.push_back(Point_2(1.0, 0.0));
        shape.push_back(Point_2(0.5, 0.0));

        /* Create the airfoil */
        AirfoilSection airfoil{shape};

        /* Create a simple wing surface with 2 sections */
        wing.sections.emplace_back(airfoil);
        wing.sections.emplace_back(airfoil);
        wing.sections.emplace_back(airfoil);
        wing.sections[0].origin = geom2::Point_3(0, 0, 0);
        wing.sections[0].set_chord_length(3.0);
        wing.sections[0].set_twist_angle(0.0);
        wing.sections[1].origin = geom2::Point_3(0, 0, -2);
        wing.sections[1].set_chord_length(1.0);
        wing.sections[1].set_twist_angle(10.0 * geom2::detail::to_radians);
        wing.sections[2].origin = geom2::Point_3(0, 0, -4);
        wing.sections[2].set_chord_length(1.0);
        wing.sections[2].set_twist_angle(0.0);
    }

    geom2::MultisectionSurface<AirfoilSection> wing{};
};

class MeasureWingWithDihedral : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create the shape */
        Polygon_2 shape{};
        shape.push_back(Point_2(0, 0));
        shape.push_back(Point_2(0.5, 0.2));
        shape.push_back(Point_2(1.0, 0.0));
        shape.push_back(Point_2(0.5, 0.0));

        /* Create the airfoil */
        AirfoilSection airfoil{shape};

        /* Create a simple wing surface with 2 sections */
        wing.sections.emplace_back(airfoil);
        wing.sections.emplace_back(airfoil);
        wing.sections.emplace_back(airfoil);
        wing.sections[0].origin = geom2::Point_3(0, 0, 0);
        wing.sections[0].set_chord_length(3.0);
        wing.sections[1].origin = geom2::Point_3(0, 0, -2);
        wing.sections[1].set_chord_length(1.0);
        wing.sections[2].origin = geom2::Point_3(0, 2, -4);
        wing.sections[2].set_chord_length(1.0);
    }

    geom2::MultisectionSurface<AirfoilSection> wing{};
};

/* === Tests === */
/**
 * @brief Test calculating an empty surface area.
 */
TEST_F(MeasurePolygon, CalculateEmptyArea)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;

    /* Insert sections */
    Vector_3 offset(1, 0, 1);
    surface.sections.emplace_back(shape);

    /* Check the surface area */
    EXPECT_EQ(geom2::measure::area(surface), 0.0);
}

/**
 * @brief Test calculating a surface area.
 */
TEST_F(MeasurePolygon, CalculateArea)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    PolygonBuilder builder;
    Vector_3 offset(0, 0, 1);
    builder.arrange(shape, offset, 3);
    surface.sections = builder.get_result();

    /* Check the surface area */
    EXPECT_EQ(geom2::measure::area(surface), 16.0);
}

/**
 * @brief Measure the area of a planar surface.
 */
TEST(MeasurePlanarSurface, CalculateArea)
{
    /* Create the surface */
    geom2::Polygon_2 shape;
    shape.push_back(geom2::Point_2(-1, 0));
    shape.push_back(geom2::Point_2(1, 0));
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections.back().set_width(1.0);
    surface.sections.back().origin = {0.2, 0, 3};

    /* Check the surface area */
    EXPECT_EQ(geom2::measure::area(surface), 4.5);
}

/**
 * @brief Measure the local width of a surface.
 */
TEST_F(MeasurePolygon, CalculateLocalWidth)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    PolygonBuilder builder;
    Vector_3 offset(0, 0, 1);
    builder.arrange(shape, offset, 3);
    surface.sections = builder.get_result();
    surface.sections[0].set_width(3.0);
    surface.sections[2].set_width(1.0);

    /* Check the local width */
    EXPECT_EQ(geom2::measure::width(surface, 0.0), 3.0);
    EXPECT_EQ(geom2::measure::width(surface, 0.5), 2.5);
    EXPECT_EQ(geom2::measure::width(surface, 2.0), 1.0);
}

/**
 * @brief Measure the local width of a pyramid.
 */
TEST_F(MeasurePyramid, CalculateLocalWidth)
{
    /* Check the local width */
    EXPECT_EQ(geom2::measure::width(pyramid, 0.0), 0.0);
    EXPECT_EQ(geom2::measure::width(pyramid, 1.0), 1.0);
    EXPECT_EQ(geom2::measure::width(pyramid, 2.0), 2.0);
}

/**
 * @brief Test Measuring the max width of a polygon surface.
 */
TEST_F(MeasurePolygon, WidthMax)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections[0].set_width(0.2);
    surface.sections[1].set_width(0.5);
    surface.sections[2].set_width(0.3);

    /* Check the max width */
    EXPECT_EQ(geom2::measure::width_max(surface), 0.5);
}

/**
 * @brief Test measuring the max width of a pyramid surface.
 */
TEST_F(MeasurePyramid, WidthMax)
{
    /* Check the max width */
    EXPECT_EQ(geom2::measure::width_max(pyramid), 2.0);
}

/**
 * @brief Measure the local height of a surface.
 */
TEST_F(MeasurePolygon, CalculateLocalHeight)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    PolygonBuilder builder;
    Vector_3 offset(0, 0, 1);
    builder.arrange(shape, offset, 3);
    surface.sections = builder.get_result();

    /* Check the local height */
    EXPECT_EQ(geom2::measure::height(surface, 0.5), 2.0);
}

/**
 * @brief Test measuring the max height of a polygon surface.
 */
TEST_F(MeasurePolygon, HeightMax)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections[0].set_height(0.2);
    surface.sections[1].set_height(0.5);
    surface.sections[1].origin = {0, 0, 1};
    surface.sections[2].set_height(0.3);
    surface.sections[2].origin = {0, 0, 2};

    /* Check the max height */
    EXPECT_EQ(geom2::measure::height_max(surface), 0.5);
}

/**
 * @brief Test measuring the length of a polygon surface.
 */
TEST_F(MeasurePolygon, CalculateLength)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    PolygonBuilder builder;
    Vector_3 offset(0, 0, 2);
    builder.arrange(shape, offset, 3);
    surface.sections = builder.get_result();

    /* Check the length */
    EXPECT_EQ(geom2::measure::length(surface), 4.0);
}

/**
 * @brief Test measuring the thickness at x position
 */
TEST_F(MeasureAirfoil, ThicknessAtXPosition)
{
    /* Create the airfoil */
    AirfoilSection airfoil{shape};
    airfoil.set_chord_length(3.0);

    /* Measure the thickness at X = 0.4 */
    EXPECT_FLOAT_EQ(geom2::measure::thickness(airfoil, 0.4), 0.48);

    /* Scale thickness and measure again */
    airfoil.scale_thickness(2.0);
    EXPECT_FLOAT_EQ(geom2::measure::thickness(airfoil, 0.4), 0.96);
}

/**
 * @brief Test measuring the maximum thickness of an airfoil
 */
TEST_F(MeasureAirfoil, MaximumThickness)
{
    /* Create the airfoil */
    AirfoilSection airfoil{shape};
    airfoil.set_chord_length(3.0);

    /* Measure the maximum thickness */
    EXPECT_FLOAT_EQ(geom2::measure::thickness_max(airfoil), 0.60);

    /* Scale thickness and measure again */
    airfoil.scale_thickness(2.0);
    EXPECT_FLOAT_EQ(geom2::measure::thickness_max(airfoil), 1.20);
}

/**
 * @brief Test measuring the top and bottom point at x position
 */
TEST_F(MeasureAirfoil, TopAndBottomAtXPosition)
{
    /* Create the airfoil */
    AirfoilSection airfoil{shape};
    airfoil.set_chord_length(3.0);

    /* Measure the thickness at X = 0.4 */
    auto [top, bottom] = geom2::measure::top_and_bottom(airfoil.get_contour(false), 0.4);
    EXPECT_NEAR(top.x(), 0.4, 1e-3);
    EXPECT_NEAR(top.y(), 0.16, 1e-3);
    EXPECT_NEAR(bottom.x(), 0.4, 1e-3);
    EXPECT_NEAR(bottom.y(), 0.0, 1e-3);
}

/**
 * @brief Test measuring the wetted area of a planar wing.
 */
TEST_F(MeasurePlanarWing, WettedArea)
{
    /* Measure the wetted area */
    EXPECT_NEAR(geom2::measure::area(wing), 12.606, 1e-3);

    /* Measure the wetted area as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::area(wing), 2*12.606, 1e-3);
}

/**
 * @brief Test measuring the chord length at span position of a planar wing
 */
TEST_F(MeasurePlanarWing, ChordLength)
{
    /* Measure the chord length at different positions */
    EXPECT_FLOAT_EQ(geom2::measure::chord(wing, 0.0), 3.0);
    EXPECT_FLOAT_EQ(geom2::measure::chord(wing, -1.0), 2.0);
    EXPECT_FLOAT_EQ(geom2::measure::chord(wing, -2.0), 1.0);

    /* A symmetric wing can also be measured in the opposite direction */
    wing.is_symmetric = true;
    EXPECT_FLOAT_EQ(geom2::measure::chord(wing, 1.0), 2.0);
}

/**
 * @brief Test measuring the chord length at span position of a twisted wing
 */
TEST_F(MeasureTwistedWing, ChordLength)
{
    /* Measure the chord length at span position 0.5 */
    EXPECT_FLOAT_EQ(geom2::measure::chord(wing, 0.0), 3.0);
    EXPECT_FLOAT_EQ(geom2::measure::chord(wing, -1.0), 2.0);
    EXPECT_FLOAT_EQ(geom2::measure::chord(wing, -2.0), 1.0);
}

/**
 * @brief Test measuring the span of a planar wing
 */
TEST_F(MeasurePlanarWing, Span)
{
    /* Measure the span */
    EXPECT_FLOAT_EQ(geom2::measure::span(wing), 4.0);

    /* Measure the span as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_FLOAT_EQ(geom2::measure::span(wing), 8.0);
}

/**
 * @brief Test measuring the span of a twisted wing
 */
TEST_F(MeasureTwistedWing, Span)
{
    /* Measure the span */
    EXPECT_FLOAT_EQ(geom2::measure::span(wing), 4.0);
}

/**
 * @brief Test measuring the span of a wing with dihedral
 */
TEST_F(MeasureWingWithDihedral, Span)
{
    /* Measure the span */
    EXPECT_FLOAT_EQ(geom2::measure::span(wing), 4.0);
}

/**
 * @brief Test measuring the leading edge offset of a planar wing
 */
TEST_F(MeasurePlanarWing, OffsetLE)
{
    /* Measure the leading edge offset */
    EXPECT_EQ(geom2::measure::offset_LE(wing, -1.0), geom2::Point_3(0, 0, -1));

    /* Move the outer wing section in x direction and check the resulting offset */
    wing.sections[1].origin = geom2::Point_3(1, 0, -2);
    EXPECT_EQ(geom2::measure::offset_LE(wing, -1.0), geom2::Point_3(0.5, 0, -1));
    
    /* A symmetric wing can also be measured on the other side */
    wing.is_symmetric = true;
    EXPECT_EQ(geom2::measure::offset_LE(wing, 1.0), geom2::Point_3(0.5, 0, 1));
}

/**
 * @brief Test measuring the leading edge offset of a wing with dihedral
 */
TEST_F(MeasureWingWithDihedral, OffsetLE)
{
    /* Check the resulting offset */
    EXPECT_EQ(geom2::measure::offset_LE(wing, -3.0), geom2::Point_3(0, 1, -3));
}

/**
 * @brief Measure the sweep angle of a planar wing at an arbitrary chord offset.
 */
TEST_F(MeasurePlanarWing, Sweep)
{
    /* Measure the sweep angle */
    EXPECT_NEAR(geom2::measure::sweep(wing, -0.5, 0.00), 0.0 * geom2::detail::to_radians, 1e-3);
    EXPECT_NEAR(geom2::measure::sweep(wing, -0.5, 1.00), 45.0 * geom2::detail::to_radians, 1e-3);
    EXPECT_NEAR(geom2::measure::sweep(wing, -0.5, 0.25), 14.0 * geom2::detail::to_radians, 1e-1);

    /* Move the middle section back and measure again */
    wing.sections[1].origin = geom2::Point_3(2.0, 0, -2);
    EXPECT_NEAR(geom2::measure::sweep(wing, -0.5, 0.00), -45.0 * geom2::detail::to_radians, 1e-3);
    EXPECT_NEAR(geom2::measure::sweep(wing, -0.5, 1.00), 0.0 * geom2::detail::to_radians, 1e-3);

    /* A symmetric wing can also be measured on the other side */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::sweep(wing, 0.5, 0.00), -45.0 * geom2::detail::to_radians, 1e-3);
}

/**
 * @brief Measure the sweep angle of a twisted wing at an arbitrary chord offset.
 */
TEST_F(MeasureTwistedWing, Sweep)
{
    GTEST_SKIP() << "How is the sweep angle of a twisted wing defined? Should it be the projected angle or the 3D angle?";
}

/**
 * @brief Measure the projected reference area of a planar wing
 */
TEST_F(MeasurePlanarWing, ReferenceArea)
{
    /* Measure the reference area */
    EXPECT_FLOAT_EQ(geom2::measure::reference_area(wing), 6.0);

    /* Measure the reference area as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_FLOAT_EQ(geom2::measure::reference_area(wing), 12.0);

    /* Measure the area when the extrusion is in positive Z direction */
    wing.is_symmetric = false;
    wing.sections[1].origin = geom2::Point_3(0, 0, 2);
    wing.sections[2].origin = geom2::Point_3(0, 0, 4);
    EXPECT_FLOAT_EQ(geom2::measure::reference_area(wing), 6.0);

    /* Measure the area as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_FLOAT_EQ(geom2::measure::reference_area(wing), 12.0);
}

/**
 * @brief Measure the projected reference area of a twisted wing
 */
TEST_F(MeasureTwistedWing, ReferenceArea)
{
    /* Measure the reference area */
    EXPECT_NEAR(geom2::measure::reference_area(wing), 5.969, 1e-3);
}

/**
 * @brief Measure the projected reference area of a wing with dihedral
 */
TEST_F(MeasureWingWithDihedral, ReferenceArea)
{
    /* Measure the reference area */
    EXPECT_FLOAT_EQ(geom2::measure::reference_area(wing), 6.0);
}

/**
 * @brief Measure the dihedral angle of a planar wing
 */
TEST_F(MeasurePlanarWing, Dihedral)
{
    /* Measure the dihedral angle */
    EXPECT_FLOAT_EQ(geom2::measure::dihedral(wing, -3.0), 0.0);
}

/**
 * @brief Measure the dihedral angle of a twisted wing
 */
TEST_F(MeasureTwistedWing, Dihedral)
{
    /* Measure the dihedral angle */
    EXPECT_FLOAT_EQ(geom2::measure::dihedral(wing, -3.0), 0.0);
}

/**
 * @brief Measure the dihedral angle of a wing with dihedral
 */
TEST_F(MeasureWingWithDihedral, Dihedral)
{
    /* Measure the dihedral angle */
    EXPECT_NEAR(geom2::measure::dihedral(wing, -3.0), 45.0 * geom2::detail::to_radians, 1e-3);

    /* Measure a negative dihedral angle */
    wing.sections.back().origin = geom2::Point_3(0, -2, -4);
    EXPECT_NEAR(geom2::measure::dihedral(wing, -3.0), -45.0 * geom2::detail::to_radians, 1e-3);

    /* A symmetric wing can also be measured on the other side */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::dihedral(wing, 3.0), -45.0 * geom2::detail::to_radians, 1e-3);
}

/**
 * @brief Measure the twist of a planar wing.
 */
TEST_F(MeasurePlanarWing, Twist)
{
    /* Measure the twist angle */
    EXPECT_FLOAT_EQ(geom2::measure::twist(wing, -3.0), 0.0);

    /* Measure the twist angle of a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_FLOAT_EQ(geom2::measure::twist(wing, 3.0), 0.0);
}

/**
 * @brief Measure the twist of a wing with dihedral.
 */
TEST_F(MeasureWingWithDihedral, Twist)
{
    /* Measure the twist angle */
    EXPECT_FLOAT_EQ(geom2::measure::twist(wing, -3.0), 0.0);

    /* Measure the twist angle of a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_FLOAT_EQ(geom2::measure::twist(wing, 3.0), 0.0);
}

/**
 * @brief Measure the twist of a twisted wing.
 */
TEST_F(MeasureTwistedWing, Twist)
{
    /* Set the twist of the wing to an even value in rad */
    wing.sections[1].set_twist_angle(0.1);

    /* Measure the twist angle */
    EXPECT_NEAR(geom2::measure::twist(wing, -2.0), 0.10, 1e-3);
    EXPECT_NEAR(geom2::measure::twist(wing, -3.0), 0.05, 1e-3);
    EXPECT_NEAR(geom2::measure::twist(wing, -4.0), 0.00, 1e-3);

    /* Measure the twist of a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::twist(wing, 2.0), 0.10, 1e-3);
    EXPECT_NEAR(geom2::measure::twist(wing, 3.0), 0.05, 1e-3);
    EXPECT_NEAR(geom2::measure::twist(wing, 4.0), 0.00, 1e-3);
}

/**
 * @brief Measure the volume of a polygon surface.
 */
TEST_F(MeasurePolygon, Volume)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections.back().origin = {0, 0, 5};

    /* Measure the volume */
    EXPECT_FLOAT_EQ(geom2::measure::volume(surface), 20.0);

    /* Add another section and measure again */
    surface.sections.emplace_back(shape);
    surface.sections.back().origin = {0, 0, 10};
    EXPECT_FLOAT_EQ(geom2::measure::volume(surface), 40.0);
}

/**
 * @brief Measure the volume of a planar wing surface.
 */
TEST_F(MeasurePlanarWing, Volume)
{
    /* Measure the volume */
    EXPECT_NEAR(geom2::measure::volume(wing), 1.066, 1e-3);

    /* Measure the volume as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::volume(wing), 2*1.066, 2e-3);
}

/**
 * @brief Measure the aspect ratio of a planar wing surface.
 */
TEST_F(MeasurePlanarWing, AspectRatio)
{
    /* Measure the aspect ratio of the wing */
    EXPECT_NEAR(geom2::measure::aspect_ratio(wing), (16.0/6.0), 1e-3);

    /* Measure the aspect ratio as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::aspect_ratio(wing), (64.0/12.0), 1e-3);
}

/**
 * @brief Measure the taper ratio of a planar wing surface.
 */
TEST_F(MeasurePlanarWing, TaperRatio)
{
    /* Measure the taper ratio of the wing */
    EXPECT_NEAR(geom2::measure::taper_ratio(wing), (1.0/3.0), 1e-3);

    /* Measure the taper ratio as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::taper_ratio(wing), (1.0/3.0), 1e-3);
}

/**
 * @brief Measure the mean aerodynamic chord of a planar wing surface.
 */
TEST_F(MeasurePlanarWing, MeanAerodynamicChord)
{
    /* Measure the mean aerodynamic chord of the wing */
    EXPECT_NEAR(geom2::measure::mean_aerodynamic_chord(wing), 1.777, 1e-3);

    /* Measure the mean aerodynamic chord as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::mean_aerodynamic_chord(wing), 1.777, 1e-3);
}

/**
 * @brief Measure the span position of the mean aerodynamic chord of a planar wing surface.
 */
TEST_F(MeasurePlanarWing, MeanAerodynamicChordPosition)
{
    /* Measure the mean aerodynamic chord position of the wing */
    EXPECT_NEAR(geom2::measure::mean_aerodynamic_chord_position(wing), -1.555, 1e-3);

    /* Measure the mean aerodynamic chord position as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::mean_aerodynamic_chord_position(wing), -1.555, 1e-3);

    /* Test when the main extrusion is in positive local Z direction */
    wing.is_symmetric = false;
    wing.sections[1].origin = geom2::Point_3(0, 0, 2);
    wing.sections[2].origin = geom2::Point_3(0, 0, 4);
    EXPECT_NEAR(geom2::measure::mean_aerodynamic_chord_position(wing), 1.555, 1e-3);

    /* Measure the mean aerodynamic chord position as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::mean_aerodynamic_chord_position(wing), 1.555, 1e-3);
}

/**
 * @brief Measure the span position of the mean aerodynamic chord of a wing surface
 * with dihedral.
 */
TEST_F(MeasureWingWithDihedral, MeanAerodynamicChordPosition)
{
    /* Increase the initial dihedral to increase its effect on the segment area */
    wing.sections.back().origin = geom2::Point_3(0, 4, -4);

    /* Measure the mean aerodynamic chord position of the wing */
    EXPECT_NEAR(geom2::measure::mean_aerodynamic_chord_position(wing), -1.555, 1e-3);

    /* Measure the mean aerodynamic chord position as a symmetric wing */
    wing.is_symmetric = true;
    EXPECT_NEAR(geom2::measure::mean_aerodynamic_chord_position(wing), -1.555, 1e-3);
}

/**
 * @brief Measure the geometric centers of a polygon surface.
 */
TEST_F(MeasurePolygon, Centroids)
{
    /* Create a segment */
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections[0].origin = {1, 0, 0};
    surface.sections[0].set_width(2.0);
    surface.sections[1].origin = {0, 0.5, 1};
    surface.sections[1].set_width(1.0);
    surface.sections[2].origin = {0, 0, 2};
    surface.sections[2].set_width(1.0);

    /* Measure the centroids */
    std::vector<geom2::Point_3> centroids = geom2::measure::centroids(surface);

    /* Check the centroids */
    ASSERT_EQ(centroids.size(), surface.sections.size() - 1);
    EXPECT_EQ(centroids[0], geom2::Point_3(0.5, 0.25, 0.5));
    EXPECT_EQ(centroids[1], geom2::Point_3(0.0, 0.25, 1.5));
}

/**
 * @brief Test measuring the geometric centers of a planar wing
 */
TEST_F(MeasurePlanarWing, Centroids)
{
    /* Measure the centroid */
    std::vector<geom2::Point_3> centroids = geom2::measure::centroids(wing);

    /* Check the centroids */
    ASSERT_EQ(centroids.size(), wing.sections.size() - 1);
    EXPECT_EQ(centroids[0], geom2::Point_3(1.0, 0, -1));
    EXPECT_EQ(centroids[1], geom2::Point_3(0.5, 0, -3));
}

/**
 * @brief Test the barycenter of a polygon surface.
 */
TEST_F(MeasurePolygon, Centroid)
{
    /* Create a segment */
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections[0].origin = {1, 0, 0};
    surface.sections[0].set_width(2.0);
    surface.sections[1].origin = {0, 0.5, 1};
    surface.sections[1].set_width(1.0);
    surface.sections[2].origin = {0, 0, 2};
    surface.sections[2].set_width(1.0);

    /* Measure the barycenter */
    geom2::Point_3 centroid = geom2::measure::centroid(surface);

    /* Check the barycenter */
    EXPECT_EQ(centroid, geom2::Point_3(0.25, 0.25, 1.0));
}

/**
 * @brief Test measuring the barycenter of a planar wing
 */
TEST_F(MeasurePlanarWing, Centroid)
{
    /* Measure the barycenter */
    geom2::Point_3 centroid = geom2::measure::centroid(wing);

    /* Check the barycenter */
    EXPECT_EQ(centroid, geom2::Point_3(0.75, 0, -2));

    /* Measure the barycenter as a symmetric wing */
    wing.is_symmetric = true;
    centroid = geom2::measure::centroid(wing);
    EXPECT_EQ(centroid, geom2::Point_3(0.75, 0, 0));
};

/**
 * @brief Test measuring the inertia of a simple polygon cube.
 */
TEST_F(MeasurePolygon, Inertia)
{
    /* Create the cube surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections[1].origin = {0, 0, 1};

    /* Get the inertia tensor in respect to the CG */
    auto I = geom2::measure::inertia(surface);

    /* Test the Ixx result */
    EXPECT_NEAR(I(0,0), 6.000, 1e-1);
    /* Test the Iyy result */
    EXPECT_NEAR(I(1,1), 6.000, 1e-1);
    /* Test the Izz result */
    EXPECT_NEAR(I(2,2), 10.667, 1e-1);

    /* Test the Ixy result */
    EXPECT_NEAR(I(0,1), 0.000, 1e-1);
    /* Test the Ixz result */
    EXPECT_NEAR(I(0,2), 0.000, 1e-1);
    /* Test the Iyz result */
    EXPECT_NEAR(I(1,2), 0.000, 1e-1);

    /* Test the Iyx result */
    EXPECT_NEAR(I(1,0), 0.000, 1e-1);
    /* Test the Izx result */
    EXPECT_NEAR(I(2,0), 0.000, 1e-1);
    /* Test the Izy result */
    EXPECT_NEAR(I(2,1), 0.000, 1e-1);
}

/**
 * @brief Test measuring the inertia of a simple planar wing.
 */
TEST_F(MeasurePlanarWing, Inertia)
{
    /* Get the inertia tensor */
    auto I = geom2::measure::inertia(wing);

    /* Test the Ixx result */
    EXPECT_NEAR(I(0,0), 17.560, 1e-1);
    /* Test the Iyy result */
    EXPECT_NEAR(I(1,1), 22.197, 1e-1);
    /* Test the Izz result */
    EXPECT_NEAR(I(2,2), 5.021, 1e-1);

    /* Test the Ixy result */
    EXPECT_NEAR(I(0,1), 0.147, 1e-1);
    /* Test the Ixz result */
    EXPECT_NEAR(I(0,2), 5.178, 1e-1);
    /* Test the Iyz result */
    EXPECT_NEAR(I(1,2), 0.506, 1e-1);

    /* Test the Iyx result */
    EXPECT_DOUBLE_EQ(I(1,0), I(0,1));
    /* Test the Izx result */
    EXPECT_DOUBLE_EQ(I(2,0), I(0,2));
    /* Test the Izy result */
    EXPECT_DOUBLE_EQ(I(2,1), I(1,2));

    /* Measure a symmetric wing */
    wing.is_symmetric = true;
    auto inertia_sym = geom2::measure::inertia(wing);
    EXPECT_EQ(I, inertia_sym);
}

/**
 * @brief Test measuring the center of a polygon surface.
 */
TEST_F(MeasureCone, Center)
{
    /* Measure center at z = 0.0 */
    geom2::Point_3 center = geom2::measure::center(cone, 0.0);
    EXPECT_NEAR(center.x(), 0.0, 1e-3);
    EXPECT_NEAR(center.y(), 0.0, 1e-3);
    EXPECT_NEAR(center.z(), 0.0, 1e-3);

    /* Measure center at z = 1.0 */
    center = geom2::measure::center(cone, 1.0);
    EXPECT_NEAR(center.x(), 0.0, 1e-3);
    EXPECT_NEAR(center.y(), 0.25 / 3, 1e-3);
    EXPECT_NEAR(center.z(), 1.0, 1e-3);

    /* Measure center at z = 1.5 */
    center = geom2::measure::center(cone, 1.5);
    EXPECT_NEAR(center.x(), 0.0, 1e-3);
    EXPECT_NEAR(center.y(), 0.125, 1e-3);
    EXPECT_NEAR(center.z(), 1.5, 1e-3);

    /* Measure center at z = 2.0 */
    center = geom2::measure::center(cone, 2.0);
    EXPECT_NEAR(center.x(), 0.0, 1e-3);
    EXPECT_NEAR(center.y(), 0.50 / 3, 1e-3);
    EXPECT_NEAR(center.z(), 2.0, 1e-3);

    /* Measure center at z = 3.0 */
    center = geom2::measure::center(cone, 3.0);
    EXPECT_NEAR(center.x(), 0.0, 1e-3);
    EXPECT_NEAR(center.y(), 0.25, 1e-3);
    EXPECT_NEAR(center.z(), 3.0, 1e-3);
}

/**
 * @brief Test measuring the top of a polygon surface.
 */
TEST_F(MeasureCone, Top)
{
    /* Measure the top at z = 0.0 */
    geom2::Point_3 top = geom2::measure::top(cone, 0.0);
    EXPECT_NEAR(top.x(), 0.0, 1e-3);
    EXPECT_NEAR(top.y(), 0.5, 1e-3);
    EXPECT_NEAR(top.z(), 0.0, 1e-3);

    /* Measure the top at z = 1.0 */
    top = geom2::measure::top(cone, 1.0);
    EXPECT_NEAR(top.x(), 0.0, 1e-3);
    EXPECT_NEAR(top.y(), 0.5, 1e-3);
    EXPECT_NEAR(top.z(), 1.0, 1e-3);

    /* Measure the top at z = 1.5 */
    top = geom2::measure::top(cone, 1.5);
    EXPECT_NEAR(top.x(), 0.0, 1e-3);
    EXPECT_NEAR(top.y(), 0.5, 1e-3);
    EXPECT_NEAR(top.z(), 1.5, 1e-3);

    /* Measure the top at z = 2.0 */
    top = geom2::measure::top(cone, 2.0);
    EXPECT_NEAR(top.x(), 0.0, 1e-3);
    EXPECT_NEAR(top.y(), 0.5, 1e-3);
    EXPECT_NEAR(top.z(), 2.0, 1e-3);

    /* Measure the top at z = 3.0 */
    top = geom2::measure::top(cone, 3.0);
    EXPECT_NEAR(top.x(), 0.0, 1e-3);
    EXPECT_NEAR(top.y(), 0.5, 1e-3);
    EXPECT_NEAR(top.z(), 3.0, 1e-3);
}

/**
 * @brief Test measuring the bottom of a polygon surface.
 */
TEST_F(MeasureCone, Bottom)
{
    /* Measure the bottom at z = 0.0 */
    geom2::Point_3 bottom = geom2::measure::bottom(cone, 0.0);
    EXPECT_NEAR(bottom.x(), 0.0, 1e-3);
    EXPECT_NEAR(bottom.y(), -0.5, 1e-3);
    EXPECT_NEAR(bottom.z(), 0.0, 1e-3);

    /* Measure the bottom at z = 1.0 */
    bottom = geom2::measure::bottom(cone, 1.0);
    EXPECT_NEAR(bottom.x(), 0.0, 1e-3);
    EXPECT_NEAR(bottom.y(), -1.0 / 3, 1e-3);
    EXPECT_NEAR(bottom.z(), 1.0, 1e-3);

    /* Measure the bottom at z = 1.5 */
    bottom = geom2::measure::bottom(cone, 1.5);
    EXPECT_NEAR(bottom.x(), 0.0, 1e-3);
    EXPECT_NEAR(bottom.y(), -0.25, 1e-3);
    EXPECT_NEAR(bottom.z(), 1.5, 1e-3);

    /* Measure the bottom at z = 2.0 */
    bottom = geom2::measure::bottom(cone, 2.0);
    EXPECT_NEAR(bottom.x(), 0.0, 1e-3);
    EXPECT_NEAR(bottom.y(), -1.0 / 6, 1e-3);
    EXPECT_NEAR(bottom.z(), 2.0, 1e-3);

    /* Measure the bottom at z = 3.0 */
    bottom = geom2::measure::bottom(cone, 3.0);
    EXPECT_NEAR(bottom.x(), 0.0, 1e-3);
    EXPECT_NEAR(bottom.y(), 0.0, 1e-3);
    EXPECT_NEAR(bottom.z(), 3.0, 1e-3);
}

/**
 * @brief Test measuring the left of a polygon surface.
 */
TEST_F(MeasureCone, Left)
{
    /* Measure the left at z = 0.0 */
    geom2::Point_3 left = geom2::measure::left(cone, 0.0);
    EXPECT_NEAR(left.x(), -1.0, 1e-3);
    EXPECT_NEAR(left.y(), 0.0, 1e-3);
    EXPECT_NEAR(left.z(), 0.0, 1e-3);

    /* Measure the left at z = 1.0 */
    left = geom2::measure::left(cone, 1.0);
    EXPECT_NEAR(left.x(), -0.833, 1e-3);
    EXPECT_NEAR(left.y(), 0.25 / 3, 1e-3);
    EXPECT_NEAR(left.z(), 1.0, 1e-3);

    /* Measure the left at z = 1.5 */
    left = geom2::measure::left(cone, 1.5);
    EXPECT_NEAR(left.x(), -0.75, 1e-3);
    EXPECT_NEAR(left.y(), 0.125, 1e-3);
    EXPECT_NEAR(left.z(), 1.5, 1e-3);

    /* Measure the left at z = 2.0 */
    left = geom2::measure::left(cone, 2.0);
    EXPECT_NEAR(left.x(), -0.666, 1e-3);
    EXPECT_NEAR(left.y(), 0.166, 1e-3);
    EXPECT_NEAR(left.z(), 2.0, 1e-3);

    /* Measure the left at z = 3.0 */
    left = geom2::measure::left(cone, 3.0);
    EXPECT_NEAR(left.x(), -0.5, 1e-3);
    EXPECT_NEAR(left.y(), 0.25, 1e-3);
    EXPECT_NEAR(left.z(), 3.0, 1e-3);
}

/**
 * @brief Test measuring the right of a polygon surface.
 */
TEST_F(MeasureCone, Right)
{
    /* Measure the right at z = 0.0 */
    geom2::Point_3 right = geom2::measure::right(cone, 0.0);
    EXPECT_NEAR(right.x(), 1.0, 1e-3);
    EXPECT_NEAR(right.y(), 0.0, 1e-3);
    EXPECT_NEAR(right.z(), 0.0, 1e-3);

    /* Measure the right at z = 1.0 */
    right = geom2::measure::right(cone, 1.0);
    EXPECT_NEAR(right.x(), 0.833, 1e-3);
    EXPECT_NEAR(right.y(), 0.25 / 3, 1e-3);
    EXPECT_NEAR(right.z(), 1.0, 1e-3);

    /* Measure the right at z = 1.5 */
    right = geom2::measure::right(cone, 1.5);
    EXPECT_NEAR(right.x(), 0.75, 1e-3);
    EXPECT_NEAR(right.y(), 0.125, 1e-3);
    EXPECT_NEAR(right.z(), 1.5, 1e-3);

    /* Measure the right at z = 2.0 */
    right = geom2::measure::right(cone, 2.0);
    EXPECT_NEAR(right.x(), 0.666, 1e-3);
    EXPECT_NEAR(right.y(), 0.166, 1e-3);
    EXPECT_NEAR(right.z(), 2.0, 1e-3);

    /* Measure the right at z = 3.0 */
    right = geom2::measure::right(cone, 3.0);
    EXPECT_NEAR(right.x(), 0.5, 1e-3);
    EXPECT_NEAR(right.y(), 0.25, 1e-3);
    EXPECT_NEAR(right.z(), 3.0, 1e-3);
}

