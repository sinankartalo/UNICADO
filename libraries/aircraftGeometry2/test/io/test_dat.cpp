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
#include <string>
#include <iostream>

/* Unit Under Test */
#include "aircraftGeometry2/io/dat.h"

/* === Types === */
using Point_2 = geom2::Point_2;
using Polygon_2 = geom2::Polygon_2;

/* === Fixtures === */

/* === Tests === */
/**
 * @brief Test reading a tab-separated file.
 */
TEST(ReadDat, LineTabSeparated)
{
    /* Get the file path */
    std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
    file /= "dat-files/circle-tab.dat";

    /* Read the content */
    Polygon_2 result = geom2::io::read_dat_file(file);

    /* Check the total size */
    EXPECT_EQ(result.size(), 41);

    /* Check the first point */
    Point_2 firstPoint = result[0];
    EXPECT_EQ(firstPoint.x(), 0.5);
    EXPECT_EQ(firstPoint.y(), 0.0);

    /* Check second point */
    Point_2 secondPoint = result[1];
    EXPECT_EQ(secondPoint.x(), 0.49384);
    EXPECT_EQ(secondPoint.y(), 0.07822);
}

/**
 * @brief Test reading a comma-separated file.
 */
TEST(ReadDat, CommaSeparated)
{
    /* Get the file path */
    std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
    file /= "dat-files/circle-comma.dat";

    /* Read the content */
    Polygon_2 result = geom2::io::read_dat_file(file);

    /* Check the total size */
    EXPECT_EQ(result.size(), 41);

    /* Check the first point */
    Point_2 firstPoint = result[0];
    EXPECT_EQ(firstPoint.x(), 0.5);
    EXPECT_EQ(firstPoint.y(), 0.0);

    /* Check second point */
    Point_2 secondPoint = result[1];
    EXPECT_EQ(secondPoint.x(), 0.49384);
    EXPECT_EQ(secondPoint.y(), 0.07822);
}

/**
 * @brief Test reading a semicolon-separated file.
 */
TEST(ReadDat, SemicolonSeparated)
{
    /* Get the file path */
    std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
    file /= "dat-files/circle-semicolon.dat";

    /* Read the content */
    Polygon_2 result = geom2::io::read_dat_file(file);

    /* Check the total size */
    EXPECT_EQ(result.size(), 41);

    /* Check the first point */
    Point_2 firstPoint = result[0];
    EXPECT_EQ(firstPoint.x(), 0.5);
    EXPECT_EQ(firstPoint.y(), 0.0);

    /* Check second point */
    Point_2 secondPoint = result[1];
    EXPECT_EQ(secondPoint.x(), 0.49384);
    EXPECT_EQ(secondPoint.y(), 0.07822);
}

/**
 * @brief Test reading a non-existing file.
 */
TEST(ReadDat, FileNotFound)
{
    /* Get the file path */
    std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
    file /= "dat-files/does-not-exist.dat";

    /* Read the content */
    EXPECT_THROW(geom2::io::read_dat_file(file), std::runtime_error);
}

/**
 * @brief Test reading an airfoil dat file which should be sorted after reading.
 */
TEST(ReadAirfoil, LeadingEdgeFirst)
{
    /* Get the file path */
    std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
    file /= "dat-files/n0012-tab.dat";

    /* Read the content */
    Polygon_2 result = geom2::io::read_airfoil(file);

    /* Check the total size */
    EXPECT_EQ(result.size(), 200);
    EXPECT_TRUE(result.is_simple());

    /* Check the first point */
    Point_2 firstPoint = result[0];
    EXPECT_EQ(firstPoint.x(), 0.0);
    EXPECT_EQ(firstPoint.y(), 0.0);
}

/**
 * @brief Test reading an airfoil dat file which defined the trailing edge first.
 */
TEST(ReadAirfoil, TrailingEdgeFirst)
{
    /* Get the file path */
    std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
    file /= "dat-files/F15_12.dat";

    /* Read the content */
    Polygon_2 result = geom2::io::read_airfoil(file);

    /* Check the total size */
    EXPECT_EQ(result.size(), 158);
    EXPECT_TRUE(result.is_simple());

    /* Check the first point */
    Point_2 firstPoint = result[0];
    EXPECT_EQ(firstPoint.x(), 0.0);
    EXPECT_EQ(firstPoint.y(), 0.0);
}

/**
 * @brief Test reading invalid airfoil dat file.
 */
TEST(ReadAirfoil, InvalidAirfoil)
{
    /* When the leading edge is not at [0,0] should throw std::domain_error */
    std::filesystem::path file{CMAKE_TEST_STUBS_DIR};
    file /= "dat-files/n0012-wrong-origin.dat";
    EXPECT_THROW(geom2::io::read_airfoil(file), std::domain_error);

    /* Coordinates which are in the wrong direction in x should throw std::domain_error */
    file = std::filesystem::path{CMAKE_TEST_STUBS_DIR};
    file /= "dat-files/n0012-wrong-direction.dat";
    EXPECT_THROW(geom2::io::read_airfoil(file), std::domain_error);
}
