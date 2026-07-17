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
#include <aircraftGeometry2/airfoil_surface.h>

/* === Types === */
using Point_3 = geom2::Point_3;
using Direction_3 = geom2::Direction_3;

/* === Fixtures === */
class PylonAIXMLv2 : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Open the simple fuselage example */
        std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
        file /= "acxml-v2/pylon.xml";
        if (!std::filesystem::exists(file))
        {
            throw std::runtime_error("Could not find file: " + file.string());
        }
        AcXml = aixml::openDocument(file);
    }
    std::shared_ptr<node> AcXml;
};

class PylonAIXMLv3 : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Open the simple wing example */
        std::filesystem::path file = data_dir / "acxml-v3/pylon.xml";
        if (!std::filesystem::exists(file))
        {
            throw std::runtime_error("Could not find file: " + file.string());
        }
        AcXml = aixml::openDocument(file);
    }
    std::shared_ptr<node> AcXml;
    std::filesystem::path data_dir{CMAKE_TEST_STUBS_DIR};
};

/* === Tests === */
/**
 * @brief Test the base properties which should be set by the factory.
 */
TEST_F(PylonAIXMLv2, BaseProperties)
{
    /* Set the pylon base properties */
    AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceRefPoint/r_Surface") = 1.0;
    AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceRefPoint/y_Surface") = 2.0;
    AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceRefPoint/h_Surface") = 3.0;

    /* Create pylon */
    geom2::AirfoilSurfaceFactory pylon{AcXml, ""};

    /* Build from node */
    auto surface = pylon.create("Pylon@SimplePylon");

    /* Test its properties*/
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
}

/**
 * @brief Test creating a pylon with two parallel sections.
 */
TEST_F(PylonAIXMLv2, CreateTwoParallelSections)
{
    /* Set the section properties */
    node &section1 = AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceParameters/SurfaceSegment@1");
    section1.at("l_i_Segment") = 2.0;
    section1.at("l_o_Segment") = 2.0;
    section1.at("s_Segment") = 3.0;
    node &section2 = AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceParameters/SurfaceSegment@2");
    section2.at("l_i_Segment") = 2.0;
    section2.at("l_o_Segment") = 0.8;
    section2.at("s_Segment") = 1.5;

    /* Create pylon */
    geom2::AirfoilSurfaceFactory pylon{AcXml, ""};
    auto surface = pylon.create("Pylon@SimplePylon");

    /* Test its properties*/
    ASSERT_EQ(surface.sections.size(), 3);

    /* Section1 */
    auto result = surface.sections[0];
    EXPECT_EQ(result.name, "n0012-tab");
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 2.0);

    /* Section2 */
    result = surface.sections[1];
    EXPECT_EQ(result.name, "n0012-tab");
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, -3.0));
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 2.0);

    /* Section3 */
    result = surface.sections[2];
    EXPECT_EQ(result.name, "n0012-tab");
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, -4.5));
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 0.8);
}

/**
 * @brief Test creating a pylon with a segment with dihedral.
 */
TEST_F(PylonAIXMLv2, CreateSegmentWithDihedral)
{
    /* Set the section properties */
    node &section1 = AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceParameters/SurfaceSegment@1");
    section1.at("l_i_Segment") = 2.0;
    section1.at("l_o_Segment") = 2.0;
    section1.at("s_Segment") = 3.0;
    node &section2 = AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceParameters/SurfaceSegment@2");
    section2.at("l_i_Segment") = 2.0;
    section2.at("l_o_Segment") = 0.8;
    section2.at("s_Segment") = 1.5;
    section2.at("nu_Segment") = 45;

    /* Create pylon */
    geom2::AirfoilSurfaceFactory pylon{AcXml, ""};
    auto surface = pylon.create("Pylon@SimplePylon");

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
    EXPECT_EQ(result.origin.x(), 0.0);
    EXPECT_NEAR(result.origin.y(), 1.0605, 1e-3);
    EXPECT_NEAR(result.origin.z(), -4.0607, 1e-3);
    EXPECT_EQ(result.normal.dx(), 0.0);
    GTEST_SKIP() << "Should the dihedral of pylon sections also be neglected on section level as for wings?";
    EXPECT_NEAR(result.normal.dy(), -0.7071, 1e-3);
    EXPECT_NEAR(result.normal.dz(), 0.7071, 1e-3);
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 0.8);
}

/**
 * @brief Test creating a pylon with a segment with sweep.
 */
TEST_F(PylonAIXMLv2, CreateSegmentWithSweep)
{
    /* Set the section properties */
    node &section1 = AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceParameters/SurfaceSegment@1");
    section1.at("l_i_Segment") = 2.0;
    section1.at("l_o_Segment") = 2.0;
    section1.at("s_Segment") = 3.0;
    node &section2 = AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceParameters/SurfaceSegment@2");
    section2.at("l_i_Segment") = 2.0;
    section2.at("l_o_Segment") = 0.8;
    section2.at("s_Segment") = 1.5;
    section2.at("phi_Segment") = 45;

    /* Create pylon */
    geom2::AirfoilSurfaceFactory pylon{AcXml, ""};
    auto surface = pylon.create("Pylon@SimplePylon");

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
    EXPECT_NEAR(result.origin.x(), 1.5, 1e-3);
    EXPECT_EQ(result.origin.y(), 0.0);
    EXPECT_NEAR(result.origin.z(), -4.5, 1e-3);
    EXPECT_EQ(result.normal, Direction_3(0.0, 0.0, 1.0));
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 0.8);
}

/**
 * @brief Test creating a pylon with a segment with twist.
 */
TEST_F(PylonAIXMLv2, CreateSegmentWithTwist)
{
    /* Set the section properties */
    node &section1 = AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceParameters/SurfaceSegment@1");
    section1.at("l_i_Segment") = 2.0;
    section1.at("l_o_Segment") = 2.0;
    section1.at("s_Segment") = 3.0;
    node &section2 = AcXml->at("AcftExchangeFile/Geometry/Pylon/SurfaceParameters/SurfaceSegment@2");
    section2.at("l_i_Segment") = 2.0;
    section2.at("l_o_Segment") = 1.0;
    section2.at("s_Segment") = 1.5;
    section2.at("epsilon_Segment") = 45;

    /* Create pylon */
    geom2::AirfoilSurfaceFactory pylon{AcXml, ""};
    auto surface = pylon.create("Pylon@SimplePylon");

    /* Test its properties*/
    ASSERT_EQ(surface.sections.size(), 3);

    /* Roughly test whether a twist is applied on the last section */
    auto result = surface.sections[2].get_contour(true).bbox();
    EXPECT_NEAR(result.x_span(), 0.707, 1e-2);
}

/**
 * @brief Test the base properties which should be set by the factory.
 */
TEST_F(PylonAIXMLv3, BaseProperties)
{
    /* Set the pylon base properties */
    AcXml->at("aircraft_exchange_file/component_design/propulsion/specific/propulsion@0/pylon/position/x/value") = 1.0;
    AcXml->at("aircraft_exchange_file/component_design/propulsion/specific/propulsion@0/pylon/position/y/value") = 2.0;
    AcXml->at("aircraft_exchange_file/component_design/propulsion/specific/propulsion@0/pylon/position/z/value") = 3.0;

    /* Create pylon */
    geom2::AirfoilSurfaceFactory pylon{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = pylon.create("propulsion/specific/propulsion@0/pylon");

    /* Test its properties*/
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
    EXPECT_EQ(surface.normal, Direction_3(0.0, -1.0, 0.0));
}

/**
 * @brief Test creating a pylon with two parallel sections.
 */
TEST_F(PylonAIXMLv3, CreateTwoParallelSections)
{
    /* Create pylon */
    geom2::AirfoilSurfaceFactory pylon{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = pylon.create("propulsion/specific/propulsion@0/pylon");

    /* Test the result */
    ASSERT_EQ(surface.sections.size(), 2);

    /* Section 0 */
    auto result = surface.sections[0];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_EQ(result.name, "n0012-tab");
    EXPECT_EQ(result.get_chord_length(), 2.0);
    EXPECT_EQ(result.get_thickness_scale(), 1.0);

    /* Section 1 */
    result = surface.sections[1];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, -1.0));
    EXPECT_EQ(result.name, "n0012-tab");
    EXPECT_EQ(result.get_chord_length(), 1.0);
    EXPECT_EQ(result.get_thickness_scale(), 1.0);
}
