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

#include <filesystem>
#include <gtest/gtest.h>

/* Unit Under Test */
#include "engine/engine_data.h"

/* === Fixtures === */
class EngineDataV2 : public ::testing::Test
{
  protected:

    /* Test setup */
    void SetUp() override
    {
        std::filesystem::path xml_path{CMAKE_TEST_STUBS_DIR};
        xml_path /= "V2527-A5/V2527-A5.xml";
        if (!std::filesystem::exists(xml_path))
        {
            FAIL() << "The engine XML file does not exist.";
        }
        engine_xml = aixml::openDocument(xml_path);

    }
    std::shared_ptr<node> engine_xml;

};

/* === Tests === */
/**
 * @brief Test reading the design conditions in the V2 engine XML format.
 */
TEST_F(EngineDataV2, ReadDesignConditions)
{
    /* Create the test engine */
    EngineData engine{engine_xml};

    /* Read the design conditions */
    EXPECT_NEAR(engine.design_point().altitude, 10668.0, 1e-3);
    EXPECT_NEAR(engine.design_point().Mach, 0.8, 1e-3);
    EXPECT_NEAR(engine.design_thrust(), 17010.0, 1e-3);
    EXPECT_NEAR(engine.SLST(), 110.31*1000, 1e-3);
    EXPECT_NEAR(engine.MCT(), 98920.0, 1e-3);
}

/**
 * @brief Test reading the engine name in the V2 engine XML format.
 */
TEST_F(EngineDataV2, ReadEngineName)
{
    /* Create the test engine */
    EngineData engine{engine_xml};

    /* Read the engine name */
    EXPECT_EQ(engine.name(), "V2527-A5");
}

/**
 * @brief Try reading a value which is supposed to be in the 
 * engine xml but is missing.
 */
TEST_F(EngineDataV2, ReadMissingValue)
{
    /* Delete the design thrust node */
    engine_xml->at("EngineDataFile/EngineDesignCondition").deleteChild("thrust");
    EngineData engine{engine_xml};

    /* Read the design thrust */
    double result{-1};
    ASSERT_NO_THROW(result=engine.design_thrust());
    EXPECT_DOUBLE_EQ(result, 0.0);
}

/**
 * @brief Test reading the engine dimensions in the V2 engine XML format.
 */
TEST_F(EngineDataV2, ReadEngineDimensions)
{
    /* Create the test engine */
    EngineData engine{engine_xml};

    /* Read the engine dimensions */
    EXPECT_NEAR(engine.dimensions().height, 2.118, 1e-3);
    EXPECT_NEAR(engine.dimensions().width, 1.829, 1e-3);
    EXPECT_NEAR(engine.dimensions().length, 2.508, 1e-3);
    EXPECT_NEAR(engine.dimensions().diameter, 1.613, 1e-3);
}

/**
 * @brief Test reading the engine mass in the V2 engine XML format.
 */
TEST_F(EngineDataV2, ReadMassProperties)
{
    /* Create the test engine */
    EngineData engine{engine_xml};

    /* Read the mass properties */
    EXPECT_NEAR(engine.dry_mass(), 2404.0, 1e-3);
}

/**
 * @brief Test reading one value of the test engine deck.
 * 
 */
TEST_F(EngineDataV2, ReadDeckValue)
{
    /* Create the test engine */
    EngineData engine{engine_xml};

    /* Read the deck example value */
    OperatingPoint op_point{1.0, 10000.0, 0.8};
    EXPECT_NEAR(engine.get_deck_value("thrust", op_point), 31.293, 0.1);

    /* Create a test engine where decks are parsed when needed */
    EngineData engine_lazy{engine_xml, true};
    EXPECT_NEAR(engine_lazy.get_deck_value("thrust", op_point), 31.293, 0.1);

    /* Create a const engine which should parse all decks at construction */
    const EngineData engine_const(engine_xml);
    EXPECT_NEAR(engine_const.get_deck_value("thrust", op_point), 31.293, 0.1);

    /* Create a const engine which should not parse all decks at construction */
    const EngineData engine_const_lazy(engine_xml, true);
    /* Getting the deck value now shot throw since we are not allowed lazy parse when const! */
    EXPECT_THROW(engine_const_lazy.get_deck_value("thrust", op_point), std::out_of_range);
}

/**
 * @brief Test getting the lower and upper operation point of the engine deck.
 * 
 */
TEST_F(EngineDataV2, GetUpperAndLowerOPBoundaries)
{
    /* Create the test engine */
    EngineData engine{engine_xml};

    /* Get the max and min operating point */
    EXPECT_NEAR(engine.get_upper_operating_point("thrust").N, 1.1, 1e-6);
    EXPECT_NEAR(engine.get_upper_operating_point("thrust").Mach, 0.95, 1e-6);
    EXPECT_NEAR(engine.get_upper_operating_point("thrust").altitude, 14000, 1e-6);

    EXPECT_NEAR(engine.get_lower_operating_point("thrust").N, 0.29, 1e-6);
    EXPECT_NEAR(engine.get_lower_operating_point("thrust").Mach, 0.0, 1e-6);
    EXPECT_NEAR(engine.get_lower_operating_point("thrust").altitude, 0.0, 1e-6);

    EXPECT_THROW(engine.get_upper_operating_point("very_fake_deck_value"), std::out_of_range);
    EXPECT_THROW(engine.get_lower_operating_point("very_fake_deck_value"), std::out_of_range);
}