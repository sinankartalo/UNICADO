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
#include "engine/engine_deck.h"

/* === Fixtures === */
class ParseCSV : public ::testing::Test
{
  protected:
    /* Test setup */
    void SetUp() override
    {
        csv_file /=  "value.csv";
        if (!std::filesystem::exists(csv_file))
        {
            FAIL() << "The csv file " << csv_file << "does not exist!";
        }
    }
    std::filesystem::path csv_file{CMAKE_TEST_STUBS_DIR};
};

class ParseBigCSV : public ::testing::Test
{
  protected:
    /* Test setup */
    void SetUp() override
    {
        csv_file /=  "V2527-A5/thrust.csv";
        if (!std::filesystem::exists(csv_file))
        {
            FAIL() << "The csv file " << csv_file << "does not exist!";
        }
    }
    std::filesystem::path csv_file{CMAKE_TEST_STUBS_DIR};
};

/* === Tests === */
/**
 * @brief Test the deck value data structure.
 */
TEST(DeckData, Constructor)
{
    /* Create a deck value */
    DeckData deck_value{};

    /* Check the resulting properties */
    EXPECT_EQ(deck_value.name, "unknown");
    EXPECT_TRUE(deck_value.FL.empty());
    EXPECT_TRUE(deck_value.Mach.empty());
    EXPECT_TRUE(deck_value.N.empty());
    EXPECT_TRUE(deck_value.values.empty());
}

/**
 * @brief Test the CSV file parsing.
 */
TEST_F(ParseCSV, ValidData)
{
    /* Parse a deck value */
    DeckData deck_value = DeckData::from_csv(csv_file);

    /* Check the resulting properties */
    EXPECT_EQ(deck_value.name, "value");
    EXPECT_EQ(deck_value.FL.size(), 3);
    EXPECT_EQ(deck_value.Mach.size(), 3);
    EXPECT_EQ(deck_value.N.size(), 2);
    EXPECT_EQ(deck_value.values.shape()[0], 2); // N Dimension
    EXPECT_EQ(deck_value.values.shape()[1], 3); // FL Dimension
    EXPECT_EQ(deck_value.values.shape()[2], 3); // Mach Dimension
    EXPECT_EQ(deck_value.values[0][0][0], 1.0);
    EXPECT_EQ(deck_value.values[0][0][1], 2.0);
}

/**
 * @brief Test parsing an invalid file extension.
 */
TEST_F(ParseCSV, InvalidFileExtension)
{
    /* Parse a file which is not a CSV file */
    EXPECT_THROW(DeckData::from_csv("test.dat"), std::runtime_error);
}

/**
 * @brief Test the interpolation of a deck value.
 */
using InterpolateDeckValue = ParseCSV;
TEST_F(InterpolateDeckValue, ValidValue)
{
    /* Create a deck value */
    DeckValue deck_value{DeckData::from_csv(csv_file)};

    /* Vary N */
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 10, 0.5}), 5.0);
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({0.95, 10, 0.5}), 9.5);
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({0.90, 10, 0.5}), 14.0);

    /* Vary FL */
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 0, 0.5}), 2.0);
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 10, 0.5}), 5.0);
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 15, 0.5}), 6.5);
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 20, 0.5}), 8.0);

    /* Vary Mach */
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 10, 0.0}), 4.0);
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 10, 0.5}), 5.0);
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 10, 0.75}), 5.5);
    EXPECT_DOUBLE_EQ(deck_value.get_value_at({1.00, 10, 1.00}), 6.0);
}

/**
 * @brief Test interpolating a value which is outside the valid range.
 */
TEST_F(InterpolateDeckValue, OutsideValidRange)
{
    /* Create a deck value */
    DeckValue deck_value{DeckData::from_csv(csv_file)};

    /* Outside N range */
    EXPECT_THROW(deck_value.get_value_at({0.89, 10, 0.5}), std::out_of_range);
    EXPECT_THROW(deck_value.get_value_at({1.01, 10, 0.5}), std::out_of_range);

    /* Outside FL range */
    EXPECT_THROW(deck_value.get_value_at({1.00, 20.1, 0.5}), std::out_of_range);
    EXPECT_THROW(deck_value.get_value_at({1.00, -0.1, 0.5}), std::out_of_range);

    /* Outside Mach range */
    EXPECT_THROW(deck_value.get_value_at({1.00, 10, 1.01}), std::out_of_range);
    EXPECT_THROW(deck_value.get_value_at({1.00, 10, -0.01}), std::out_of_range);
}


/**
 * @brief Test parsing a big CSV file.
 */
TEST_F(ParseBigCSV, ValidData)
{
    /* Parse a deck value */
    DeckData deck_value = DeckData::from_csv(csv_file);

    /* Check the resulting properties */
    EXPECT_EQ(deck_value.name, "thrust");
}


/**
 * @brief Test returning the minimal possible operting point.
 */
TEST_F(ParseBigCSV, LowerBoundry)
{
    /* Parse a deck value */
    DeckValue deck_value{DeckData::from_csv(csv_file)};

    /*  */
    EXPECT_NEAR(deck_value.lower_boundary().N, 0.29, 1e-6);
    EXPECT_NEAR(deck_value.lower_boundary().Mach, 0.0, 1e-6);
    EXPECT_NEAR(deck_value.lower_boundary().altitude, 0.0, 1e-6);
}

/**
 * @brief Test returning the minimal possible operting point.
 */
TEST_F(ParseBigCSV, UpperBoundry)
{
    /* Parse a deck value */
    DeckValue deck_value{DeckData::from_csv(csv_file)};

    /*  */
    EXPECT_NEAR(deck_value.upper_boundary().N, 1.1, 1e-6);
    EXPECT_NEAR(deck_value.upper_boundary().Mach, 0.95, 1e-6);
    EXPECT_NEAR(deck_value.upper_boundary().altitude, 14000, 1e-6);
}


//TEST_F(ParseBigCSV, ValidOldData)
//{
//    /* Parse a deck value */
//    EngineDeck deck_value(csv_file.string());
//
//    /* Check the resulting properties */
//    EXPECT_TRUE(true);
//}
