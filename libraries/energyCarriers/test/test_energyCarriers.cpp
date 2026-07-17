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

#include "energyCarriers/energyCarriers.h"
#include <gtest/gtest.h>
#include <stdexcept>

// Test for valid input "kerosene"
TEST(EnergyCarrierTest, ValidKerosene) {
    EnergyCarrier carrier("kerosene", 800.0);  // Density of kerosene ~800 kg/m^3
    EXPECT_EQ(carrier.type, "kerosene");
    EXPECT_DOUBLE_EQ(carrier.density, 800.0);
    EXPECT_DOUBLE_EQ(carrier.gravimetric_energy_density, 43100000.0);
    EXPECT_DOUBLE_EQ(carrier.volumetric_energy_density, 800.0 * 43100000.0);
    EXPECT_DOUBLE_EQ(carrier.emission_index_CO2, 3.149);
    EXPECT_DOUBLE_EQ(carrier.emission_index_H2O, 1.2);
    EXPECT_DOUBLE_EQ(carrier.emission_index_SO2, 0.84e-3);
    EXPECT_DOUBLE_EQ(carrier.emission_index_SO4, 2e-4);
    EXPECT_DOUBLE_EQ(carrier.emission_index_soot, 0.025e-3);
}

// Test for valid input "liquid_hydrogen"
TEST(EnergyCarrierTest, ValidHydrogen) {
    EnergyCarrier carrier("liquid_hydrogen", 70.0);  // Density of liquid_hydrogen ~70 kg/m^3
    EXPECT_EQ(carrier.type, "liquid_hydrogen");
    EXPECT_DOUBLE_EQ(carrier.density, 70.0);
    EXPECT_DOUBLE_EQ(carrier.gravimetric_energy_density, 120000000.0);
    EXPECT_DOUBLE_EQ(carrier.volumetric_energy_density, 70.0 * 120000000.0);
    EXPECT_DOUBLE_EQ(carrier.emission_index_CO2, 0.0);
    EXPECT_DOUBLE_EQ(carrier.emission_index_H2O, 8.94);
    EXPECT_DOUBLE_EQ(carrier.emission_index_SO2, 0.0);
    EXPECT_DOUBLE_EQ(carrier.emission_index_SO4, 0.0);
    EXPECT_DOUBLE_EQ(carrier.emission_index_soot, 0.0);
}

// Test for invalid input type
TEST(EnergyCarrierTest, InvalidType) {
    EXPECT_THROW(
        {
            EnergyCarrier carrier("unknown", 500.0);
        },
        std::out_of_range);
}

// Test for zero density
TEST(EnergyCarrierTest, ZeroDensity) {
    EnergyCarrier carrier("kerosene", 0.0);
    EXPECT_EQ(carrier.volumetric_energy_density, 0.0);
}

// Test for negative density
TEST(EnergyCarrierTest, NegativeDensity) {
    EnergyCarrier carrier("kerosene", -10.0);
    EXPECT_EQ(carrier.volumetric_energy_density, -10.0 * 43100000.0);
}

// Main entry point for Google Test
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
