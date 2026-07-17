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
#include "aerodynamics/aerodynamics_v3.h"
#include "interpolation/data_types.h"


TEST(AircraftAero, TestAircraftInitialization)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v2/polar.xml";
	auto PolarXML = aixml::openDocument(file);
	aerodynamics::Aircraft::trim_settings settings;
	settings.method = "linearized";
	aerodynamics::Aircraft aircraft(PolarXML, settings);
}

TEST(AircraftAero, TestAircraftTrim)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v2/polar (3).xml";

	auto PolarXML = aixml::openDocument(file);
	aerodynamics::Aircraft::trim_settings settings;
	aerodynamics::Aircraft aircraft(PolarXML, settings);

	auto conditions = aerodynamics::Flight_Condition(10000., 0.76);

	aircraft.change_settings("main_wing", "horizontal_stabiliser", "linear");

	std::vector<double> cog_position = { 15.5, 0., 0. };

	aircraft.linearized_trim(conditions, 1500000., cog_position);

	auto wing_aoa = aircraft.settings.reference_wing_angle;
	auto ht_incidence = aircraft.settings.adjustable_surface_angle;

	std::cout << "\nwing angle of attack = " << wing_aoa << " deg\n";
	std::cout << "ht incidence = " << ht_incidence << " deg\n";
	std::cout << "L/D = " << aircraft.get_CL_CD(conditions, 1500000., cog_position) << "\n";
}

TEST(AircraftAero, TestAircraftTrimLogic)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v2/polar (3).xml";

	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft::trim_settings settings;
	aerodynamics::Aircraft aircraft(PolarXML, settings);

	auto conditions = aerodynamics::Flight_Condition(10000., 0.76);

	aircraft.change_settings("main_wing", "horizontal_stabiliser", "linear");

	std::vector<double> cog_position = { 13.5, 0., 0. };

	aircraft.linearized_trim(conditions, 1500000., cog_position);

	auto wing_aoa = aircraft.settings.reference_wing_angle;
	auto ht_incidence = aircraft.settings.adjustable_surface_angle;

	std::cout << "\nwing angle of attack = " << wing_aoa << " deg\n";
	std::cout << "ht incidence = " << ht_incidence << " deg\n";
	std::cout << "L/D = " << aircraft.get_CL_CD(conditions, 1500000., cog_position) << "\n";
	
	std::vector<double> weights = { 100000., 300000., 500000., 700000., 900000., 1100000., 1300000., 1500000., 1700000., 1900000., 2100000. };
	weights = { 700000. };
	std::vector<double> machs = { 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.76, 0.78 };

	std::vector<double> L_D = {};
	
	for (auto mach : machs)
	{
		conditions = aerodynamics::Flight_Condition(10000., mach);
		std::cout << "CL = ";
		for (auto weight : weights)
		{
			aircraft.linearized_trim(conditions, weight, cog_position);
			std::cout << aircraft.get_CL(conditions, weight) << ", ";
		}
		std::cout << "\n";
		std::cout << "L/D = ";
		for (auto weight : weights)
		{
			aircraft.linearized_trim(conditions, weight, cog_position);
			std::cout << aircraft.get_CL_CD(conditions, weight, cog_position) << ", ";
		}
		std::cout << "\n";
	}
}

TEST(AircraftAero, TestAircraftTrimLogic2)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v2/polar (3).xml";

	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft::trim_settings settings;
	aerodynamics::Aircraft aircraft(PolarXML, settings);

	auto conditions = aerodynamics::Flight_Condition(10000., 0.76);

	aircraft.change_settings("main_wing", "horizontal_stabiliser", "linear");

	std::vector<double> cog_position = { 13.5, 0., 0. };

	std::vector<double> weights = { 100000., 300000., 500000., 700000., 900000., 1100000., 1300000., 1500000., 1700000., 1900000., 2100000. };
	weights = { 700000. };
	std::vector<double> machs = { 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.76, 0.78 };

	std::vector<double> L_D = {};

	for (auto mach : machs)
	{
		conditions = aerodynamics::Flight_Condition(10000., mach);
		std::cout << "L/D = ";
		for (auto weight : weights)
		{
			aircraft.linearized_trim(conditions, weight, cog_position);
			std::cout << aircraft.get_CL_CD(conditions, weight, cog_position) << ", ";
		}
		std::cout << "\n";
	}
}

TEST(AircraftAero, TestAircraftInitialization_NonLinearized)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v2/polar.xml";
	auto PolarXML = aixml::openDocument(file);
	aerodynamics::Aircraft::trim_settings settings;
	settings.method = "non_linearized";
	aerodynamics::Aircraft aircraft(PolarXML, settings);

	types::PropertyType conditions;

	conditions["Mach"] = 0.667;
	conditions["h"] = 8000.;
	conditions["alpha"] = 3.76;

	auto CL_wing = aircraft.components["main_wing"].get_property("CL", conditions);
	std::cout << "main wing CL = " << CL_wing << "\n";

	auto CM_wing = aircraft.components["main_wing"].get_property("CM", conditions);
	std::cout << "main wing CM = " << CM_wing << "\n";

	auto CD_wing = aircraft.components["main_wing"].get_property("CD", conditions);
	std::cout << "main wing CD = " << CD_wing << "\n";
}