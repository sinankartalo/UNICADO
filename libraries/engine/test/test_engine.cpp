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
#include "engine/engine.h"

/* === Fixtures === */
class V2725_A5 : public ::testing::Test
{
  protected:

    /* Test setup */
    void SetUp() override
    {
        engine_path /=  "V2527-A5";
        if (!std::filesystem::exists(engine_path))
        {
            FAIL() << "The engine directory" << engine_path << "does not exist!";
        }
    }
    std::filesystem::path engine_path{CMAKE_TEST_STUBS_DIR};

};


/**
 * @struct Parameter for penalty calculations
 * @brief Struct to hold test parameters
 * @param flight_level: Current flight level [m]
 * @param mach_number: Current mach_number number [-]
 * @param derate: Value between 1.0 and 0.1 to artificially throttle the engine [-]
 * @param thrust_rating: Current rating [takeoff, maximum_continuous, climb, cruise, idle]
 * @param bleed_air_offtake: Current bleed air offtake [kg/s]
 * @param shaft_power_offtake: Current shaft power offtake [W]
 * @param thrust_limit: Aircraft thrust limitation. The resulting thrust is not allowed to exceed the thrust_limit, except if thrust_limit is lower than the thrust at the lowest possible shaft speed. [N]
 * @param scale_factor The thrust scaling to apply to this engine.
 */
struct PenaltyTestParams {
    double flight_level, mach_number, derate;
    std::string thrust_rating;
    double bleed_air_offtake, shaft_power_offtake, thrust_limit, scale_factor;
    double expected_N1_with_penalties, expected_N1_with_thrust_limit;
};

class V2725_A5_ParameterizedTestFixture : public ::testing::TestWithParam<PenaltyTestParams>
{
  protected:

    /* Test setup */
    void SetUp() override
    {
        engine_path /=  "V2527-A5";
        if (!std::filesystem::exists(engine_path))
        {
            FAIL() << "The engine directory" << engine_path << "does not exist!";
        }
    }
    std::filesystem::path engine_path{CMAKE_TEST_STUBS_DIR};

};

/* ============ Instantiate the test suite with test cases ============== */
INSTANTIATE_TEST_SUITE_P(
    N1WithPenaltiesCheckAbsValues_Instantiate,
    V2725_A5_ParameterizedTestFixture,
    ::testing::Values(
        PenaltyTestParams{10000, 0.8, 1.0, "cruise", 1.0, 5000.0, 10000.0, 1.0, 0.973989, 0.823579},
        PenaltyTestParams{ 5000, 0.8, 1.0, "cruise", 1.0, 5000.0, 10000.0, 1.0, 0.884718, 0.707300},
        PenaltyTestParams{    0, 0.8, 1.0, "cruise", 1.0, 5000.0, 10000.0, 1.0, 0.780151, 0.639215},
        PenaltyTestParams{10000, 0.4, 1.0, "cruise", 1.0, 5000.0, 10000.0, 1.0, 0.983613, 0.769528},
        PenaltyTestParams{10000, 0.0, 1.0, "cruise", 1.0, 5000.0, 10000.0, 1.0, 0.986063, 0.627450},
        PenaltyTestParams{10000, 0.8, 1.0, "cruise", 0.5, 5000.0, 10000.0, 1.0, 0.971545, 0.820755},
        PenaltyTestParams{10000, 0.8, 1.0, "cruise", 0.0, 5000.0, 10000.0, 1.0, 0.969100, 0.817931},
        PenaltyTestParams{10000, 0.8, 1.0, "cruise", 1.0, 2500.0, 10000.0, 1.0, 0.974162, 0.823725},
        PenaltyTestParams{10000, 0.8, 1.0, "cruise", 1.0,    0.0, 10000.0, 1.0, 0.974335, 0.823871},
        PenaltyTestParams{10000, 0.8, 1.0, "cruise", 1.0, 5000.0,  5000.0, 1.0, 0.973989, 0.714144},
        PenaltyTestParams{10000, 0.8, 1.0, "cruise", 1.0, 5000.0,   500.0, 1.0, 0.973989, 0.568585}
    )
);

/* ======= Test Default Values ======== */

/**
 * @brief Test construction of the V2 engine.
 */
TEST_F(V2725_A5, Constructor)
{
    /* Create an existing engine */
    Engine engine{engine_path};

    /* Check the resulting properties */
    EXPECT_EQ(engine.get_unscaled_engine().name(), "V2527-A5");
    EXPECT_DOUBLE_EQ(engine.get_scale_factor(), 1.0);
    EXPECT_NEAR(engine.get_engine_dimensions().width, 1.829, 1e-3);
    EXPECT_NEAR(engine.get_engine_dimensions().height, 2.118, 1e-3);
    EXPECT_NEAR(engine.get_engine_dimensions().length, 2.508, 1e-3);
    EXPECT_NEAR(engine.get_engine_dimensions().diameter, 1.613, 1e-3);

    /* Test the copy constructor */
    Engine engine_copy{engine};
    EXPECT_NE(&engine, &engine_copy);
    EXPECT_EQ(engine_copy.get_unscaled_engine().name(), "V2527-A5");
    EXPECT_EQ(engine_copy.get_scale_factor(), 1.0);
}

/**
 * @brief Test getting the operating point of the engine when not set.
 */
TEST_F(V2725_A5, DefaultOperatingPoint)
{
    /* Create an existing engine */
    Engine engine{engine_path};

    /* Get the operating point */
    OperatingPoint op = engine.get_operating_point();

    /* Check if the operating point is the default one */
    EXPECT_NEAR(op.altitude, engine.get_unscaled_engine().design_point().altitude, 1e-3);
    EXPECT_NEAR(op.Mach, engine.get_unscaled_engine().design_point().Mach, 1e-3);
    EXPECT_NEAR(op.N, 1.0, 1e-3);
};


/* ========== Test Setter functions ============ */

/**
 * @brief Test setting & getting the operating point of the engine.
 */
TEST_F(V2725_A5, SetNGetOperatingPoint)
{
    /* Create an existing engine */
    Engine engine{engine_path};

    /* Set the operating point */
    OperatingPoint op;
    op.altitude = 10668.0;
    op.Mach = 0.8;
    op.N = 1.0;
    engine.set_operating_point(op);

    /* Check if the operating point was set */
    EXPECT_NEAR(engine.get_operating_point().altitude, 10668.0, 1e-3);
    EXPECT_NEAR(engine.get_operating_point().Mach, 0.8, 1e-3);
    EXPECT_NEAR(engine.get_operating_point().N, 1.0, 1e-3);
}

/**
 * @brief Test setting & getting the amount of engines
 */
TEST_F(V2725_A5, SetNGetSameEngineTypeQuantity)
{
    /* Create the test engine */
    Engine engine{engine_path};
    
    /* check if int works */
    engine.set_same_engine_type_quantity(2);
    EXPECT_NEAR(engine.get_same_engine_type_quantity(), 2, 1e-6);

    /* check if a double is cut off */
    engine.set_same_engine_type_quantity(5.6789);
    EXPECT_NEAR(engine.get_same_engine_type_quantity(), 5, 1e-6);

    /* check that negative numbers don't work*/
    engine.set_same_engine_type_quantity(-5.3415);
    EXPECT_NEAR(engine.get_same_engine_type_quantity(), 5, 1e-6);
}

/**
 * @brief test N1 calculation while idle 
 */
TEST_F(V2725_A5, CalculateN1WithPenalties)
{
    /* Define input parameters */
    double scale_factor{1.3};
    OperatingPoint op{0.6, 0.0, 0.0};
    atmosphere atm{};
    atm.setAtmosphere(0, ISA_TEMPERATURE, ISA_PRESSURE);
    double derate{1.0};
    std::string thrust_rating{"takeoff"};
    double bleed_air_offtake{1.0}; // kg/s
    double shaft_power_offtake{5000.0}; // W

    /* Create the test engines */
    Engine engine_unscaled_without_penalties{engine_path};
    Engine engine_unscaled_with_penalties{engine_path};
    Engine engine_scaled_without_penalties{engine_path, scale_factor};
    Engine engine_scaled_with_penalties{engine_path, scale_factor};

    /* Set operating point */
    engine_unscaled_without_penalties.set_operating_point(op);
    engine_unscaled_with_penalties.set_operating_point(op);
    engine_scaled_without_penalties.set_operating_point(op);
    engine_scaled_with_penalties.set_operating_point(op);

    /* call the function to test */
    engine_unscaled_without_penalties.calculate_N1_with_penalties(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                0.0,
                                                                0.0);
    engine_unscaled_with_penalties.calculate_N1_with_penalties(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                bleed_air_offtake,
                                                                shaft_power_offtake);
    engine_scaled_without_penalties.calculate_N1_with_penalties(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                0.0,
                                                                0.0);
    engine_scaled_with_penalties.calculate_N1_with_penalties(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                bleed_air_offtake,
                                                                shaft_power_offtake);

    /* compare relative shaft speeds */
    EXPECT_GT(engine_unscaled_with_penalties.get_operating_point().N, 
              engine_unscaled_without_penalties.get_operating_point().N);
    EXPECT_GT(engine_scaled_with_penalties.get_operating_point().N, 
              engine_scaled_without_penalties.get_operating_point().N);

    /* compare relative fuel flows */
    EXPECT_GT(engine_unscaled_with_penalties.get_aircraft_fuelflow(), 
              engine_unscaled_without_penalties.get_aircraft_fuelflow());
    EXPECT_GT(engine_scaled_with_penalties.get_aircraft_fuelflow(), 
              engine_scaled_without_penalties.get_aircraft_fuelflow());
    EXPECT_GT(engine_scaled_with_penalties.get_aircraft_fuelflow(), 
              engine_unscaled_with_penalties.get_aircraft_fuelflow());
    EXPECT_GT(engine_scaled_without_penalties.get_aircraft_fuelflow(), 
              engine_unscaled_without_penalties.get_aircraft_fuelflow());
}

/**
 * @brief test absolute values of CalculateN1WithPenalties
 */
TEST_P(V2725_A5_ParameterizedTestFixture, N1WithPenaltiesCheckAbsValues) {
    PenaltyTestParams params = GetParam();
    Engine engine{engine_path};
    OperatingPoint op{0.0 ,params.flight_level, params.mach_number};
    atmosphere atm{};

    atm.setAtmosphere(params.flight_level, ISA_TEMPERATURE, ISA_PRESSURE);
    engine.set_operating_point(op);
    engine.calculate_N1_with_penalties(params.flight_level, params.mach_number, atm, params.derate, params.thrust_rating, params.bleed_air_offtake, params.shaft_power_offtake);

    // check absoliute values
    EXPECT_NEAR(engine.get_operating_point().N, params.expected_N1_with_penalties, 1e-6);
}

/**
 * @brief test if thrust is below the thrust limit
 */
TEST_F(V2725_A5, CalculateN1WithThrustLimitWithoutPenalties)
{
    /* Define input parameters */
    double scale_factor{1.0};
    OperatingPoint op{0.8, 0.0, 0.0};
    atmosphere atm{};
    atm.setAtmosphere(0, ISA_TEMPERATURE, ISA_PRESSURE);
    double derate{1.0};
    std::string thrust_rating{"takeoff"};
    double bleed_air_offtake{1.0}; // kg/s
    double shaft_power_offtake{8000.0}; // W
    double thrust_limit{80000}; // [N]

    /* Create the test engines */
    Engine engine_unscaled_without_penalties{engine_path};
    Engine engine_scaled_without_penalties{engine_path, scale_factor};

    /* Set operating point */
    engine_unscaled_without_penalties.set_operating_point(op);
    engine_scaled_without_penalties.set_operating_point(op);

    /* call the function to test */
    engine_unscaled_without_penalties.calculate_N1_with_thrustlimit(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                0.0,
                                                                0.0,
                                                                thrust_limit);
    engine_scaled_without_penalties.calculate_N1_with_thrustlimit(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                0.0,
                                                                0.0,
                                                                thrust_limit);

    /* check that no thrust limits are exceeded */
    EXPECT_GT(thrust_limit, engine_unscaled_without_penalties.get_thrust_aircraft());
    EXPECT_GT(thrust_limit, engine_scaled_without_penalties.get_thrust_aircraft());
}

/**
 * @brief test if thrust is below the thrust limit
 */
TEST_F(V2725_A5, CalculateN1WithThrustLimitAndPenalties)
{
    /* Define input parameters */
    double scale_factor{1.3};
    OperatingPoint op{0.8, 0.0, 0.0};
    atmosphere atm{};
    atm.setAtmosphere(0, ISA_TEMPERATURE, ISA_PRESSURE);
    double derate{1.0};
    std::string thrust_rating{"takeoff"};
    double bleed_air_offtake{1.0}; // kg/s
    double shaft_power_offtake{8000.0}; // W
    double thrust_limit{80000}; // [N]

    /* Create the test engines */
    Engine engine_unscaled_without_penalties{engine_path};
    Engine engine_unscaled_with_penalties{engine_path};
    Engine engine_scaled_without_penalties{engine_path, scale_factor};
    Engine engine_scaled_with_penalties{engine_path, scale_factor};

    engine_unscaled_without_penalties.set_same_engine_type_quantity(2);
    engine_unscaled_with_penalties.set_same_engine_type_quantity(2);
    engine_scaled_without_penalties.set_same_engine_type_quantity(2);
    engine_scaled_with_penalties.set_same_engine_type_quantity(2);

    /* Set operating point */
    engine_unscaled_without_penalties.set_operating_point(op);
    engine_unscaled_with_penalties.set_operating_point(op);
    engine_scaled_without_penalties.set_operating_point(op);
    engine_scaled_with_penalties.set_operating_point(op);

    /* call the function to test */
    engine_unscaled_without_penalties.calculate_N1_with_thrustlimit(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                0.0,
                                                                0.0,
                                                                thrust_limit);
    engine_unscaled_with_penalties.calculate_N1_with_thrustlimit(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                bleed_air_offtake,
                                                                shaft_power_offtake,
                                                                thrust_limit);
    engine_scaled_without_penalties.calculate_N1_with_thrustlimit(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                0.0,
                                                                0.0,
                                                                thrust_limit);
    engine_scaled_with_penalties.calculate_N1_with_thrustlimit(op.altitude,
                                                                op.Mach,
                                                                atm,
                                                                derate,
                                                                thrust_rating,
                                                                bleed_air_offtake,
                                                                shaft_power_offtake,
                                                                thrust_limit);

    /* check that no thrust limits are exceeded */
    EXPECT_NEAR(thrust_limit, engine_unscaled_without_penalties.get_thrust_aircraft(),1e-5);
    EXPECT_NEAR(thrust_limit, engine_unscaled_with_penalties.get_thrust_aircraft(),1e-5);
    EXPECT_NEAR(thrust_limit, engine_scaled_without_penalties.get_thrust_aircraft(),1e-5);
    EXPECT_NEAR(thrust_limit, engine_scaled_with_penalties.get_thrust_aircraft(),1e-5);

    /*Check the operating points */
    EXPECT_NEAR(engine_unscaled_without_penalties.get_operating_point().N, 0.6359, 1e-3);
    EXPECT_NEAR(engine_unscaled_with_penalties.get_operating_point().N, 0.6362, 1e-3);
    EXPECT_NEAR(engine_scaled_without_penalties.get_operating_point().N, 0.5661, 1e-3);
    EXPECT_NEAR(engine_scaled_with_penalties.get_operating_point().N, 0.5664, 1e-3);

    /* compare relative shaft speeds */
    EXPECT_GT(engine_unscaled_with_penalties.get_operating_point().N, 
              engine_unscaled_without_penalties.get_operating_point().N);
    EXPECT_GT(engine_scaled_with_penalties.get_operating_point().N, 
              engine_scaled_without_penalties.get_operating_point().N);
    
    /* compare relative fuel flows */
    EXPECT_GT(engine_unscaled_with_penalties.get_aircraft_fuelflow(), 
              engine_unscaled_without_penalties.get_aircraft_fuelflow());
    EXPECT_GT(engine_scaled_with_penalties.get_aircraft_fuelflow(), 
              engine_scaled_without_penalties.get_aircraft_fuelflow());
    EXPECT_GT(engine_scaled_with_penalties.get_aircraft_fuelflow(), 
              engine_unscaled_with_penalties.get_aircraft_fuelflow());
    EXPECT_NEAR(engine_scaled_without_penalties.get_aircraft_fuelflow(), 
              engine_unscaled_without_penalties.get_aircraft_fuelflow(),1e-3);
}

/**
 * @brief test absolute values of CalculateN1WithThrustLimit
 */
TEST_P(V2725_A5_ParameterizedTestFixture, N1WithThrustLimitCheckAbsValues) {
    PenaltyTestParams params = GetParam();
    Engine engine{engine_path};
    OperatingPoint op{0.0 ,params.flight_level, params.mach_number};
    atmosphere atm{};

    atm.setAtmosphere(params.flight_level, ISA_TEMPERATURE, ISA_PRESSURE);
    engine.set_operating_point(op);
    engine.calculate_N1_with_thrustlimit(params.flight_level, 
                                        params.mach_number, 
                                        atm, 
                                        params.derate, 
                                        params.thrust_rating, 
                                        params.bleed_air_offtake, 
                                        params.shaft_power_offtake,
                                        params.thrust_limit);

    // check absoliute values
    EXPECT_NEAR(engine.get_operating_point().N, params.expected_N1_with_thrust_limit, 1e-6);
}

/* ========= Test Getter Functions ========== */

/**
 * @brief Test getting the scale factor.
 */
TEST_F(V2725_A5, GetScaleFactor)
{
    /* Create a scaled engine */
    Engine engine0{engine_path, 1.2345678};
    Engine engine1{engine_path, 6};
    //Engine engine2{engine_path, -1.2345678};
  
    EXPECT_NEAR(engine0.get_scale_factor(), 1.2345678, 1e-3);
    EXPECT_NEAR(engine1.get_scale_factor(), 6, 1e-3);
    //EXPECT_NEAR(engine2.get_scale_factor(), -1.2345678, 1e-3);
}

/**
 * @brief Test getting the scaled engine dimensions.
 */
TEST_F(V2725_A5, GetScaledDimensions)
{
    /* Create a scaled engine */
    Engine engine{engine_path, 2.0};

    /* Check the resulting dimensions */
    auto result = engine.get_engine_dimensions();
    EXPECT_NEAR(result.height, 2.995, 1e-3);
    EXPECT_NEAR(result.width, 2.586, 1e-3);
    EXPECT_NEAR(result.length, 3.309, 1e-3);
    EXPECT_NEAR(result.diameter, 2.281, 1e-3);
}


/* ================ Test Getting Deck Values =============== */

/**
 * @brief Test getting the thrust of the engine.
 */
TEST_F(V2725_A5, GetThrust)
{
    /* Create the test engine */
    Engine engine{engine_path};
    Engine engine_scaled{engine_path, 2.0};

    /* Create operating point */
    OperatingPoint op{1.0, 1000.0, 0.8};

    /* Test the unscaled thrust */
    engine.set_operating_point(op);
    engine_scaled.set_operating_point(op);

    EXPECT_NEAR(engine.get_thrust_aircraft(), 74.989*1000, 0.1);
    EXPECT_NEAR(engine_scaled.get_thrust_aircraft(), 2.0 * 74.989*1000, 0.1);
    EXPECT_NEAR(engine_scaled.get_thrust(), engine_scaled.get_thrust_aircraft(), 0.001);
}

/**
 * @brief Test getting the fuel flow of the engine.
 */
TEST_F(V2725_A5, GetFuelFlow)
{
    double scale_factor{2.0};
    int number_of_engines{3};

    /* Create the test engines */
    Engine engine_unscaled{engine_path};
    Engine engine_scaled{engine_path, scale_factor};
    Engine multiple_engines_unscaled{engine_path};
    Engine multiple_engines_scaled{engine_path, scale_factor};

    /* Create operating point */
    OperatingPoint op{1.0, 1000.0, 0.8};

    /* Set operating point */
    engine_unscaled.set_operating_point(op);
    engine_scaled.set_operating_point(op);
    multiple_engines_unscaled.set_operating_point(op);
    multiple_engines_scaled.set_operating_point(op);

    multiple_engines_unscaled.set_same_engine_type_quantity(number_of_engines);
    multiple_engines_scaled.set_same_engine_type_quantity(number_of_engines);
    
    /* check fuel flow */
    EXPECT_NEAR(engine_unscaled.get_aircraft_fuelflow(), 1.4581, 1e-3);
    EXPECT_NEAR(engine_scaled.get_aircraft_fuelflow(), scale_factor * 1.4581, 1e-3);
    EXPECT_NEAR(multiple_engines_unscaled.get_aircraft_fuelflow(), number_of_engines * engine_unscaled.get_aircraft_fuelflow(), 1e-3);
    EXPECT_NEAR(multiple_engines_scaled.get_aircraft_fuelflow(), number_of_engines * engine_scaled.get_aircraft_fuelflow(), 1e-3);
}

///**
// * @brief Test getting the convergence.
// */
//TEST_F(V2725_A5, GetConvergence)
//{
//    /* Create the test engine */
//    Engine engine{engine_path};
//
//    /* Create operating point */
//    OperatingPoint op{1.0, 0.0, 0.8};
//
//    double testvalue = engine.converge_value(op, 100000, 0.05, "_FN");
//    /* Test the unscaled thrust */
//    EXPECT_NEAR(testvalue,0.9,0.01);
//}

// /**
//  * @brief Tests gettig the engine's EGT.
//  */
// TEST_F(V2725_A5, GetEngineEGT)
// {
//     /* Create the test engines */
//     Engine engine{engine_path};
// 
//     /* Create operating point */
//     OperatingPoint op{1.0, 1000.0, 0.8};
// 
//     /* Set operating point */
//     engine.set_operating_point(op);
// 
//     EXPECT_NEAR(engine.get_engine_EGT(), 9999, 1e-6);
// 
//     // TODO: Tobi: Add EGT.csv file
//     // TODO: Oli: Add values from .csv
// }
// 
// /**
//  * @brief Tests gettig the engine's EPR.
//  */
// TEST_F(V2725_A5, GetEngineEPR)
// {
//     /* Create the test engines */
//     Engine engine{engine_path};
// 
//     /* Create operating point */
//     OperatingPoint op{1.0, 1000.0, 0.8};
// 
//     /* Set operating point */
//     engine.set_operating_point(op);
// 
//     EXPECT_NEAR(engine.get_engine_EPR(), 9999, 1e-6);
// 
//     // TODO: Tobi: Add EPR.csv file
//     // TODO: Oli: Add values from .csv
// }
// 
// /**
//  * @brief Tests gettig the engine's OPR.
//  */
// TEST_F(V2725_A5, GetEngineOPR)
// {
//     /* Create the test engines */
//     Engine engine{engine_path};
// 
//     /* Create operating point */
//     OperatingPoint op{1.0, 1000.0, 0.8};
// 
//     /* Set operating point */
//     engine.set_operating_point(op);
// 
//     EXPECT_NEAR(engine.get_engine_OPR(), 9999, 1e-6); 
//     
//     // TODO: Tobi: Add OPR.csv file
//     // TODO: Oli: Add values from .csv
// }

/**
 * @brief Tests gettig the engine's sNOx factor.
 */
TEST_F(V2725_A5, GetEngineSNOxEmissionIndex)
{
    /* Create the test engines */
    Engine engine{engine_path};

    /* Create operating point */
    OperatingPoint op{1.0, 1000.0, 0.8};

    /* Set operating point */
    engine.set_operating_point(op);

    EXPECT_NEAR(engine.get_engine_NOx_emission_index(), 32 * 1551.8, 1e-6);
}

/**
 * @brief Tests the emissions factors for all LTO cycles according to ICAO.
 */
TEST_F(V2725_A5, GetLTOEmissionIndex)
{
    /* Create the test engines */
    Engine engine{engine_path};

    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::taxi,  EngineEmissions::HC), 0.105e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::taxi,  EngineEmissions::CO), 12.43e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::taxi,  EngineEmissions::NOx), 4.7e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::taxi,  EngineEmissions::SN), 2.6, 1e-6);

    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::takeoff,  EngineEmissions::HC), 0.041e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::takeoff,  EngineEmissions::CO), 0.53e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::takeoff,  EngineEmissions::NOx), 26.5e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::takeoff,  EngineEmissions::SN), 5.2, 1e-6);

    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::climb,  EngineEmissions::HC), 0.041e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::climb,  EngineEmissions::CO), 0.62e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::climb,  EngineEmissions::NOx), 22.3e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::climb,  EngineEmissions::SN), 7.2, 1e-6);

    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::approach,  EngineEmissions::HC), 0.061e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::approach,  EngineEmissions::CO), 2.44e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::approach,  EngineEmissions::NOx), 8.9e-3, 1e-6);
    EXPECT_NEAR(engine.get_LTO_emission_index(LTOPhases::approach,  EngineEmissions::SN), 4.2, 1e-6);
}

/**
 * @brief Tests the fuel flow factors for the LTO cycles according to ICAO.
 */
TEST_F(V2725_A5, GetLTOFuelFlow)
{
    /* Create the test engines */
    Engine engine{engine_path};

    EXPECT_NEAR(engine.get_LTO_fuelflow(LTOPhases::taxi), 0.128, 1e-6);
    EXPECT_NEAR(engine.get_LTO_fuelflow(LTOPhases::takeoff), 1.053, 1e-6);
    EXPECT_NEAR(engine.get_LTO_fuelflow(LTOPhases::climb), 0.88, 1e-6);
    EXPECT_NEAR(engine.get_LTO_fuelflow(LTOPhases::approach), 0.319, 1e-6);
}

/**
 * @brief Test getting the maximal shaft power offtake.
 */
TEST_F(V2725_A5, GetMaxShaftPowerOfftake)
{
    /* Create the test engines */
    Engine engine{engine_path};
    Engine engine_scaled{engine_path, 1.3};

    /* Test the unscaled max shaft power extration */
    EXPECT_NEAR(engine.get_max_shaft_power_offtake_engine(), 150000.0, 1e-3);
    /* Test the scaled max shaft power extration */
    EXPECT_NEAR(engine_scaled.get_max_shaft_power_offtake_engine(), 150000.0 * 1.3, 1e-3);
}

/**
 * @brief Test getting the maximal bleed air offtake at a specified operating point.
 */
TEST_F(V2725_A5, GetMaxBleedAirOfftake)
{
    /* Create the test engines */
    Engine engine{engine_path};
    Engine engine_scaled{engine_path, 1.3};

    /* Create operating point */
    OperatingPoint op{0.8, 10000.0, 0.8};

    /* Set operating point */
    engine.set_operating_point(op);
    engine_scaled.set_operating_point(op);

    /* Test the unscaled max shaft power extration */
    EXPECT_NEAR(engine.get_max_bleed_offtake_at_current_operating_point(), 0.3 * 19.415, 1e-3);
    /* Test the scaled max shaft power extration */
    EXPECT_NEAR(engine_scaled.get_max_bleed_offtake_at_current_operating_point(), 0.3 * 19.415 * 1.3, 1e-3);

    // from .xml      -> relMaxBleed = 0.3
    // from .csv @ OP -> core_mass_flow = 19.415 kg/s
}

/**
 * @brief Test getting the maximal bleed air offtake at a specified operating point.
 */
TEST_F(V2725_A5, GetMaxBleedAirOfftakeAtOP)
{
    /* Define scale factor */
    double scale_factor = 1.45;

    /* Create the test engines */
    Engine engine{engine_path, scale_factor};

    /* Create operating point */
    OperatingPoint op{0.8, 10000.0, 0.8};

    /* Set operating point */
    engine.set_operating_point(op);

    EXPECT_NEAR(engine.get_max_bleed_offtake_at_current_operating_point(), 0.3 * 19.415 * scale_factor, 1e-6);
}

/**
 * @brief Tests getting the physical properties from the defined engine stage.
 */
TEST_F(V2725_A5, GetPhysicalPropertiesStage)
{
    /* Create the test engines */
    Engine engine{engine_path};

    /* Create operating point */
    OperatingPoint op{0.8, 10000.0, 0.8};

    /* Set operating point */
    engine.set_operating_point(op);

    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St2), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St3), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St4), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St5), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St13), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St22), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St25), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St45), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St18), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::FuelToAirRatio, EngineStage::St8), 9999, 1e-6);
// 
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St2), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St3), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St4), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St5), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St13), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St22), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St25), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St45), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St18), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MachNumber, EngineStage::St8), 9999, 1e-6);
// 
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St2), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St3), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St4), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St5), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St13), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St22), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St25), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St45), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St18), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalPressure, EngineStage::St8), 9999, 1e-6);
// 
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St2), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St3), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St4), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St5), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St13), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St22), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St25), 9999, 1e-6);
    EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St45), 894.75, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St18), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::TotalTemperature, EngineStage::St8), 9999, 1e-6);
// 
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St2), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St3), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St4), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St5), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St13), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St22), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St25), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St45), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St18), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::StaticTemperature, EngineStage::St8), 9999, 1e-6);
// 
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St2), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St3), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St4), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St5), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St13), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St22), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St25), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St45), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St18), 9999, 1e-6);
    // EXPECT_NEAR(engine.get_physical_properties_stage(StageProperties::MassFlow, EngineStage::St8), 9999, 1e-6);

    // TODO(Tobi): Add .csv decks
    // TODO(Oli): Add values from .csv decks
}

/**
 * @brief Test getting the scaled SLST.
 */
TEST_F(V2725_A5, GetScaledSLST)
{
    /* Define scale factor */
    double scale_factor = 1.45;

    /* Create the test engines */
    Engine engine{engine_path, scale_factor};


    EXPECT_NEAR(engine.get_scaled_SLST(),  110312.6953 * scale_factor, 1e-6);
}


/**
 * @brief Test getting the scaled SLST.
 */
TEST_F(V2725_A5, GetScaledMass)
{
    /* Define scale factor */
    double scale_factor = 1.45;

    /* Create the test engines */
    Engine engine{engine_path, scale_factor};

    EXPECT_NEAR(engine.get_dry_mass(), 2404 * 1.50489016, 1e-6);
}