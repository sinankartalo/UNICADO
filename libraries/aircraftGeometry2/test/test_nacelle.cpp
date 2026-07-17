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
#include <aircraftGeometry2/hull_surface.h>
#include <aircraftGeometry2/geometry/import_geom.h>

/* === Types === */
using Point_3 = geom2::Point_3;

/* === Fixtures === */
class NacelleAIXMLv2 : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Open the simple fuselage example from the stubs folder */
        std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
        file /= "acxml-v2/nacelle.xml";
        if (!std::filesystem::exists(file))
        {
            throw std::runtime_error("Could not find file: " + file.string());
        }
        AcXml = aixml::openDocument(file);
    }
    std::shared_ptr<node> AcXml;
};

class NacelleAIXMLv3 : public ::testing::Test
{
protected:
    void SetUp() override
    {
        /* Open the simple fuselage example from the stubs folder */
        std::filesystem::path file = data_dir / "acxml-v3/nacelle.xml";
        if (!std::filesystem::exists(file))
        {
            throw std::runtime_error("Could not find file: " + file.string());
        }
        AcXml = aixml::openDocument(file);
    }
    std::filesystem::path data_dir{CMAKE_TEST_STUBS_DIR};
    std::shared_ptr<node> AcXml;
    std::string root_path_nacelle{"aircraft_exchange_file/component_design/propulsion/specific/"}; //cppcheck-suppress unusedStructMember
};

/* === Tests === */
/**
 * @brief Test the base properties which should be set by the factory.
 */
TEST_F(NacelleAIXMLv2, BaseProperties)
{
    /* Set the nacelle base properties */
    AcXml->at("AcftExchangeFile/Geometry/Nacelle/NacelleRefPoint/r_Nacelle") = 1.0;
    AcXml->at("AcftExchangeFile/Geometry/Nacelle/NacelleRefPoint/y_Nacelle") = 2.0;
    AcXml->at("AcftExchangeFile/Geometry/Nacelle/NacelleRefPoint/h_Nacelle") = 3.0;

    /* Create nacelle */
    geom2::HullFactory nacelle{AcXml, ""};

    /* Build from node */
    auto surface = nacelle.create("Nacelle@SimpleHull");

    /* Test its properties*/
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
}

/**
 * @brief Test creating a nacelle.
 */
TEST_F(NacelleAIXMLv2, CreateNacelle)
{
    /* Create nacelle as defined in the stub file */
    geom2::HullFactory nacelle{AcXml, ""};
    auto surface = nacelle.create("Nacelle@SimpleHull");

    /* Test its properties*/
    ASSERT_EQ(surface.sections.size(), 4);

    /* Inlet */
    auto result = surface.sections[0];
    EXPECT_EQ(result.name, "circle-tab");
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_NEAR(result.get_contour(true).bbox().x_span(), 1.976, 1e-3);
    EXPECT_NEAR(result.get_contour(true).bbox().y_span(), 2.289, 1e-3);

    /* Section1 */
    result = surface.sections[1];
    EXPECT_EQ(result.name, "circle-tab");
    EXPECT_NEAR(result.origin.x(), 0.0, 1e-3);
    EXPECT_NEAR(result.origin.y(), 0.0, 1e-3);
    EXPECT_NEAR(result.origin.z(), 0.677, 1e-3);
    EXPECT_NEAR(result.get_contour(true).bbox().x_span(), 2.471, 1e-3);
    EXPECT_NEAR(result.get_contour(true).bbox().y_span(), 2.861, 1e-3);

    /* Section2 */
    result = surface.sections[2];
    EXPECT_EQ(result.name, "circle-tab");
    EXPECT_NEAR(result.origin.x(), 0.0, 1e-3);
    EXPECT_NEAR(result.origin.y(), 0.0, 1e-3);
    EXPECT_NEAR(result.origin.z(), 2.033, 1e-3);
    EXPECT_NEAR(result.get_contour(true).bbox().x_span(), 2.471, 1e-3);
    EXPECT_NEAR(result.get_contour(true).bbox().y_span(), 2.861, 1e-3);

    /* Outlet */
    result = surface.sections[3];
    EXPECT_EQ(result.name, "circle-tab");
    EXPECT_NEAR(result.origin.x(), 0.0, 1e-3);
    EXPECT_NEAR(result.origin.y(), 0.0, 1e-3);
    EXPECT_NEAR(result.origin.z(), 2.711, 1e-3);
    EXPECT_NEAR(result.get_contour(true).bbox().x_span(), 1.976, 1e-3);
    EXPECT_NEAR(result.get_contour(true).bbox().y_span(), 2.289, 1e-3);
}

/**
 * @brief Test the base properties which should be set by the factory.
 */
TEST_F(NacelleAIXMLv3, BaseProperties)
{
    /* Set the nacelle base properties */
    AcXml->at(root_path_nacelle + "nacelle@0/position/x/value") = 1.0;
    AcXml->at(root_path_nacelle + "nacelle@0/position/y/value") = 2.0;
    AcXml->at(root_path_nacelle + "nacelle@0/position/z/value") = 3.0;

    /* Create nacelle */
    geom2::HullFactory nacelle{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = nacelle.create("propulsion/specific/nacelle@0");

    /* Test its properties*/
    EXPECT_EQ(surface.origin, Point_3(1.0, 2.0, 3.0));
    EXPECT_EQ(surface.normal, geom2::Direction_3(0.0, 0.0, 1.0));
}

/**
 * @brief Test creating a nacelle with two parallel sections.
 */
TEST_F(NacelleAIXMLv3, CreateTwoParallelSections)
{
    /* Create nacelle */
    geom2::HullFactory nacelle{AcXml, data_dir / "dat-files"};

    /* Build from node */
    auto surface = nacelle.create("propulsion/specific/nacelle@0");

    /* Test the result */
    ASSERT_EQ(surface.sections.size(), 2);

    /* Section 0 */
    auto result = surface.sections[0];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, 0.0));
    EXPECT_EQ(result.name, "circle-tab");
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 2.0);
    EXPECT_EQ(result.get_contour(true).bbox().y_span(), 1.5);

    /* Section 1 */
    result = surface.sections[1];
    EXPECT_EQ(result.origin, Point_3(0.0, 0.0, 2.0));
    EXPECT_EQ(result.get_contour(true).bbox().x_span(), 1.0);
    EXPECT_EQ(result.get_contour(true).bbox().y_span(), 1.0);
}

/**
 * @brief Test creating a nacelle with two parallel sections.
 */
TEST_F(NacelleAIXMLv3, CreateExternalNacelle)
{
    std::filesystem::path file = data_dir / "acxml-v3/nacelle_geometry.xml";
    std::filesystem::path file_acxml = data_dir / "acxml-v3/ac_geometry.xml";
    auto external_geometry = aixml::openDocument(file);
    auto nacelle = geom2::import_external_nacelle_design(external_geometry, data_dir / "dat-files");
}