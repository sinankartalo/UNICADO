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
#include <aixml/node.h>
#include <aixml/endnode.h>
#include "aerodynamics/aerodynamics_v2.h"
#include "interpolation/interpolation.h"
#include "solver/solver.h"

TEST(AircraftAero, TestAircraftInitialization)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft aircraft(PolarXML);
}

TEST(ComponentAero, TestSetComponentLHS)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft aircraft(PolarXML);

	auto polar = aircraft.components["main-wing"].configurations[0].polars[0];
	std::unordered_map<std::string, value> container = {};

	aerodynamics::Component::LHS lhs(aircraft.components["main-wing"]);
	auto mach = get_vector(lhs.container["clean"]["M"]);
	auto h = get_vector(lhs.container["clean"]["h"]);
}

TEST(ComponentAero, TestSetComponentRHS)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft aircraft(PolarXML);

	auto polar = aircraft.components["main-wing"].configurations[0].polars[0];

	aerodynamics::Component::RHS rhs(aircraft.components["main-wing"]);
	aerodynamics::Component::LHS lhs(aircraft.components["main-wing"]);

	auto M_vec = get_vector(lhs.container["clean"]["M"]);
	auto h_vec = get_vector(lhs.container["clean"]["h"]);
}

TEST(AircraftAero, TestComponentPropertiesCL)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft aircraft(PolarXML);

	auto conditions = aerodynamics::Flight_Condition(9000., 0.52);

	aircraft.change_settings("main-wing", "horizontal-stabiliser", "linear");

	auto main_wing = aircraft.components["main-wing"];
	std::vector<double> v_conditions = { conditions.freestream_M, conditions.altitude };

	auto elem = main_wing.interpolator->conditions = v_conditions;
	main_wing.interpolator->update();

	auto coords = main_wing.interpolator->barycentric_coordinates;

	std::cout << "barycentric coordinates are\n";
	for (auto i : coords)
	{
		std::cout << i << "  ";
	}
	std::cout << "\n";
}

TEST(AircraftAero, TestAircraftCL)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft aircraft(PolarXML);

	auto conditions = aerodynamics::Flight_Condition(10000., 0.76);

	aircraft.change_settings("main-wing", "horizontal-stabiliser", "linear");

	auto CL = aircraft.get_CL(conditions, 70000.);

	auto L = 0.5 * conditions.density * pow(conditions.u, 2.) * aircraft.components["main-wing"].area * CL;

	EXPECT_NEAR(0, L-70000.,1e-9);
}

TEST(AircraftAero, TestAircraftTrim)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v2/polar (3).xml";
	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft aircraft(PolarXML);

	auto conditions = aerodynamics::Flight_Condition(10000., 0.3);

	aircraft.change_settings("main_wing", "horizontal_stabiliser", "linear");

	std::vector<double> cog_location = { 14., 0., 0. };

	aircraft.linearized_trim(conditions, 600000., cog_location);

	auto wing_aoa = aircraft.settings.reference_wing_angle;
	auto ht_incidence = aircraft.settings.adjustable_surface_angle;

	std::cout << "wing angle of attack = " << wing_aoa << " deg\n";
	std::cout << "ht incidence = " << ht_incidence << " deg\n";

	std::cout << "aircraft CD is = " << aircraft.get_CD(conditions, 600000.) << "\n";
	std::cout << "aircraft L/D is = " << aircraft.get_CL_CD(conditions, 600000., cog_location) << "\n";
}

TEST(AircraftAero, TestAircraftTrim_str)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v2/polar (4).xml";
	
	auto PolarXML = file.string();

	aerodynamics::Aircraft aircraft(PolarXML);

	auto conditions = aerodynamics::Flight_Condition(10000., 0.76);

	aircraft.change_settings("main_wing", "horizontal_stabiliser", "linear");

	std::vector<double> cog_location = { 15.5, 0., 0. };

	aircraft.linearized_trim(conditions, 600000., cog_location);

	auto wing_aoa = aircraft.settings.reference_wing_angle;
	auto ht_incidence = aircraft.settings.adjustable_surface_angle;

	std::cout << "wing angle of attack = " << wing_aoa << " deg\n";
	std::cout << "ht incidence = " << ht_incidence << " deg\n";

	std::cout << "aircraft CD is = " << aircraft.get_CD(conditions, 600000.) << "\n";
	std::cout << "aircraft L/D is = " << aircraft.get_CL_CD(conditions, 600000., cog_location) << "\n";
}