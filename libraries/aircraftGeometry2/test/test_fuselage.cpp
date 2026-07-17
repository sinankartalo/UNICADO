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
#include <aircraftGeometry2/fuselage.h>

/* === Types === */
using Point_3 = geom2::Point_3;

/* === Fixtures === */
class FuselageAIXMLv2 : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Open the simple fuselage example */
        std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
        file /= "acxml-v2/fuselage.xml";
        if (!std::filesystem::exists(file))
        {
            throw std::runtime_error("Could not find file: " + file.string());
        }
        AcXml = aixml::openDocument(file);
    }
    std::shared_ptr<node> AcXml;
};

class FuselageAIXMLv3 : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Open the simple wing example */
        std::filesystem::path file = data_dir / "acxml-v3/fuselage.xml";
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
 * @brief Test the base properties which should be set by the builder.
 *
 */
TEST_F(FuselageAIXMLv2, FuselageBaseProperties)
{
    /* Set the fuselage properties */
    AcXml->at("AcftExchangeFile/Geometry/Fuselage/FuselageRefPoint/r_Fuselage") = 1.0;
    AcXml->at("AcftExchangeFile/Geometry/Fuselage/FuselageRefPoint/y_Fuselage") = 2.0;
    AcXml->at("AcftExchangeFile/Geometry/Fuselage/FuselageRefPoint/h_Fuselage") = 3.0;

    /* Create fuselage factory */
    geom2::FuselageFactory fuselage{AcXml, ""};

    /* Build from node */
    auto surface = fuselage.create("Fuselage@SimpleFuselage");

    /* Test its properties*/
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
    EXPECT_FALSE(surface.sections.empty());
}

/**
 * @brief Test the nose segment properties which should be set by the builder.
 *
 */
TEST_F(FuselageAIXMLv2, FuselageNoseSegments)
{
    /* Set the fuselage properties */
    AcXml->at("AcftExchangeFile/Geometry/Fuselage/FuselageParameters/NoseDescription/TipSegment/h_Tip") = 0.42;
    node &segment = AcXml->at("AcftExchangeFile/Geometry/Fuselage/FuselageParameters/NoseDescription/NoseSegment@1");
    segment.at("w_Segment") = 2.0;
    segment.at("h_Segment") = 3.0;
    segment.at("l_Segment") = 4.0;
    segment.at("deltah_Segment") = 5.0;

    /* Create fuselage factory */
    geom2::FuselageFactory fuselage{AcXml, ""};

    /* Build from node */
    auto surface = fuselage.create("Fuselage@SimpleFuselage");

    /* Tip Segment */
    auto tip = surface.sections.at(0);
    EXPECT_EQ(tip.origin, Point_3(0.0, 0.42, 0));
    EXPECT_EQ(tip.name, "ellipse");
    EXPECT_EQ(tip.get_contour(true).bbox().x_span(), 0.0);
    EXPECT_EQ(tip.get_contour(true).bbox().y_span(), 0.0);

    /* First Segment */
    auto seg = surface.sections.at(1);
    EXPECT_EQ(seg.origin, Point_3(0.0, 5.0, 4.0));
    EXPECT_EQ(seg.name, "circle-tab");
    EXPECT_EQ(seg.get_contour(true).bbox().x_span(), 2.0);
    EXPECT_EQ(seg.get_contour(true).bbox().y_span(), 3.0);
}

/**
 * @brief Test the mid section segment properties which should be set by the builder.
 *
 */
TEST_F(FuselageAIXMLv2, FuselageMidSectionSegments)
{
    /* Set the fuselage properties */
    node &segment = AcXml->at("AcftExchangeFile/Geometry/Fuselage/FuselageParameters/MidSectionDescription/MidSectionSegment@1");
    segment.at("w_Segment") = 2.0;
    segment.at("h_Segment") = 3.0;
    segment.at("l_Segment") = 4.0;
    segment.at("deltah_Segment") = 5.0;

    /* Create fuselage factory */
    geom2::FuselageFactory fuselage{AcXml, ""};

    /* Build from node */
    auto surface = fuselage.create("Fuselage@SimpleFuselage");

    /* First Segment */
    // Since the fixture contains an empty nose section the "first"
    // mid section is the second surface segment...
    auto seg = surface.sections[2];
    EXPECT_EQ(seg.origin, Point_3(0.0, 5.0, 4.0));
    EXPECT_EQ(seg.get_contour(true).bbox().x_span(), 2.0);
    EXPECT_EQ(seg.get_contour(true).bbox().y_span(), 3.0);
}

/**
 * @brief Test the tail segment properties which should be set by the builder.
 *
 */
TEST_F(FuselageAIXMLv2, FuselageTailSegments)
{
    /* Set the fuselage properties */
    node &segment = AcXml->at("AcftExchangeFile/Geometry/Fuselage/FuselageParameters/TailDescription/TailSegment@1");
    segment.at("w_Segment") = 2.0;
    segment.at("h_Segment") = 3.0;
    segment.at("l_Segment") = 4.0;
    segment.at("deltah_Segment") = 5.0;

    /* Create fuselage factory */
    geom2::FuselageFactory fuselage{AcXml, ""};

    /* Build from node */
    auto surface = fuselage.create("Fuselage@SimpleFuselage");

    /* First Segment */
    // Since the fixture contains an empty nose and mid section the "first"
    // tail section is the third surface segment...
    auto seg = surface.sections[3];
    EXPECT_EQ(seg.origin, Point_3(0.0, 5.0, 4.0));
    EXPECT_EQ(seg.get_contour(true).bbox().x_span(), 2.0);
    EXPECT_EQ(seg.get_contour(true).bbox().y_span(), 3.0);
}

/**
 * @brief Test the base properties which should be set by the builder.
 *
 */
TEST_F(FuselageAIXMLv3, FuselageBaseProperties)
{
    /* Set the fuselage properties */
    AcXml->at("aircraft_exchange_file/component_design/fuselage/specific/geometry/fuselage@0/position/x/value") = 1.0;
    AcXml->at("aircraft_exchange_file/component_design/fuselage/specific/geometry/fuselage@0/position/y/value") = 2.0;
    AcXml->at("aircraft_exchange_file/component_design/fuselage/specific/geometry/fuselage@0/position/z/value") = 3.0;

    /* Create fuselage factory */
    geom2::FuselageFactory fuselage{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = fuselage.create("fuselage/specific/geometry/fuselage@0");

    /* Test its properties*/
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
    EXPECT_EQ(surface.normal, geom2::Direction_3(1.0, 0.0, 0.0));
    EXPECT_FALSE(surface.sections.empty());
}

/**
 * @brief Test the shape of a fuselage with dat files defining the section shape
 *
 */
TEST_F(FuselageAIXMLv3, FuselageDATSections)
{
    /* Create the fuselage factory */
    geom2::FuselageFactory fuselage{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = fuselage.create("fuselage/specific/geometry/fuselage@0");

    /* Test its properties*/
    ASSERT_EQ(surface.sections.size(), 2);

    /* Sections 0 */
    auto section = surface.sections[0];
    EXPECT_EQ(section.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_EQ(section.name, "circle-tab");
    EXPECT_EQ(section.get_contour(true).bbox().x_span(), 0.5);
    EXPECT_EQ(section.get_contour(true).bbox().y_span(), 2.0);

    /* Sections 1 */
    section = surface.sections[1];
    EXPECT_EQ(section.origin, Point_3(0.0, 0.0, 2.0));
    EXPECT_EQ(section.name, "circle-tab");
    EXPECT_EQ(section.get_contour(true).bbox().x_span(), 4.0);
    EXPECT_EQ(section.get_contour(true).bbox().y_span(), 3.0);
}

/**
 * @brief Test the shape of a fuselage with dat files defining the section shape
 *
 */
TEST_F(FuselageAIXMLv3, FuselageEllipseSections)
{
    /* Change the section shape to ellipse */
    AcXml->at("aircraft_exchange_file/component_design/fuselage/specific/geometry/fuselage@0/sections/section@0/section_shape/value") = "ellipse";

    /* Create the fuselage factory */
    geom2::FuselageFactory fuselage{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = fuselage.create("fuselage/specific/geometry/fuselage@0");

    /* Test its properties*/
    ASSERT_EQ(surface.sections.size(), 2);

    /* Sections 0 */
    auto section = surface.sections[0];
    EXPECT_EQ(section.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_EQ(section.name, "ellipse");
    EXPECT_EQ(section.get_contour(true).bbox().x_span(), 0.5);
    EXPECT_EQ(section.get_contour(true).bbox().y_span(), 2.0);

    /* Sections 1 */
    section = surface.sections[1];
    EXPECT_EQ(section.origin, Point_3(0.0, 0.0, 2.0));
    EXPECT_EQ(section.get_contour(true).bbox().x_span(), 4.0);
    EXPECT_EQ(section.get_contour(true).bbox().y_span(), 3.0);
}

/**
 * @brief Test the nose section of the fuselage.
 * 
 */
TEST_F(FuselageAIXMLv3, FuselageNoseSection)
{
    /* Set the dimensions of the first section to 0 */
    node &section = AcXml->at("aircraft_exchange_file/component_design/fuselage/specific/geometry/fuselage@0/sections/section@0");
    section.at("upper_height/value") = 0.0;
    section.at("lower_height/value") = 0.0;
    section.at("width/value") = 0.0;

    /* Create the fuselage factory */
    geom2::FuselageFactory fuselage{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = fuselage.create("fuselage/specific/geometry/fuselage@0");

    /* Test its properties*/
    ASSERT_EQ(surface.sections.size(), 2);

    /* Sections 0 */
    auto nose = surface.sections[0];
    EXPECT_EQ(nose.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_EQ(nose.get_contour(false).size(), 1);
    EXPECT_EQ(nose.get_contour(false).bbox().x_span(), 0.0);
    EXPECT_EQ(nose.get_contour(false).bbox().y_span(), 0.0);
}
