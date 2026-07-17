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
#include "aircraftGeometry2/io/convert.h"

/* === Fixtures === */
class ConvertHullSurface : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Setup the base properties */
        surface.name = "some_name";
        surface.origin = {1, 2, 3};
        surface.normal = {0, 0, 1};

        /* Setup the shape for the sections */
        geom2::Polygon_2 shape;
        shape.push_back({0, -1});
        shape.push_back({1, 0});
        shape.push_back({0, 2});
        shape.push_back({-1, 0});
        geom2::PolygonSection section{shape};

        /* Setup the surface */
        surface.sections.emplace_back(section);
        surface.sections.emplace_back(section);
        surface.sections[0].origin = {0, 0, 0};
        surface.sections[0].set_width(2);
        surface.sections[1].origin = {0, 0, 1};
        surface.sections[1].set_height(6);
    }

    geom2::MultisectionSurface<geom2::PolygonSection> surface;
};

class ConvertAirfoilSurface : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Setup the base properties */
        surface.name = "main_wing";
        surface.origin = {1, 2, 3};
        surface.normal = {0, 0, 1};

        /* Setup the shape for the sections */
        geom2::Polygon_2 shape;
        shape.push_back({0, 0});
        shape.push_back({0.4, 0.2});
        shape.push_back({1, 0});
        shape.push_back({0, 0});
        geom2::AirfoilSection section{shape};

        /* Setup the surface */
        surface.sections.emplace_back(section);
        surface.sections.emplace_back(section);
        surface.sections[0].origin = {0, 0, 0};
        surface.sections[0].set_chord_length(3.0);
        surface.sections[0].scale_thickness(3.0);
        surface.sections[1].origin = {0, 0, 1};
        surface.sections[1].set_chord_length(2);
        surface.sections[1].set_twist_angle(0.1);
    }

    geom2::MultisectionSurface<geom2::AirfoilSection> surface;
};

/* === Tests === */

/**
 * @brief Test converting a poylgon surface as a nacelle node.
 */
TEST_F(ConvertHullSurface, ToAixmlNode)
{
    /* Treat the surface as a hull surface */
    geom2::io::SurfaceType nacelle = geom2::io::Hull{surface};

    /* Convert using the aixml format */
    node root{};
    node& result = std::visit(geom2::io::AixmlConverter{root, {"nacelle", "0", "fits my engine"}}, nacelle);

    /* Test the root properties */
    EXPECT_EQ(result.getName(), "nacelle");
    EXPECT_EQ(result.getStringAttrib("ID"), "0");
    EXPECT_EQ(result.getStringAttrib("description"), "fits my engine");

    /* Test the position node */
    EXPECT_EQ(double{result.at("position/x/value")}, surface.origin.x());
    EXPECT_EQ(double{result.at("position/y/value")}, surface.origin.y());
    EXPECT_EQ(double{result.at("position/z/value")}, surface.origin.z());

    /* Test the direction node */
    EXPECT_EQ(double{result.at("normal/x/value")}, surface.normal.dx());
    EXPECT_EQ(double{result.at("normal/y/value")}, surface.normal.dy());
    EXPECT_EQ(double{result.at("normal/z/value")}, surface.normal.dz());

    /* Test the section geometry */
    auto sections = result.getVector("sections/section");
    ASSERT_EQ(sections.size(), 2);
    EXPECT_EQ(sections[0]->getStringAttrib("ID"), "0");
    EXPECT_EQ(double{sections[0]->at("origin/x/value")}, surface.sections[0].origin.x());
    EXPECT_EQ(double{sections[0]->at("origin/y/value")}, surface.sections[0].origin.y());
    EXPECT_EQ(double{sections[0]->at("origin/z/value")}, surface.sections[0].origin.z());
    EXPECT_EQ(double{sections[0]->at("width/value")}, surface.sections[0].get_contour(true).bbox().x_span());
    EXPECT_EQ(double{sections[0]->at("height/value")}, surface.sections[0].get_contour(true).bbox().y_span());
    EXPECT_EQ(std::string(sections[0]->at("profile/value")), surface.sections[0].name);
}

/**
 * @brief Test converting a simple fuselage surface to an aixml node.
 */
using ConvertFuselageSurface = ConvertHullSurface; // Reuse the same test fixture of the hull surface
TEST_F(ConvertFuselageSurface, ToAixmlNode)
{
    /* Treat the surface as a hull surface */
    geom2::io::SurfaceType fuselage = geom2::io::Fuselage{surface};

    /* Convert using the aixml format */
    node root{};
    node& result = std::visit(geom2::io::AixmlConverter{root, {"fuselage", "0", "fits many people"}}, fuselage);

    /* Test the root properties */
    EXPECT_EQ(result.getName(), "fuselage");
    EXPECT_EQ(result.getStringAttrib("ID"), "0");
    EXPECT_EQ(result.getStringAttrib("description"), "fits many people");

    /* Test the name node */
    EXPECT_EQ(std::string(result.at("name/value")), surface.name);
    EXPECT_EQ(result.at("name").getStringAttrib("description"), "Name of the fuselage");

    /* Test the position node */
    EXPECT_EQ(double{result.at("position/x/value")}, surface.origin.x());
    EXPECT_EQ(double{result.at("position/y/value")}, surface.origin.y());
    EXPECT_EQ(double{result.at("position/z/value")}, surface.origin.z());

    /* Test the direction node */
    EXPECT_EQ(double{result.at("direction/x/value")}, surface.normal.dx());
    EXPECT_EQ(double{result.at("direction/y/value")}, surface.normal.dy());
    EXPECT_EQ(double{result.at("direction/z/value")}, surface.normal.dz());

    /* Test the section geometry */
    auto sections = result.getVector("sections/section");
    ASSERT_EQ(sections.size(), 2);
    EXPECT_EQ(sections[0]->getStringAttrib("ID"), "0");
    EXPECT_EQ(std::string(sections[0]->at("name/value")), "section_0");
    EXPECT_EQ(double{sections[0]->at("origin/x/value")}, surface.sections[0].origin.x());
    EXPECT_EQ(double{sections[0]->at("origin/y/value")}, surface.sections[0].origin.y());
    EXPECT_EQ(double{sections[0]->at("origin/z/value")}, surface.sections[0].origin.z());
    EXPECT_EQ(double{sections[0]->at("upper_height/value")}, surface.sections[0].get_contour(true).bbox().ymax());
    EXPECT_EQ(double{sections[0]->at("lower_height/value")}, std::abs(surface.sections[0].get_contour(true).bbox().ymin()));
    EXPECT_EQ(double{sections[0]->at("width/value")}, surface.sections[0].get_contour(true).bbox().x_span());
    EXPECT_EQ(std::string(sections[0]->at("section_shape/value")), surface.sections[0].name);
    EXPECT_EQ(sections[1]->getStringAttrib("ID"), "1");
    EXPECT_EQ(std::string(sections[1]->at("name/value")), "section_1");
    EXPECT_EQ(double{sections[1]->at("origin/x/value")}, surface.sections[1].origin.x());
    EXPECT_EQ(double{sections[1]->at("origin/y/value")}, surface.sections[1].origin.y());
    EXPECT_EQ(double{sections[1]->at("origin/z/value")}, surface.sections[1].origin.z());
    EXPECT_EQ(double{sections[1]->at("upper_height/value")}, surface.sections[1].get_contour(true).bbox().ymax());
    EXPECT_EQ(double{sections[1]->at("lower_height/value")}, std::abs(surface.sections[1].get_contour(true).bbox().ymin()));
    EXPECT_EQ(double{sections[1]->at("width/value")}, surface.sections[1].get_contour(true).bbox().x_span());
    EXPECT_EQ(std::string(sections[1]->at("section_shape/value")), surface.sections[1].name);
}

/**
 * @brief Test converting a airfoil surface to an aixml node.
 */
TEST_F(ConvertAirfoilSurface, ToAixmlNode)
{
    /* Treat the surface as an airfoil surface */
    geom2::io::SurfaceType pylon = geom2::io::AirfoilSurface{surface};

    /* Convert using the aixml format */
    node root{};
    node& result = std::visit(geom2::io::AixmlConverter{root, {"pylon", "0", "pylon surface"}}, pylon);

    /* Test the root properties */
    EXPECT_EQ(result.getName(), "pylon");
    EXPECT_EQ(result.getStringAttrib("ID"), "0");
    EXPECT_EQ(result.getStringAttrib("description"), "pylon surface");

    /* Test the position node */
    EXPECT_EQ(result.at("position").getStringAttrib("description"), "reference position in global coordinates");
    EXPECT_EQ(double{result.at("position/x/value")}, surface.origin.x());
    EXPECT_EQ(double{result.at("position/y/value")}, surface.origin.y());
    EXPECT_EQ(double{result.at("position/z/value")}, surface.origin.z());
    EXPECT_EQ(result.at("position/x").getStringAttrib("description"), "x coordinate of point");
    EXPECT_EQ(result.at("position/y").getStringAttrib("description"), "y coordinate of point");
    EXPECT_EQ(result.at("position/z").getStringAttrib("description"), "z coordinate of point");


    /* Test the normal node */
    EXPECT_EQ(result.at("normal").getStringAttrib("description"), "unit vector according to global coordinate system for direction applied at position");
    EXPECT_EQ(double{result.at("normal/x/value")}, surface.normal.dx());
    EXPECT_EQ(double{result.at("normal/y/value")}, surface.normal.dy());
    EXPECT_EQ(double{result.at("normal/z/value")}, surface.normal.dz());
    EXPECT_EQ(result.at("normal/x").getStringAttrib("description"), "x direction of unit vector");
    EXPECT_EQ(result.at("normal/y").getStringAttrib("description"), "y direction of unit vector");
    EXPECT_EQ(result.at("normal/z").getStringAttrib("description"), "z direction of unit vector");

    /* Test the sections */
    EXPECT_EQ(result.at("sections").getStringAttrib("description"), "sections");
    auto sections = result.at("sections").getVector("section");
    ASSERT_EQ(sections.size(), 2);
    EXPECT_EQ(sections[0]->getStringAttrib("ID"), "0");
    EXPECT_EQ(sections[0]->at("origin").getStringAttrib("description"), "origin of chord (local)");
    EXPECT_EQ(double{sections[0]->at("origin/x/value")}, surface.sections[0].origin.x());
    EXPECT_EQ(double{sections[0]->at("origin/y/value")}, surface.sections[0].origin.y());
    EXPECT_EQ(double{sections[0]->at("origin/z/value")}, surface.sections[0].origin.z());
    EXPECT_EQ(double{sections[0]->at("chord_length/value")}, surface.sections[0].get_chord_length());
    EXPECT_EQ(double{sections[0]->at("geometric_twist/value")}, 0.0);
    EXPECT_EQ(std::string(sections[0]->at("profile/value")), surface.sections[0].name);
    EXPECT_EQ(sections[1]->getStringAttrib("ID"), "1");
    EXPECT_EQ(sections[1]->at("origin").getStringAttrib("description"), "origin of chord (local)");
    EXPECT_EQ(double{sections[1]->at("origin/x/value")}, surface.sections[1].origin.x());
    EXPECT_EQ(double{sections[1]->at("origin/y/value")}, surface.sections[1].origin.y());
    EXPECT_EQ(double{sections[1]->at("origin/z/value")}, surface.sections[1].origin.z());
    EXPECT_EQ(double{sections[1]->at("chord_length/value")}, surface.sections[1].get_chord_length());
    EXPECT_NEAR(double{sections[1]->at("geometric_twist/value")}, 0.1, 1e-3);
    EXPECT_EQ(std::string(sections[1]->at("profile/value")), surface.sections[1].name);
}

/**
 * @brief Test converting a wing surface to an aixml node.
 */
using ConvertWingSurface = ConvertAirfoilSurface; // Reuse the same test fixture of the airfoil surface
TEST_F(ConvertWingSurface, ToAixmlNode)
{
    /* Treat the surface as an airfoil surface */
    geom2::io::SurfaceType wing = geom2::io::Wing{surface};

    /* Convert using the aixml format */
    node root{};
    node& result = std::visit(geom2::io::AixmlConverter{root, {"aerodynamic_surface", "0", "aerodynamic surface"}}, wing);

    /* Test the root properties */
    EXPECT_EQ(result.getName(), "aerodynamic_surface");
    EXPECT_EQ(result.getStringAttrib("ID"), "0");
    EXPECT_EQ(result.getStringAttrib("description"), "aerodynamic surface");
    EXPECT_FALSE(isTrue(result.at("parameters/symmetric/value")));

    /* Test the name node */
    EXPECT_EQ(std::string(result.at("name/value")), surface.name);
    EXPECT_EQ(result.at("name").getStringAttrib("description"), "name of surface");

    /* Test the position node */
    EXPECT_EQ(result.at("position").getStringAttrib("description"), "reference position in global coordinates");
    EXPECT_EQ(double{result.at("position/x/value")}, surface.origin.x());
    EXPECT_EQ(double{result.at("position/y/value")}, surface.origin.y());
    EXPECT_EQ(double{result.at("position/z/value")}, surface.origin.z());
    EXPECT_EQ(result.at("position/x").getStringAttrib("description"), "x coordinate of point");
    EXPECT_EQ(result.at("position/y").getStringAttrib("description"), "y coordinate of point");
    EXPECT_EQ(result.at("position/z").getStringAttrib("description"), "z coordinate of point");

    /* Test the parameters node */
    EXPECT_EQ(result.at("parameters").getStringAttrib("description"), "aerodynamic surface parameters");

    /* Test the direction node */
    EXPECT_EQ(result.at("parameters/direction").getStringAttrib("description"), "unit vector according to global coordinate system for direction applied at position");
    EXPECT_EQ(double{result.at("parameters/direction/x/value")}, surface.normal.dx());
    EXPECT_EQ(double{result.at("parameters/direction/y/value")}, surface.normal.dy());
    EXPECT_EQ(double{result.at("parameters/direction/z/value")}, surface.normal.dz());
    EXPECT_EQ(result.at("parameters/direction/x").getStringAttrib("description"), "x direction of unit vector");
    EXPECT_EQ(result.at("parameters/direction/y").getStringAttrib("description"), "y direction of unit vector");
    EXPECT_EQ(result.at("parameters/direction/z").getStringAttrib("description"), "z direction of unit vector");

    /* Test the sections */
    EXPECT_EQ(result.at("parameters/sections").getStringAttrib("description"), "sections");
    auto sections = result.at("parameters/sections").getVector("section");
    ASSERT_EQ(sections.size(), 2);
    EXPECT_EQ(sections[0]->getStringAttrib("ID"), "0");
    EXPECT_EQ(sections[0]->at("chord_origin").getStringAttrib("description"), "origin of chord (local)");
    EXPECT_EQ(double{sections[0]->at("chord_origin/x/value")}, surface.sections[0].origin.x());
    EXPECT_EQ(double{sections[0]->at("chord_origin/y/value")}, surface.sections[0].origin.y());
    EXPECT_EQ(double{sections[0]->at("chord_origin/z/value")}, surface.sections[0].origin.z());
    EXPECT_EQ(double{sections[0]->at("chord_length/value")}, surface.sections[0].get_chord_length());
    EXPECT_EQ(double{sections[0]->at("geometric_twist/value")}, 0.0);
    EXPECT_EQ(double{sections[0]->at("scale_thickness/value")},surface.sections[0].get_thickness_scale());
    EXPECT_EQ(std::string(sections[0]->at("profile/value")), surface.sections[0].name);
    EXPECT_EQ(sections[1]->getStringAttrib("ID"), "1");
    EXPECT_EQ(sections[1]->at("chord_origin").getStringAttrib("description"), "origin of chord (local)");
    EXPECT_EQ(double{sections[1]->at("chord_origin/x/value")}, surface.sections[1].origin.x());
    EXPECT_EQ(double{sections[1]->at("chord_origin/y/value")}, surface.sections[1].origin.y());
    EXPECT_EQ(double{sections[1]->at("chord_origin/z/value")}, surface.sections[1].origin.z());
    EXPECT_EQ(double{sections[1]->at("chord_length/value")}, surface.sections[1].get_chord_length());
    EXPECT_NEAR(double{sections[1]->at("geometric_twist/value")}, 0.1, 1e-3);
    EXPECT_EQ(double{sections[1]->at("scale_thickness/value")},surface.sections[1].get_thickness_scale());
    EXPECT_EQ(std::string(sections[1]->at("profile/value")), surface.sections[1].name);
}

/**
 * @brief Test converting an wing surface to an aixml node and
 * insert it into an existing node tree.
 */
TEST_F(ConvertWingSurface, UpdateAixmlNode)
{
    /* Treat the surface as an airfoil surface */
    geom2::io::SurfaceType wing = geom2::io::Wing{surface};

    /* Create the xml tree to insert into */
    auto acxml = std::make_shared<node>("aircraft_exchange_file");
    node& insert_here = acxml->appendChild("component_design").appendChild("wing").appendChild("specific").appendChild("geometry");

    /* Convert using the aixml format */
    std::visit(geom2::io::AixmlConverter{insert_here, {"aerodynamic_surface", "0", ""}}, wing);

    /* Test result */
    ASSERT_TRUE(acxml->find("component_design/wing/specific/geometry/aerodynamic_surface"));

    /* Test inserting another surface at the same position */
    auto result2 = std::visit(geom2::io::AixmlConverter{insert_here, {"aerodynamic_surface", "1"}}, wing);
    auto surfaces = acxml->getVector("component_design/wing/specific/geometry/aerodynamic_surface");
    ASSERT_EQ(surfaces.size(), 2);
}

/**
 * @brief Test updating an existing aixml wing node without overwriting
 * other existing entries
 */
TEST_F(ConvertWingSurface, UpdateExistingAixmlNode)
{
    /* Use the string literal operator for convenience */
    using std::literals::string_literals::operator""s;

    /* Treat the surface as an airfoil surface */
    geom2::io::SurfaceType wing = geom2::io::Wing{surface};

    /* Create the existing node tree  */
    auto acxml = std::make_shared<node>("aircraft_exchange_file");
    node& insert_here = acxml->appendChild("component_design")
        .appendChild("wing")
        .appendChild("specific")
        .appendChild("geometry");
    insert_here.appendChild("aerodynamic_surface").setAttrib("ID", "0");
    insert_here["aerodynamic_surface/name/value"s] = "old_wing";
    insert_here["aerodynamic_surface/parameters/direction"s] = 4.0;
    insert_here["aerodynamic_surface/parameters/sections/section@0"s] = 3.0;
    insert_here["aerodynamic_surface/parameters/sections/section@1"s] = 3.1;
    insert_here["aerodynamic_surface/parameters/sections/section@2"s] = 3.2;
    insert_here.at("aerodynamic_surface").appendChild("mass_properties").setAttrib("description", "mass");
    insert_here.at("aerodynamic_surface/mass_properties") = 42.0;

    /* Update the node tree */
    std::visit(geom2::io::AixmlConverter{insert_here, {"aerodynamic_surface", "0", ""}}, wing);

    /* Test whether the existing nodes are still there */
    ASSERT_NO_THROW(acxml->at("component_design/wing/specific/geometry/aerodynamic_surface/mass_properties"));
    EXPECT_EQ(double{acxml->at("component_design/wing/specific/geometry/aerodynamic_surface/mass_properties")}, 42.0);

    /* Test whether the existing surface is updated */
    ASSERT_EQ(acxml->getVector("component_design/wing/specific/geometry/aerodynamic_surface").size(), 1);
    EXPECT_EQ(std::string(acxml->at("component_design/wing/specific/geometry/aerodynamic_surface/name/value")), surface.name);
    EXPECT_EQ(acxml->getVector("component_design/wing/specific/geometry/aerodynamic_surface/parameters/direction").size(), 1);
    EXPECT_EQ(acxml->getVector("component_design/wing/specific/geometry/aerodynamic_surface/parameters/sections/section").size(), 2);
}

/**
 * @brief Test converting a spar surface to an aixml node.
 */
TEST(ConvertSparSurface, ToAixmlNode)
{
    /* Create a simple spar surface */
    geom2::Polygon_2 shape;
    geom2::MultisectionSurface<geom2::PolygonSection> surface;
    shape.push_back({0.2, 0.0});
    shape.push_back({0.7, 0.0});
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections[0].origin = {0, 0, 0};
    surface.sections[1].origin = {0, 0, -1.0};

    /* Treat surface as spar geometry */
    geom2::io::SurfaceType spar = geom2::io::Spar{surface};

    /* Convert the geometry to the node */
    node root{};
    auto& result = std::visit(geom2::io::AixmlConverter{root, {"spar", "0", "spar geometry"}}, spar);

    /* Test the result */
    EXPECT_EQ(result.getName(), "spar");
    EXPECT_EQ(result.getStringAttrib("ID"), "0");
    EXPECT_EQ(result.getStringAttrib("description"), "spar geometry");
    EXPECT_EQ(double{result.at("position/inner_position/spanwise/value")}, 0.0);
    EXPECT_EQ(double{result.at("position/inner_position/chord/from/value")}, 0.2);
    EXPECT_EQ(double{result.at("position/inner_position/chord/to/value")}, 0.7);
    EXPECT_EQ(double{result.at("position/outer_position/spanwise/value")}, 1.0);
    EXPECT_EQ(double{result.at("position/outer_position/chord/from/value")}, 0.2);
    EXPECT_EQ(double{result.at("position/outer_position/chord/to/value")}, 0.7);
}

/**
 * @brief Test converting a control device surface to an aixml node.
 */
TEST(ConvertControlDevice, ToAixmlNode)
{
    /* Create a simple spar surface */
    geom2::Polygon_2 shape;
    geom2::MultisectionSurface<geom2::PolygonSection> surface;
    shape.push_back({0.2, 0.0});
    shape.push_back({0.7, 0.0});
    surface.sections.emplace_back(shape);
    surface.sections.emplace_back(shape);
    surface.sections[0].origin = {0, 0, 0};
    surface.sections[1].origin = {0, 0, -1.0};

    /* Treat surface as spar geometry */
    geom2::io::SurfaceType device = geom2::io::ControlDevice{surface};

    /* Convert the geometry to the node */
    node root{};
    auto& result = std::visit(geom2::io::AixmlConverter{root, {"control_device", "0", "wow many lift"}}, device);

    /* Test the result */
    EXPECT_EQ(result.getName(), "control_device");
    EXPECT_EQ(result.getStringAttrib("ID"), "0");
    EXPECT_EQ(result.getStringAttrib("description"), "wow many lift");
    EXPECT_EQ(double{result.at("position/inner_position/spanwise/value")}, 0.0);
    EXPECT_EQ(double{result.at("position/inner_position/chord/from/value")}, 0.2);
    EXPECT_EQ(double{result.at("position/inner_position/chord/to/value")}, 0.7);
    EXPECT_EQ(double{result.at("position/outer_position/spanwise/value")}, 1.0);
    EXPECT_EQ(double{result.at("position/outer_position/chord/from/value")}, 0.2);
    EXPECT_EQ(double{result.at("position/outer_position/chord/to/value")}, 0.7);
}
