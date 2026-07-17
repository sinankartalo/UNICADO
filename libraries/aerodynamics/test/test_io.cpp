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

TEST(IOReadPolar, TestPolarInitializationV1)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	auto XML_polar = PolarXML->at("components/component@main-wing/configurations/configuration@clean/polars/polar@1");

	aerodynamics::Polar polar(XML_polar, 1);
}

TEST(IOReadPolar, TestPolarInitializationV1_value)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar_value.xml";
	auto PolarXML = aixml::openDocument(file);

	auto XML_polar = PolarXML->at("components/component@main-wing/configurations/configuration@clean/polars/polar@1");

	aerodynamics::Polar polar(XML_polar, 1);
}

TEST(IOReadPolar, TestConfigurationPolarInitializationV1)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	auto XML_polar = PolarXML->at("components/component@main-wing/configurations/configuration@clean");

	aerodynamics::Configuration configuration(XML_polar, 1);
}

TEST(IOReadPolar, TestComponentPolarInitializationV1)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	auto XML_polar = PolarXML->at("components/component@main-wing");

	aerodynamics::Component component(XML_polar, 1, 126.4, 5.);
}

TEST(IOReadPolar, TestComponentPolarInitializationV1_2)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	auto XML_polar = PolarXML->at("components/component@main-wing");

	aerodynamics::Component component(XML_polar, 1);
}

TEST(IOReadPolar, TestAircraftPolarInitializationV1)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };
	file /= "PEF-v1/polar.xml";
	auto PolarXML = aixml::openDocument(file);

	aerodynamics::Aircraft aircraft(PolarXML);
}

TEST(IOReadPolar, TestPolarWriterV1)
{
	std::filesystem::path file{ CMAKE_TEST_STUBS_DIR };

	auto PolarXML = aixml::openDocument(file / "PEF-v1/polar.xml");

	auto XML_polar = PolarXML->at("components/component@main-wing/configurations/configuration@clean/polars/polar@1");

	aerodynamics::Polar polar(XML_polar, 1);

	aerodynamics::create_polar_xml(file / "PEF-v1/polar_output_test.xml", 1);

	auto out_polar_xml = aixml::openDocument(file / "PEF-v1/polar_output_test.xml");

	std::string surface_name = "main-wing";
	out_polar_xml->operator[]("polar-exchange-file/components/component@" + surface_name);
	out_polar_xml->operator[]("polar-exchange-file/components/component@" + surface_name + "/configurations/configuration@clean");
	out_polar_xml->operator[]("polar-exchange-file/components/component@" + surface_name + "/configurations/configuration@clean/polars/polar@1");
	out_polar_xml->operator[]("polar-exchange-file/components/component@" + surface_name + "/configurations/configuration@clean/polars/polar@2");

	node& polar_node = out_polar_xml->at("polar-exchange-file/components/component@main-wing/configurations/configuration@clean/polars/polar@2");

	polar.XML_writer(&polar_node, 1);

	aixml::saveDocument(*out_polar_xml, 1);
}

TEST(IOReadPolar, TestRECalculation1)
{
	auto condition_main = aerodynamics::Flight_Condition(3000., 0.2);

	auto condition_test = aerodynamics::Flight_Condition(0.2, condition_main.RE, 1.);

	EXPECT_NEAR(condition_main.altitude, condition_test.altitude, 1e-1);
	std::cout << condition_main.RE << "\n";
}

TEST(IOReadPolar, TestRECalculation2)
{
	auto condition_main = aerodynamics::Flight_Condition(3000., 0.45);

	auto condition_test = aerodynamics::Flight_Condition(0.45, condition_main.RE, 1.);

	EXPECT_NEAR(condition_main.altitude, condition_test.altitude, 1e-1);
	std::cout << condition_main.RE << "\n";
}

TEST(IOReadPolar, TestRECalculation3)
{
	auto condition_main = aerodynamics::Flight_Condition(3000., 0.5);

	auto condition_test = aerodynamics::Flight_Condition(0.5, condition_main.RE, 1.);

	EXPECT_NEAR(condition_main.altitude, condition_test.altitude, 1e-1);
	std::cout << condition_main.RE << "\n";
}