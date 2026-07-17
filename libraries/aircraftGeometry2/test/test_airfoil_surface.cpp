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
#include "aircraftGeometry2/airfoil_surface.h"
#include "aircraftGeometry2/geometry/import_geom.h"
#include "aircraftGeometry2/processing/measure.h"

/* === Fixtures === */
class WingAIXMLv2 : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Open the simple wing example */
        std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
        file /= "acxml-v2/wing.xml";
        if (!std::filesystem::exists(file))
        {
            throw std::runtime_error("Could not find file: " + file.string());
        }
        AcXml = aixml::openDocument(file);
    }
    std::shared_ptr<node> AcXml;
};

class WingAIXMLv3 : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Open the simple wing example */
        std::filesystem::path file = data_dir / "acxml-v3/wing.xml";
        if (!std::filesystem::exists(file))
        {
            throw std::runtime_error("Could not find file: " + file.string());
        }
        AcXml = aixml::openDocument(file);
    }
    std::shared_ptr<node> AcXml;
    std::filesystem::path data_dir{CMAKE_TEST_STUBS_DIR};
};


/* === Types === */
using Point_3 = geom2::Point_3;
using SparAIXMLv2 = WingAIXMLv2;
using SparAIXMLv3 = WingAIXMLv3;

/* === Tests === */
/**
 * @brief Test base properties when creating a spar of a simple wing
 */
TEST_F(SparAIXMLv2, BaseProperties)
{
    /* Set the wing base properties */
    AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceRefPoint/r_Surface") = 1.0;
    AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceRefPoint/y_Surface") = 2.0;
    AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceRefPoint/h_Surface") = 3.0;

    /* Create spar */
    geom2::SparFactory spar{AcXml, ""};

    /* Build from node */
    auto surface = spar.create("LiftingSurface@MainWing");

    /* Test its properties*/
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
}

/**
 * @brief Test the shape of a simple spar
 */
TEST_F(SparAIXMLv2, CreateTwoParallelSections)
{
    /* Set the section properties */
    node &segments = AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceParameters/HalfSurfaceDescription");
    // First Segment
    node &section = segments.at("HalfSurfaceSegment@1");
    section.at("l_i_Segment") = 2.0;
    section.at("l_o_Segment") = 1.0;
    section.at("s_Segment") = 3.0;
    section.at("l_rel_i_FrontSpar") = 0.2;
    section.at("l_rel_o_FrontSpar") = 0.2;
    section.at("l_rel_i_RearSpar") = 0.3;
    section.at("l_rel_o_RearSpar") = 0.3;
    // Second Segment
    node &section2 = segments.at("HalfSurfaceSegment@2");
    section2.at("l_i_Segment") = 1.0;
    section2.at("l_o_Segment") = 0.5;
    section2.at("s_Segment") = 3.0;
    section2.at("l_rel_i_FrontSpar") = 0.2;
    section2.at("l_rel_o_FrontSpar") = 0.2;
    section2.at("l_rel_i_RearSpar") = 0.3;
    section2.at("l_rel_o_RearSpar") = 0.3;

    /* Create spar*/
    geom2::SparFactory spar{AcXml, ""};
    auto surface = spar.create("LiftingSurface@MainWing");

    /* Test its properties*/
    ASSERT_EQ(surface.sections.size(), 3);

    /* Section 1 */
    auto poly = surface.sections[0].get_contour(true);
    ASSERT_EQ(poly.size(), 2);
    ASSERT_TRUE(poly.is_simple());
    EXPECT_NEAR(poly.bbox().x_span(), 0.1, 1e-3);

    /* Section 2 */
    poly = surface.sections[1].get_contour(true);
    EXPECT_NEAR(poly.bbox().x_span(), 0.1, 1e-3);

    /* Section 3 */
    poly = surface.sections[2].get_contour(true);
    EXPECT_NEAR(poly.bbox().x_span(), 0.1, 1e-3);
}

/**
 * @brief Test the creating a simple spar with dihedral
 */
TEST_F(SparAIXMLv2, CreateSegmentWidthDihedral)
{
    GTEST_SKIP() << "Should the wing-normalized spar contain a dihedral or not?";
    /* Set the section properties */
    node &segments = AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceParameters/HalfSurfaceDescription");
    // First Segment
    node &section = segments.at("HalfSurfaceSegment@1");
    section.at("l_i_Segment") = 2.0;
    section.at("l_o_Segment") = 1.0;
    section.at("s_Segment") = 3.0;
    section.at("nu_Segment") = 45.0;
    section.at("l_rel_i_FrontSpar") = 0.2;
    section.at("l_rel_o_FrontSpar") = 0.2;
    section.at("l_rel_i_RearSpar") = 0.3;
    section.at("l_rel_o_RearSpar") = 0.3;
    // Second Segment
    node &section2 = segments.at("HalfSurfaceSegment@2");
    section2.at("l_i_Segment") = 1.0;
    section2.at("l_o_Segment") = 0.5;
    section2.at("s_Segment") = 3.0;
    section2.at("l_rel_i_FrontSpar") = 0.2;
    section2.at("l_rel_o_FrontSpar") = 0.2;
    section2.at("l_rel_i_RearSpar") = 0.3;
    section2.at("l_rel_o_RearSpar") = 0.3;

    /* Create spar*/
    geom2::SparFactory spar{AcXml, ""};
    auto surface = spar.create("LiftingSurface@MainWing");

    /* Section 1 */
    EXPECT_NEAR(surface.sections[1].normal.dx(), 0.0, 1e-3);
    EXPECT_NEAR(surface.sections[1].normal.dy(), -0.707, 1e-3);
    EXPECT_NEAR(surface.sections[1].normal.dz(), 0.707, 1e-3);
}

/**
 * @brief Test base properties when creating a simple wing
 */
TEST_F(WingAIXMLv2, BaseProperties)
{
    /* Set the wing base properties */
    AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceRefPoint/r_Surface") = 1.0;
    AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceRefPoint/y_Surface") = 2.0;
    AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceRefPoint/h_Surface") = 3.0;

    /* Create spar */
    geom2::WingFactory wing{AcXml, ""};

    /* Build from node */
    auto surface = wing.create("LiftingSurface@MainWing");

    /* Test its properties*/
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
    EXPECT_TRUE(surface.is_symmetric);
}

/**
 * @brief Test creating a wing with two parallel sections.
 */
TEST_F(WingAIXMLv2, CreateTwoParallelSections)
{
    /* Set the section properties */
    node &segments = AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceParameters/HalfSurfaceDescription");
    // First Segment
    node &section1 = segments.at("HalfSurfaceSegment@1");
    section1.at("l_i_Segment") = 2.0;
    section1.at("l_o_Segment") = 2.0;
    section1.at("s_Segment") = 3.0;
    section1.at("l_rel_i_FrontSpar") = 0.2;
    section1.at("l_rel_o_FrontSpar") = 0.2;
    section1.at("l_rel_i_RearSpar") = 0.3;
    section1.at("l_rel_o_RearSpar") = 0.3;
    node &section2 = segments.at("HalfSurfaceSegment@2");
    section2.at("l_i_Segment") = 2.0;
    section2.at("l_o_Segment") = 1.0;
    section2.at("s_Segment") = 1.5;
    section2.at("l_rel_i_FrontSpar") = 0.2;
    section2.at("l_rel_o_FrontSpar") = 0.2;
    section2.at("l_rel_i_RearSpar") = 0.3;
    section2.at("l_rel_o_RearSpar") = 0.3;

    /* Create pylon */
    geom2::WingFactory wing{AcXml, ""};
    auto surface = wing.create("LiftingSurface@MainWing");

    /* Test its properties*/
    ASSERT_EQ(surface.sections.size(), 3);

    /* Section1 */
    auto result = surface.sections[0];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 2.0);

    /* Section2 */
    result = surface.sections[1];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, -3.0));
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 2.0);

    /* Section3 */
    result = surface.sections[2];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, -4.5));
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 1.0);
}

/**
 * @brief Test creating a TED for a simple wing
 */
TEST_F(WingAIXMLv2, CreateTED)
{
    /* Set the wing plan form */
    node &segments = AcXml->at("AcftExchangeFile/Geometry/LiftingSurface/SurfaceParameters/HalfSurfaceDescription");
    node &section1 = segments.at("HalfSurfaceSegment@1");
    section1.at("l_i_Segment") = 2.0;
    section1.at("l_o_Segment") = 1.0;
    section1.at("s_Segment") = 3.0;

    /* Set the TED parameters */
    node &flap = segments.at("ControlDeviceSetup/TEDevice@1");
    flap.at("s_rel_i") = 0.2;
    flap.at("l_rel_i") = 0.3;
    flap.at("l_rel_TE_i") = 0.4;
    flap.at("s_rel_o") = 0.5;
    flap.at("l_rel_o") = 0.2;
    flap.at("l_rel_TE_o") = 0.1;

    /* Create the control surfaces */
    geom2::ControlDeviceFactory devices{AcXml, ""};
    auto device = devices.create("LiftingSurface@MainWing/SurfaceParameters/HalfSurfaceDescription/ControlDeviceSetup/TEDevice@1");

    /* Test results */
    ASSERT_EQ(device.sections.size(), 2);
    EXPECT_EQ(device.origin, Point_3(0.0, 0.0, 0.0));
    /* Section 1 */
    auto result = device.sections[0];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, -0.2));
    EXPECT_NEAR(result.get_contour(true).bbox().x_span(), 0.3, 1e-3);
    /* Section 2 */
    result = device.sections[1];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, -0.5));
    EXPECT_NEAR(result.get_contour(true).bbox().x_span(), 0.7, 1e-3);
}

/**
 * @brief Test base properties when creating a simple wing
 */
TEST_F(WingAIXMLv3, BaseProperties)
{
    /* Set the expectations */
    std::string base_path{"aircraft_exchange_file/component_design/wing/specific/geometry/aerodynamic_surface@0"};
    AcXml->at(base_path + "/name/value") = "My Wing";
    AcXml->at(base_path + "/position/x/value") = 1.0;
    AcXml->at(base_path + "/position/y/value") = 2.0;
    AcXml->at(base_path + "/position/z/value") = 3.0;
    AcXml->at(base_path + "/parameters/direction/x/value") = 0.0;
    AcXml->at(base_path + "/parameters/direction/y/value") = -1.0;
    AcXml->at(base_path + "/parameters/direction/z/value") = 0.0;

    /* Create Factory */
    geom2::WingFactory wing{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = wing.create("wing/specific/geometry/aerodynamic_surface@0");

    /* Test its properties*/
    EXPECT_EQ(surface.name, "My Wing");
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
    EXPECT_EQ(surface.normal, geom2::Direction_3(0.0, -1.0, 0.0));
    EXPECT_TRUE(surface.is_symmetric);
}

/**
 * @brief Test creating a wing with two parallel sections.
 */
TEST_F(WingAIXMLv3, CreateParallelSections)
{
    /* Create the factory */
    geom2::WingFactory wing{AcXml, data_dir / "dat-files"};

    /* Create the surface */
    auto surface = wing.create("wing/specific/geometry/aerodynamic_surface@0");

    /* Test the section geometry */
    ASSERT_FALSE(surface.sections.empty());
    EXPECT_EQ(surface.sections.size(), 3);
    EXPECT_EQ(surface.sections[0].name, "n0012-tab");
    EXPECT_EQ(surface.sections[0].get_chord_length(), 2.0);
    EXPECT_EQ(surface.sections[0].get_contour().size(), 200);
};

/**
 * @brief Test creating a wing with thickness scaled sections.
 */
TEST_F(WingAIXMLv3, CreateScaledThicknessSections)
{
    /* Adjust the thickness scale of the first */
    std::string base_path{"aircraft_exchange_file/component_design/wing/specific/geometry/aerodynamic_surface@0"};
    AcXml->at(base_path + "/parameters/sections/section@0/scale_thickness/value") = 1.5;

    /* Create the factory */
    geom2::WingFactory wing{AcXml, data_dir / "dat-files"};

    /* Create the surface */
    auto surface = wing.create("wing/specific/geometry/aerodynamic_surface@0");

    /* Test the section geometry */
    auto section = surface.sections[0];
    EXPECT_EQ(section.get_thickness_scale(), 1.5);
}

/**
 * @brief Test creating a spar surface from the AIXML v3 format
 */
TEST_F(WingAIXMLv3, CreateSparSurface)
{
    /* Create the factory */
    geom2::SparFactory spar{AcXml, data_dir / "dat-files"};

    /* Create the surface */
    auto surface = spar.create("wing/specific/geometry/aerodynamic_surface@0/spars/spar@0");

    /* Test the section geometry */
    ASSERT_FALSE(surface.sections.empty());
    EXPECT_EQ(surface.sections.size(), 2);
    EXPECT_DOUBLE_EQ(surface.sections[0].get_contour(true).bbox().xmin(), 0.3);
    EXPECT_DOUBLE_EQ(surface.sections[0].get_contour(true).bbox().xmax(), 0.5);
    EXPECT_EQ(surface.sections[0].origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_DOUBLE_EQ(surface.sections[1].get_contour(true).bbox().xmin(), 0.3);
    EXPECT_DOUBLE_EQ(surface.sections[1].get_contour(true).bbox().xmax(), 0.4);
    EXPECT_EQ(surface.sections[1].origin, Point_3(0.0, 0.0, 0.9));
}

/**
 * @brief Test creating the control surfaces from the AIXML v3 format
 */
TEST_F(WingAIXMLv3, CreateControlSurface)
{
    /* Create the factory */
    geom2::ControlDeviceFactory devices{AcXml, data_dir / "dat-files"};

    /* Create the surface */
    auto surface = devices.create("wing/specific/geometry/aerodynamic_surface@0/control_devices/control_device@0");

    /* Test results */
    ASSERT_EQ(surface.sections.size(), 2);
    EXPECT_EQ(surface.origin, Point_3(0.0, 0.0, 0.0));
    /* Section 1 */
    auto result = surface.sections[0];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_NEAR(result.get_contour(true).bbox().x_span(), 0.2, 1e-3);
    /* Section 2 */
    result = surface.sections[1];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, 0.4));
    EXPECT_NEAR(result.get_contour(true).bbox().x_span(), 0.1, 1e-3);
}

TEST_F(WingAIXMLv3, ImportWingGeometry)
{
    auto wings = geom2::import_wings(AcXml, data_dir / "dat-files");

    const double wing_AR = geom2::measure::aspect_ratio(wings[0]);
    const double wing_area = geom2::measure::reference_area(wings[0]);
    const double wing_span = geom2::measure::span(wings[0]);

    wings[0] = geom2::scale_wing(wings[0], 1.2);

    const double scaled_wing_AR = geom2::measure::aspect_ratio(wings[0]);
    const double scaled_wing_area = geom2::measure::reference_area(wings[0]);
    const double scaled_wing_span = geom2::measure::span(wings[0]);

    EXPECT_DOUBLE_EQ(wing_AR, scaled_wing_AR);

    EXPECT_DOUBLE_EQ(wing_area * 1.2, scaled_wing_area);
}

TEST_F(WingAIXMLv3, ImportSparGeometry)
{
    auto spars = geom2::import_spars(AcXml, data_dir / "dat-files", "0");

    ASSERT_FALSE(spars[0].sections.empty());
}

TEST_F(WingAIXMLv3, ToMesh)
{
    auto spars = geom2::import_spars(AcXml, data_dir / "dat-files", "0");
    auto mesh = geom2::transform::to_mesh(spars[0]);

    auto wings = geom2::import_wings(AcXml, data_dir / "dat-files");
    mesh += geom2::transform::to_mesh(wings[0]);

    wings[0] = geom2::scale_wing(wings[0], 5.);
    mesh += geom2::transform::to_mesh(wings[0]);

    std::fstream mesh_file;
    mesh_file.open(data_dir / "spar.stl", std::ios::out);
    CGAL::IO::write_STL(mesh_file, mesh);
    mesh_file.close();
}

