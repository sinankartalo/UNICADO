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
#include <aircraftGeometry2/io/dat.h>
#include <aircraftGeometry2/geometry/factory.h>

/* Unit Under Test */
#include "aircraftGeometry2/processing/transform.h"

/* === Types ===*/
using Entity3D = geom2::Entity3D;
using Point_2 = geom2::Point_2;
using Point_3 = geom2::Point_3;
using Direction_3 = geom2::Direction_3;
using Polygon_2 = geom2::Polygon_2;
using PolygonSection = geom2::PolygonSection;

/* === Fixtures ===*/
class TransformPolygon : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Create Polygon and insert points resulting in a rotated square*/
        Polygon_2 polygon;
        polygon.push_back(Point_2{0, -1});
        polygon.push_back(Point_2{0.5, -0.5});
        polygon.push_back(Point_2{1, 0});
        polygon.push_back(Point_2{0.5, 0.5});
        polygon.push_back(Point_2{0, 1});
        polygon.push_back(Point_2{-0.5, 0.5});
        polygon.push_back(Point_2{-1, 0});
        polygon.push_back(Point_2{-0.5, -0.5});

        /* Create the section */
        section = PolygonSection{polygon};
    }

    /* === Members === */
    PolygonSection section{};
    Entity3D parent{};
};

/* === Tests === */
/**
 * @brief Test transforming a 2D point to the parent coordinate system.
 * of the entity.
 */
TEST(TransformPoint2DTo3D, Translation)
{
    /* Create the properties */
    Point_3 location(1, 2, 3);
    Direction_3 normal(0, 0, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Create the point to transform */
    Point_2 point(1, 2);

    /* Convert the point */
    Point_3 point_transformed = geom2::transform::to_parent(entity, point);

    /* Check the result */
    EXPECT_EQ(point_transformed, Point_3(2, 4, 3));
}

/**
 * @brief Test transforming a 3D point to the parent coordinate system.
 * when the normal direction is not the z-axis.
 */
TEST(TransformPoint2DTo3D, RotateAroundX)
{
    /* Create the properties */
    Point_3 location(0, 0, 0);
    Direction_3 normal(0, 1, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Create the point to transform */
    Point_2 point_x(1, 0);
    Point_2 point_y(0, 1);

    /* X coordinates should not be changed */
    Point_3 point_transformed = geom2::transform::to_parent(entity, point_x);
    EXPECT_EQ(point_transformed, Point_3(1, 0, 0));

    /* Y Coordinates should be changed */
    point_transformed = geom2::transform::to_parent(entity, point_y);
    EXPECT_NEAR(point_transformed.x(), 0, 1e-6);
    EXPECT_NEAR(point_transformed.y(), 0.707, 1e-3);
    EXPECT_NEAR(point_transformed.z(), -0.707, 1e-3);
}

/**
 * @brief Test transforming a 3D point to the parent coordinate system.
 */
TEST(TransformPoint3DTo3D, Translation)
{
    /* Create the properties */
    Point_3 location(1, 2, 3);
    Direction_3 normal(0, 0, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Create the point to transform */
    const Point_3 point_local(1, 2, 3);

    /* Convert the point */
    Point_3 point_transformed = geom2::transform::to_parent(entity, point_local);

    /* Check the result */
    EXPECT_EQ(point_transformed, Point_3(2, 4, 6));
}

/**
 * @brief Test transforming a 3D point back to the local coordinate system.
 */
TEST(TransformToLocal, Translation)
{
    /* Create the properties */
    Point_3 location(1, 2, 3);
    Direction_3 normal(0, 0, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Create the point to transform */
    const Point_3 point_global(2, 4, 6);

    /* Convert the point */
    Point_3 point_local = geom2::transform::to_local(entity, point_global);

    /* Check the result */
    EXPECT_EQ(point_local, Point_3(1, 2, 3));
}

/**
 * @brief Test transforming a 3D point to the parent coordinate system.
 */
TEST(TransformPoint3DTo3D, RotateAroundX)
{
    /* Create an entity with a positive rotation around x */
    Point_3 location(1, 2, 3);
    Direction_3 normal(0, 1, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Create the point to transform */
    const Point_3 point_local(1, 2, 3);

    /* Check result */
    Point_3 point_transformed = geom2::transform::to_parent(entity, point_local);
    EXPECT_NEAR(point_transformed.x(), 2, 1e-3);
    EXPECT_NEAR(point_transformed.y(), 5.536, 1e-3);
    EXPECT_NEAR(point_transformed.z(), 3.707, 1e-3);

    /* Rotate in the other direction */
    entity.normal = Direction_3(0, -1, 1);
    point_transformed = geom2::transform::to_parent(entity, point_local);
    EXPECT_NEAR(point_transformed.x(), 2, 1e-3);
    EXPECT_NEAR(point_transformed.y(), 1.293, 1e-3);
    EXPECT_NEAR(point_transformed.z(), 6.536, 1e-3);

    /* Test +90 deg rotation */
    entity.normal = Direction_3(0, 1, 0);
    point_transformed = geom2::transform::to_parent(entity, point_local);
    EXPECT_NEAR(point_transformed.x(), 2, 1e-3);
    EXPECT_NEAR(point_transformed.y(), 5, 1e-3);
    EXPECT_NEAR(point_transformed.z(), 1, 1e-3);

    /* Test +135 deg rotation */
    entity.normal = Direction_3(0, 1, -1);
    point_transformed = geom2::transform::to_parent(entity, point_local);
    EXPECT_NEAR(point_transformed.x(), 2, 1e-3);
    EXPECT_NEAR(point_transformed.y(), 2.707, 1e-3);
    EXPECT_NEAR(point_transformed.z(), -0.535, 1e-3);
}

/**
 * @brief Test transforming a 3D point to the local coordinate system.
 */
TEST(TransformToLocal, RotateAroundX)
{
    /* Create an entity with a positive rotation around x */
    Point_3 location(1, 2, 3);
    Direction_3 normal(0, 1, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Create the point to transform */
    Point_3 point_global(2.0, 5.536, 3.707);

    /* Check result */
    Point_3 point_local = geom2::transform::to_local(entity, point_global);
    EXPECT_NEAR(point_local.x(), 1.0, 1e-3);
    EXPECT_NEAR(point_local.y(), 2.0, 1e-3);
    EXPECT_NEAR(point_local.z(), 3.0, 1e-3);

    /* Rotate in the other direction */
    entity.normal = Direction_3(0, -1, 1);
    point_global = Point_3(2.0, 1.293, 6.536);
    point_local = geom2::transform::to_local(entity, point_global);
    EXPECT_NEAR(point_local.x(), 1.0, 1e-3);
    EXPECT_NEAR(point_local.y(), 2.0, 1e-3);
    EXPECT_NEAR(point_local.z(), 3.0, 1e-3);

    /* Test +90 deg rotation */
    entity.normal = Direction_3(0, 1, 0);
    point_global = Point_3(2.0, 5.0, 1.0);
    point_local = geom2::transform::to_local(entity, point_global);
    EXPECT_NEAR(point_local.x(), 1.0, 1e-3);
    EXPECT_NEAR(point_local.y(), 2.0, 1e-3);
    EXPECT_NEAR(point_local.z(), 3.0, 1e-3);

    /* Test +135 deg rotation */
    entity.normal = Direction_3(0, 1, -1);
    point_global = Point_3(2.0, 2.707, -0.535);
    point_local = geom2::transform::to_local(entity, point_global);
    EXPECT_NEAR(point_local.x(), 1.0, 1e-3);
    EXPECT_NEAR(point_local.y(), 2.0, 1e-3);
    EXPECT_NEAR(point_local.z(), 3.0, 1e-3);
}

/**
 * @brief Test transforming a 3D point to the parent coordinate system.
 */
TEST(TransformPoint3DTo3D, RotateAroundY)
{
    /* Create an entity with a positive rotation around y */
    Point_3 location(1, 2, 3);
    Direction_3 normal(1, 0, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Create the point to transform */
    const Point_3 point_local(1, 2, 3);

    /* Check result */
    Point_3 point_transformed = geom2::transform::to_parent(entity, point_local);
    EXPECT_NEAR(point_transformed.x(), 1.707, 1e-3);
    EXPECT_NEAR(point_transformed.y(), 3.0, 1e-3);
    EXPECT_NEAR(point_transformed.z(), 6.536, 1e-3);

    /* Rotate in the other direction */
    entity.normal = Direction_3(-1, 0, 1);
    point_transformed = geom2::transform::to_parent(entity, point_local);
    EXPECT_NEAR(point_transformed.x(), -2.536, 1e-3);
    EXPECT_NEAR(point_transformed.y(), 3, 1e-3);
    EXPECT_NEAR(point_transformed.z(), 3.707, 1e-3);

    /* Test +135 deg rotation */
    entity.normal = Direction_3(1, 0, -1);
    point_transformed = geom2::transform::to_parent(entity, point_local);
    EXPECT_NEAR(point_transformed.x(), 4.536, 1e-3);
    EXPECT_NEAR(point_transformed.y(), 3.0, 1e-3);
    EXPECT_NEAR(point_transformed.z(), 2.293, 1e-3);
}

/**
 * @brief Test transforming a 3D point to the local coordinate system.
 */
TEST(TransformToLocal, RotateAroundY)
{
    /* Create an entity with a positive rotation around y */
    Point_3 location(1, 2, 3);
    Direction_3 normal(1, 0, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};

    /* Create the point to transform */
    Point_3 point_global(1.707, 3.0, 6.536);

    /* Check result */
    Point_3 point_local = geom2::transform::to_local(entity, point_global);
    EXPECT_NEAR(point_local.x(), 1.0, 1e-3);
    EXPECT_NEAR(point_local.y(), 2.0, 1e-3);
    EXPECT_NEAR(point_local.z(), 3.0, 1e-3);

    /* Rotate in the other direction */
    entity.normal = Direction_3(-1, 0, 1);
    point_global = Point_3(-2.536, 3, 3.707);
    point_local = geom2::transform::to_local(entity, point_global);
    EXPECT_NEAR(point_local.x(), 1.0, 1e-3);
    EXPECT_NEAR(point_local.y(), 2.0, 1e-3);
    EXPECT_NEAR(point_local.z(), 3.0, 1e-3);

    /* Test +135 deg rotation */
    entity.normal = Direction_3(1, 0, -1);
    point_global = Point_3(4.536, 3.0, 2.293);
    point_local = geom2::transform::to_local(entity, point_global);
    EXPECT_NEAR(point_local.x(), 1.0, 1e-3);
    EXPECT_NEAR(point_local.y(), 2.0, 1e-3);
    EXPECT_NEAR(point_local.z(), 3.0, 1e-3);
}

/**
 * @brief Test transforming a 3D point by introducing a rotation around z.
 */
TEST(TransformPoint3DTo3D, RotateAroundZ)
{
    /* Create an entity with a positive rotation around z */
    Point_3 location(1, 2, 3);
    Direction_3 normal(0, 0, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};
    entity.rotation_z = 0.7854; // 45 degree

    /* Create the point to transform */
    const Point_3 point_local(1, 2, 3);

    /* Check result */
    Point_3 point_transformed = geom2::transform::to_parent(entity, point_local);
    EXPECT_NEAR(point_transformed.x(), 0.293, 1e-3);
    EXPECT_NEAR(point_transformed.y(), 4.121, 1e-3);
    EXPECT_NEAR(point_transformed.z(), 6.0, 1e-3);

    /* Rotate in the other direction */
    // point_transformed = geom2::transform::toParent(entity, point_local);
    // EXPECT_NEAR(point_transformed.x(), 0.586, 1e-3);
    // EXPECT_NEAR(point_transformed.y(), 0.586, 1e-3);
    // EXPECT_NEAR(point_transformed.z(), 3.0, 1e-3);
}

/**
 * @brief Test transforming a 3D point to the local system by introducing a rotation around z.
 */
TEST(TransformToLocal, RotateAroundZ)
{
    /* Create an entity with a positive rotation around z */
    Point_3 location(1, 2, 3);
    Direction_3 normal(0, 0, 1);
    std::string name("test");

    /* Create default entity */
    Entity3D entity{name, location, normal};
    entity.rotation_z = 0.7854; // 45 degree

    /* Create the point to transform */
    Point_3 point_global(0.293, 4.121, 6.0);

    /* Check result */
    Point_3 point_local = geom2::transform::to_local(entity, point_global);
    EXPECT_NEAR(point_local.x(), 1.0, 1e-3);
    EXPECT_NEAR(point_local.y(), 2.0, 1e-3);
    EXPECT_NEAR(point_local.z(), 3.0, 1e-3);
}

/**
 * @brief Test converting two polygon sections to a surface mesh.
 *
 */
TEST(TransformToSurfaceMesh, TwoSections)
{
    /* Create the shape */
    Polygon_2 shape;
    shape.push_back(Point_2{-1, -1});
    shape.push_back(Point_2{1, -1});
    shape.push_back(Point_2{1, 1});
    shape.push_back(Point_2{-1, 1});

    /* Create the sections */
    std::vector<PolygonSection> sections;
    sections.emplace_back(shape);
    sections.emplace_back(shape);

    /* Position the sections */
    sections[0].origin = {0, 0, 0};
    sections[1].origin = {0, 0, 1};

    /* Convert the sections */
    geom2::Mesh mesh = geom2::transform::to_mesh(sections);

    /* Check the result */
    EXPECT_EQ(mesh.number_of_vertices(), 8);
    EXPECT_EQ(mesh.number_of_faces(), 8);
}

/**
 * @brief Test converting two polygon sections which form a pyramid to a surface mesh.
 *
 */
TEST(TransformToSurfaceMesh, PyramidTwoSections)
{
    /* Create the shapes */
    Polygon_2 tip;
    tip.push_back(Point_2{0, 0});
    Polygon_2 shape;
    shape.push_back(Point_2{-1, -1});
    shape.push_back(Point_2{1, -1});
    shape.push_back(Point_2{1, 1});
    shape.push_back(Point_2{-1, 1});

    /* Create the sections */
    std::vector<PolygonSection> sections;
    sections.emplace_back(tip);
    sections.emplace_back(shape);

    /* Position the sections */
    sections[0].origin = {0, 0, 0};
    sections[1].origin = {0, 0, 1};

    /* Convert the sections */
    geom2::Mesh mesh = geom2::transform::to_mesh(sections);

    /* Check the result */
    EXPECT_EQ(mesh.number_of_vertices(), 5);
    EXPECT_EQ(mesh.number_of_faces(), 4);
}

/**
 * @brief Test converting three polygon sections to a surface mesh.
 *
 */
TEST(TransformToSurfaceMesh, ThreeSections)
{
    /* Create the shape */
    Polygon_2 shape;
    shape.push_back(Point_2{-1, -1});
    shape.push_back(Point_2{1, -1});
    shape.push_back(Point_2{1, 1});
    shape.push_back(Point_2{-1, 1});

    /* Create the sections */
    std::vector<PolygonSection> sections;
    sections.emplace_back(shape);
    sections.emplace_back(shape);
    sections.emplace_back(shape);

    /* Position the sections */
    sections[0].origin = {0, 0, 0};
    sections[1].origin = {0, 0, 1};
    sections[2].origin = {0, 0, 2};

    /* Convert the sections */
    geom2::Mesh mesh = geom2::transform::to_mesh(sections);

    /* Check the result */
    EXPECT_EQ(mesh.number_of_vertices(), 12);
    EXPECT_EQ(mesh.number_of_faces(), 16);
}

/**
 * @brief Test converting sections which have a different count of vertices
 * to a surface mesh.
 *
 */
TEST(TransformToSurfaceMesh, SectionsWithDifferentVertexCount)
{
    /* Create the shapes */
    Polygon_2 square;
    square.push_back(Point_2{-1, -1});
    square.push_back(Point_2{1, -1});
    square.push_back(Point_2{1, 1});
    square.push_back(Point_2{-1, 1});
    Polygon_2 single_point;
    single_point.push_back(Point_2{0, 0});

    /* Create the sections */
    std::vector<PolygonSection> sections;
    sections.emplace_back(square);
    sections.emplace_back(single_point);

    /* Position the sections */
    sections[0].origin = {0, 0, 0};
    sections[1].origin = {0, 0, 1};

    /* Convert the sections */
    geom2::Mesh mesh = geom2::transform::to_mesh(sections);

    /* Check the result */
    EXPECT_EQ(mesh.number_of_vertices(), 5);
    EXPECT_EQ(mesh.number_of_faces(), 4);
}

/**
 * @brief Test getting the top reflection point of a section.
 */
TEST_F(TransformPolygon, GetReflectionPointTop)
{
    /* Test top outline point in x direction */
    Point_3 top_reflection = geom2::transform::get_reflection_point_top(section, parent, geom2::Direction_3(1, 0, 0));
    EXPECT_EQ(top_reflection, Point_3(0, 1, 0));

    /* Test top outline point in y direction */
    top_reflection = geom2::transform::get_reflection_point_top(section, parent, geom2::Direction_3(0, 1, 0));
    EXPECT_EQ(top_reflection, Point_3(-1, 0, 0));

    /* Test when the parent has an offset in x direction */
    parent.origin = Point_3(1, 0, 0);
    top_reflection = geom2::transform::get_reflection_point_top(section, parent, geom2::Direction_3(1, 0, 0));
    EXPECT_EQ(top_reflection, Point_3(1, 1, 0));

    /* Test when the parent has an offset in y direction */
    parent.origin = Point_3(0, 1, 0);
    top_reflection = geom2::transform::get_reflection_point_top(section, parent, geom2::Direction_3(0, 1, 0));
    EXPECT_EQ(top_reflection, Point_3(-1, 1, 0));

    /* Test when the parent has an offset in z direction */
    parent.origin = Point_3(0, 0, 1);
    top_reflection = geom2::transform::get_reflection_point_top(section, parent, geom2::Direction_3(1, 0, 0));
    EXPECT_EQ(top_reflection, Point_3(0, 1, 1));

    /* Test top outline in 45 degree */
    parent.origin = Point_3(0, 0, 0);
    top_reflection = geom2::transform::get_reflection_point_top(section, parent, geom2::Direction_3(1, 1, 0));
    EXPECT_EQ(top_reflection, Point_3(0, 1, 0));

    /* Change the normal direction of the parent */
    parent.normal = geom2::Direction_3(0, 1, 1);
    top_reflection = geom2::transform::get_reflection_point_top(section, parent, geom2::Direction_3(1, 0, 0));
    EXPECT_NEAR(top_reflection.x(), 0, 1e-3);
    EXPECT_NEAR(top_reflection.y(), 0.707, 1e-3);
    EXPECT_NEAR(top_reflection.z(), -0.707, 1e-3);

    /*
    * Create Polygon and insert points resulting in a rotated square.
    * The order of the points mimics the order of the circle dat file.
    * This order did introduce a bug when getting the reflection point.
    */
    Polygon_2 polygon_asymmetric;
    polygon_asymmetric.push_back(Point_2{1, 0});
    polygon_asymmetric.push_back(Point_2{0.5, 0.5});
    polygon_asymmetric.push_back(Point_2{0, 1});
    polygon_asymmetric.push_back(Point_2{-0.5, 0.5});
    polygon_asymmetric.push_back(Point_2{-1, 0});
    polygon_asymmetric.push_back(Point_2{-0.5, -0.5});
    polygon_asymmetric.push_back(Point_2{0, -1.5});
    polygon_asymmetric.push_back(Point_2{0.5, -0.5});

    /* Create the section */
    parent.origin = Point_3(0, 0, 0);
    parent.normal = geom2::Direction_3(0, 0, 1);
    section = PolygonSection{polygon_asymmetric};

    /* Test top outline point in x direction */
    top_reflection = geom2::transform::get_reflection_point_top(section, parent, geom2::Direction_3(1, 0, 0));
    EXPECT_EQ(top_reflection, Point_3(0, 1, 0));
}

/**
 * @brief Test getting the bottom reflection point of a section.
 */
TEST_F(TransformPolygon, GetReflectionPointBottom)
{
    /* Test bottom outline point in x direction */
    Point_3 bottom_reflection = geom2::transform::get_reflection_point_bottom(section, parent, geom2::Direction_3(1, 0, 0));
    EXPECT_EQ(bottom_reflection, Point_3(0, -1, 0));

    /* Test bottom outline point in y direction */
    bottom_reflection = geom2::transform::get_reflection_point_bottom(section, parent, geom2::Direction_3(0, 1, 0));
    EXPECT_EQ(bottom_reflection, Point_3(1, 0, 0));

    /* Test when the parent has an offset in x direction */
    parent.origin = Point_3(1, 0, 0);
    bottom_reflection = geom2::transform::get_reflection_point_bottom(section, parent, geom2::Direction_3(1, 0, 0));
    EXPECT_EQ(bottom_reflection, Point_3(1, -1, 0));

    /* Test when the parent has an offset in y direction */
    parent.origin = Point_3(0, 1, 0);
    bottom_reflection = geom2::transform::get_reflection_point_bottom(section, parent, geom2::Direction_3(0, 1, 0));
    EXPECT_EQ(bottom_reflection, Point_3(1, 1, 0));

    /* Test when the parent has an offset in z direction */
    parent.origin = Point_3(0, 0, 1);
    bottom_reflection = geom2::transform::get_reflection_point_bottom(section, parent, geom2::Direction_3(1, 0, 0));
    EXPECT_EQ(bottom_reflection, Point_3(0, -1, 1));

    /* Test bottom outline in 45 degree */
    parent.origin = Point_3(0, 0, 0);
    bottom_reflection = geom2::transform::get_reflection_point_bottom(section, parent, geom2::Direction_3(1, 1, 0));
    EXPECT_EQ(bottom_reflection, Point_3(0, -1, 0));
}

/**
 * @brief Test getting the outline of a surface.
 */
TEST_F(TransformPolygon, GetOutline3D)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(section);
    surface.sections.emplace_back(section);
    surface.sections.emplace_back(section);

    /* Position the sections */
    surface.sections[0].origin = {0, 0, 0};
    surface.sections[1].origin = {0, 0, 1};
    surface.sections[2].origin = {0, 0, 2};

    /* Get the outline when viewed from the x direction */
    std::vector<geom2::Point_3> outline = geom2::transform::outline_3d(surface, geom2::Direction_3(1, 0, 0));

    /* Check the result */
    EXPECT_EQ(outline.size(), 6);
    EXPECT_EQ(outline[0], Point_3(0, 1, 0));
    EXPECT_EQ(outline[1], Point_3(0, 1, 1));
    EXPECT_EQ(outline[2], Point_3(0, 1, 2));
    EXPECT_EQ(outline[3], Point_3(0, -1, 2));
    EXPECT_EQ(outline[4], Point_3(0, -1, 1));
    EXPECT_EQ(outline[5], Point_3(0, -1, 0));

    /* Get the outline when viewed from the y direction */
    outline.clear();
    outline = geom2::transform::outline_3d(surface, geom2::Direction_3(0, 1, 0));

    /* Check the result */
    EXPECT_EQ(outline.size(), 6);
    EXPECT_EQ(outline[0], Point_3(-1, 0, 0));
    EXPECT_EQ(outline[1], Point_3(-1, 0, 1));
    EXPECT_EQ(outline[2], Point_3(-1, 0, 2));
    EXPECT_EQ(outline[3], Point_3(1, 0, 2));
    EXPECT_EQ(outline[4], Point_3(1, 0, 1));
    EXPECT_EQ(outline[5], Point_3(1, 0, 0));
}

/**
 * @brief Test getting the outline of a surface projected to 2D.
 */
TEST_F(TransformPolygon, GetOutline2D)
{
    /* Create the surface */
    geom2::MultisectionSurface<PolygonSection> surface;
    surface.sections.emplace_back(section);
    surface.sections.emplace_back(section);
    surface.sections.emplace_back(section);

    /* Position the sections */
    surface.sections[0].origin = {0, 0, 0};
    surface.sections[1].origin = {0, 0, 1};
    surface.sections[2].origin = {0, 0, 2};

    /* Get the outline when viewed from the x direction */
    std::vector<geom2::Point_2> outline = geom2::transform::outline_2d(surface, geom2::Direction_3(1, 0, 0));

    /* Check the result */
    EXPECT_EQ(outline.size(), 6);
    EXPECT_EQ(outline[0], Point_2(1, 0));
    EXPECT_EQ(outline[1], Point_2(1, 1));
    EXPECT_EQ(outline[2], Point_2(1, 2));
    EXPECT_EQ(outline[3], Point_2(-1, 2));
    EXPECT_EQ(outline[4], Point_2(-1, 1));
    EXPECT_EQ(outline[5], Point_2(-1, 0));

    /* Get the outline when viewed from the y direction */
    outline.clear();
    outline = geom2::transform::outline_2d(surface, geom2::Direction_3(0, 1, 0));

    /* Check the result */
    EXPECT_EQ(outline.size(), 6);
    EXPECT_EQ(outline[0], Point_2(-1, 0));
    EXPECT_EQ(outline[1], Point_2(-1, -1));
    EXPECT_EQ(outline[2], Point_2(-1, -2));
    EXPECT_EQ(outline[3], Point_2(1, -2));
    EXPECT_EQ(outline[4], Point_2(1, -1));
    EXPECT_EQ(outline[5], Point_2(1, -0));

    /* Get the outline when viewed from 45 degree */
    outline.clear();
    outline = geom2::transform::outline_2d(surface, geom2::Direction_3(1, 1, 0));

    /* Check the result */
    EXPECT_EQ(outline.size(), 6);
    EXPECT_NEAR(outline[0].x(), 0.000, 1e-3);
    EXPECT_NEAR(outline[0].y(), -0.707, 1e-3);
    EXPECT_NEAR(outline[1].x(), 1.000, 1e-3);
    EXPECT_NEAR(outline[1].y(), -0.707, 1e-3);
    EXPECT_NEAR(outline[2].x(), 2.000, 1e-3);
    EXPECT_NEAR(outline[2].y(), -0.707, 1e-3);
    EXPECT_NEAR(outline[3].x(), 2.000, 1e-3);
    EXPECT_NEAR(outline[3].y(), 0.707, 1e-3);
    EXPECT_NEAR(outline[4].x(), 1.000, 1e-3);
    EXPECT_NEAR(outline[4].y(), 0.707, 1e-3);
    EXPECT_NEAR(outline[5].x(), 0.000, 1e-3);
    EXPECT_NEAR(outline[5].y(), 0.707, 1e-3);

    /* Get the outline when the surface is rotated by 45 degrees */
    outline.clear();
    surface.normal = geom2::Direction_3(0, 1, 1);
    outline = geom2::transform::outline_2d(surface, geom2::Direction_3(1, 0, 0));

    /* Check the result */
    EXPECT_EQ(outline.size(), 6);
    EXPECT_NEAR(outline[0].x(), 0.707, 1e-3);
    EXPECT_NEAR(outline[0].y(), -0.707, 1e-3);
}

/**
 * @brief Test transforming a surface which consists of normalized coordinates
 * to absolute coordinates using a paren surface.
 */
TEST(TransformToAbsolute, PolygonSurfaceWithAirfoilSurface)
{
    /* Test stubs dir */
    std::filesystem::path stubs_dir{CMAKE_TEST_STUBS_DIR};

    /* Create a simple wing */
    geom2::SectionBuilder<geom2::AirfoilSection> builder;
    geom2::MultisectionSurface<geom2::AirfoilSection> wing;
    auto airfoil = geom2::io::read_airfoil(stubs_dir / "dat-files/n0012-tab.dat");
    builder.arrange(airfoil, {1, 0, -5}, 3);
    wing.sections = builder.get_result();
    wing.sections[0].set_chord_length(5.0);
    wing.sections[1].set_chord_length(4.0);
    wing.sections[2].set_chord_length(3.0);

    /* Create a polygon which is in relative coordinates */
    Polygon_2 polygon;
    polygon.push_back(Point_2{0.2, 0.0});
    polygon.push_back(Point_2{0.8, 0.0});
    geom2::MultisectionSurface<PolygonSection> polygon_surface;
    polygon_surface.origin = {1, 2, 3};
    polygon_surface.normal = {1, 1, 1};
    polygon_surface.sections.emplace_back(polygon);
    polygon[0] = Point_2{0.4, 0.0};
    polygon_surface.sections.emplace_back(polygon);
    polygon_surface.sections.back().origin = {0, 0, -0.5};

    /* Get the polygon surface in absolute coordinates based on the wing shape */
    auto polygon_absolute = geom2::transform::to_absolute(polygon_surface, wing);

    /* The origin and normal should be the same as for the initial surface*/
    EXPECT_EQ(polygon_absolute.origin, polygon_surface.origin);
    EXPECT_EQ(polygon_absolute.normal, polygon_surface.normal);

    /* Test the coordinates of the first resulting section */
    auto shape = polygon_absolute.sections[0].get_contour(true);
    EXPECT_EQ(polygon_absolute.sections[0].origin, Point_3(0, 0, 0));
    EXPECT_EQ(shape[0], Point_2(0.2*5.0, 0));
    EXPECT_EQ(shape[1], Point_2(0.8*5.0, 0));

    /* Test the coordinates of the second resulting section */
    shape = polygon_absolute.sections[1].get_contour(true);
    EXPECT_EQ(polygon_absolute.sections[1].origin, Point_3(0, 0, -5));
    EXPECT_EQ(shape[0], Point_2(2.6, 0));
    EXPECT_EQ(shape[1], Point_2(4.2, 0));
    
    /* A symmetric wing should give the same result */
    wing.is_symmetric = true;
    polygon_absolute = geom2::transform::to_absolute(polygon_surface, wing);
    shape = polygon_absolute.sections[1].get_contour(true);
    EXPECT_EQ(polygon_absolute.sections[1].origin, Point_3(0, 0, -5));
    EXPECT_EQ(shape[0], Point_2(2.6, 0));
    EXPECT_EQ(shape[1], Point_2(4.2, 0));
}

/**
 * @brief Test resampling a polygon with two vertices to match a given amount of vertices.
 */
TEST(ResamplePolygon2D, ResampleTwoVertices)
{
    /* Create a simple polygon */
    Polygon_2 polygon;
    polygon.push_back(Point_2{0, 0});
    polygon.push_back(Point_2{1, 1});

    /* Resample the polygon with 2 vertices */
    auto resampled = geom2::transform::resample(polygon, 2);
    ASSERT_EQ(resampled.size(), 2);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_EQ(resampled[1], Point_2(1.0, 1.0));

    /* Resample the polygon with 3 vertices */
    resampled = geom2::transform::resample(polygon, 3);
    ASSERT_EQ(resampled.size(), 3);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_EQ(resampled[1], Point_2(0.5, 0.5));
    EXPECT_EQ(resampled[2], Point_2(1.0, 1.0));

    /* Resample the polygon with 3 vertices */
    resampled = geom2::transform::resample(polygon, 4);
    ASSERT_EQ(resampled.size(), 4);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_NEAR(resampled[1].x(), 0.333, 1e-3);
    EXPECT_NEAR(resampled[1].y(), 0.333, 1e-3);
    EXPECT_NEAR(resampled[2].x(), 0.666, 1e-3);
    EXPECT_NEAR(resampled[2].y(), 0.666, 1e-3);
    EXPECT_EQ(resampled[3], Point_2(1.0, 1.0));

    /* Resample the polygon with 5 vertices */
    resampled = geom2::transform::resample(polygon, 5);
    ASSERT_EQ(resampled.size(), 5);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_NEAR(resampled[1].x(), 0.25, 1e-3);
    EXPECT_NEAR(resampled[1].y(), 0.25, 1e-3);
    EXPECT_NEAR(resampled[2].x(), 0.5, 1e-3);
    EXPECT_NEAR(resampled[2].y(), 0.5, 1e-3);
    EXPECT_NEAR(resampled[3].x(), 0.75, 1e-3);
    EXPECT_NEAR(resampled[3].y(), 0.75, 1e-3);
    EXPECT_EQ(resampled[4], Point_2(1.0, 1.0));
}

/**
 * @brief Test resampling a polygon with three vertices to match a given amount of vertices.
 */
TEST(ResamplePolygon2D, ResampleThreeVertices)
{
    /* Create a simple polygon */
    Polygon_2 polygon;
    polygon.push_back(Point_2{0, 0});
    polygon.push_back(Point_2{1, 1});
    polygon.push_back(Point_2{2, 0});

    /* Resample the polygon with 5 vertices */
    auto resampled = geom2::transform::resample(polygon, 5);
    ASSERT_EQ(resampled.size(), 5);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_EQ(resampled[1], Point_2(0.5, 0.5));
    EXPECT_EQ(resampled[2], Point_2(1.0, 1.0));
    EXPECT_EQ(resampled[3], Point_2(1.5, 0.5));
    EXPECT_EQ(resampled[4], Point_2(2.0, 0.0));

    /* Resample the polygon with 4 vertices */
    resampled = geom2::transform::resample(polygon, 4);
    ASSERT_EQ(resampled.size(), 4);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_EQ(resampled[1], Point_2(1.0, 1.0));
    EXPECT_EQ(resampled[2], Point_2(1.5, 0.5));
    EXPECT_EQ(resampled[3], Point_2(2.0, 0.0));

    /* Resample the polygon with 6 vertices */
    resampled = geom2::transform::resample(polygon, 6);
    ASSERT_EQ(resampled.size(), 6);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_EQ(resampled[1], Point_2(0.5, 0.5));
    EXPECT_EQ(resampled[2], Point_2(1.0, 1.0));
    EXPECT_NEAR(resampled[3].x(), 1.333, 1e-3);
    EXPECT_NEAR(resampled[3].y(), 0.666, 1e-3);
    EXPECT_NEAR(resampled[4].x(), 1.666, 1e-3);
    EXPECT_NEAR(resampled[4].y(), 0.333, 1e-3);
    EXPECT_EQ(resampled[5], Point_2(2.0, 0.0));

    /* Resample the polygon with 7 vertices */
    resampled = geom2::transform::resample(polygon, 7);
    ASSERT_EQ(resampled.size(), 7);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_NEAR(resampled[1].x(), 0.333, 1e-3);
    EXPECT_NEAR(resampled[1].y(), 0.333, 1e-3);
    EXPECT_NEAR(resampled[2].x(), 0.666, 1e-3);
    EXPECT_NEAR(resampled[2].y(), 0.666, 1e-3);
    EXPECT_EQ(resampled[3], Point_2(1.0, 1.0));
    EXPECT_NEAR(resampled[4].x(), 1.333, 1e-3);
    EXPECT_NEAR(resampled[4].y(), 0.666, 1e-3);
    EXPECT_NEAR(resampled[5].x(), 1.666, 1e-3);
    EXPECT_NEAR(resampled[5].y(), 0.333, 1e-3);
    EXPECT_EQ(resampled[6], Point_2(2.0, 0.0));
}

/**
 * @brief Test resampling a polygon with four vertices to match a given amount of vertices.
 */
TEST(ResamplePolygon2D, ResampleFourVertices)
{
    /* Create a simple polygon */
    Polygon_2 polygon;
    polygon.push_back(Point_2{0, 0});
    polygon.push_back(Point_2{1, 1});
    polygon.push_back(Point_2{2, 1});
    polygon.push_back(Point_2{3, 0});

    /* Resample the polygon with 5 vertices */
    auto resampled = geom2::transform::resample(polygon, 5);
    ASSERT_EQ(resampled.size(), 5);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_EQ(resampled[1], Point_2(1.0, 1.0));
    EXPECT_EQ(resampled[2], Point_2(2.0, 1.0));
    EXPECT_EQ(resampled[3], Point_2(2.5, 0.5));
    EXPECT_EQ(resampled[4], Point_2(3.0, 0.0));

    /* Resample the polygon with 6 vertices */
    resampled = geom2::transform::resample(polygon, 6);
    ASSERT_EQ(resampled.size(), 6);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));
    EXPECT_EQ(resampled[1], Point_2(1.0, 1.0));
    EXPECT_EQ(resampled[2], Point_2(1.5, 1.0));
    EXPECT_EQ(resampled[3], Point_2(2.0, 1.0));
    EXPECT_EQ(resampled[4], Point_2(2.5, 0.5));
    EXPECT_EQ(resampled[5], Point_2(3.0, 0.0));
}

/**
 * @brief Test the unsuccessful cases when resampling a polygon
 */
TEST(ResamplePolygon2D, ResampleThrowsError)
{
    /* Create a simple polygon */
    Polygon_2 polygon;
    polygon.push_back(Point_2{0, 0});

    /* Nothing should change when the polygon just has one vertex */
    auto resampled = geom2::transform::resample(polygon, 5);
    ASSERT_EQ(resampled.size(), 1);
    EXPECT_EQ(resampled[0], Point_2(0.0, 0.0));

    /* Expect a throw when under sampling the polygon */
    polygon.push_back(Point_2{1, 1});
    polygon.push_back(Point_2{2, 1});
    EXPECT_THROW(geom2::transform::resample(polygon, 2), std::invalid_argument);
}
