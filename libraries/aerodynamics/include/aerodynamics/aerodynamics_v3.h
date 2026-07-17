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

#ifndef AERODYNAMICS_AERODYNAMICS_V3_H_
#define AERODYNAMICS_AERODYNAMICS_V3_H_

#include "interpolation/interpolation_v2.h"
#include "interpolation/data_types.h"
#include "solver/solver_v2.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <atmosphere/atmosphere.h>
#include <variant>  
#include <aixml/node.h>
#include <aixml/endnode.h>

namespace aerodynamics
{
	auto vec_to_eigen(std::vector<double> vec) -> types::VectorXd;

	enum XML_Version : int
	{
		V1 = 1,
		V2 = 2
	};

	struct Flight_Condition
	{
		Flight_Condition(const double altitude, const double freestream_M) : altitude(altitude), freestream_M(freestream_M)
		{
			density = atm.getDensity(altitude);
			u = atm.getSpeedOfSound(altitude) * freestream_M;
			RE = density * u * 1. / atm.getViscosity(altitude);
		};

		Flight_Condition(
			 double u,
			 double v,
			 double w,
			 double p,
			 double q,
			 double r,
			 double altitude)
			: u(u), v(v), w(w), p(p), q(q), r(r), altitude(altitude)
		{
			density = atm.getDensity(altitude);
			freestream_M = u / atm.getSpeedOfSound(altitude);
		};

		Flight_Condition(double freestream_M, double RE, double characteristic_length) : freestream_M(freestream_M), RE(RE)
		{
			altitude = 5000;

			auto RE_0 = calculate_RE(freestream_M, altitude, characteristic_length);

			auto eRE_2 = RE - calculate_RE(freestream_M, altitude + 100., characteristic_length);
			auto eRE_1 = RE - calculate_RE(freestream_M, altitude - 100., characteristic_length);
			double deRE_dh = (eRE_2 - eRE_1) / 200.;

			double new_altitude = altitude - (RE - RE_0) / deRE_dh;

			while (std::abs(RE - RE_0) > 0.001)
			{
				altitude = new_altitude;
				RE_0 = calculate_RE(freestream_M, altitude, characteristic_length);
				eRE_2 = RE - calculate_RE(freestream_M, altitude + 100., characteristic_length);
				eRE_1 = RE - calculate_RE(freestream_M, altitude - 100., characteristic_length);
				deRE_dh = (eRE_2 - eRE_1) / 200.;
				new_altitude = altitude - (RE - RE_0) / deRE_dh;
			}
			u = atm.getSpeedOfSound(altitude) * freestream_M;
			density = atm.getDensity(altitude);
		}
		
		atmosphere atm;

		 double altitude = 0.;
		 double freestream_M = 0.;
		 double density = 0.;
		 double RE = 1000.;

		 double p = 0.;
		 double q = 0.;
		 double r = 0.;

		 double u = 0.;
		 double v = 0.;
		 double w = 0.;

		 inline auto calculate_RE(double freestream_M, double altitude, double characteristic_length) -> double
		 {
			 double speed = this->atm.getSpeedOfSound(altitude) * freestream_M;
			 double visc = this->atm.getViscosity(altitude);
			 double rho = this->atm.getDensity(altitude);
			 return rho * speed * characteristic_length / visc;
		 };
	};

	struct Drag
	{
		Drag(double CD_w, double CD_f, double CD_i) : CD_w(CD_w), CD_f(CD_f), CD_i(CD_i)
		{
			CD0 = CD_w + CD_f;
			CD = CD0 + CD_i;
		};

		Drag(double CD) : CD(CD)
		{};

		double CD0 = 0.;
		double CD_w = 0.;
		double CD_f = 0.;
		double CD_i = 0.;
		double CD = 0.;

		void update(double CD_w, double CD_f)
		{
			this->CD_w = CD_w;
			this->CD_f = CD_f;
			this->CD0 = CD_w + CD_f;
			this->CD = this->CD0 + this->CD_i;
		}
	};

	struct Polar
	{
		Polar(const node& polar_node, const int& polar_xml_version)
		{
			switch (polar_xml_version)
			{
			case XML_Version::V1:
				PolarV1(polar_node);
				break;
			case XML_Version::V2:
				PolarV2(polar_node);
				break;
			}
		}

		Polar() = default;

		Flight_Condition conditions = Flight_Condition(0., 0.);

		types::VectorXd alpha = {};
		types::VectorXd beta = {};

		types::VectorXd CL = {};
		types::VectorXd CS = {};
		std::vector<Drag> CD = {};

		types::VectorXd CM_x = {};
		types::VectorXd CM_y = {};
		types::VectorXd CM_z = {};

		static auto XML_parser(const node& polar_node, const std::string& what) -> types::VectorXd
		{
			std::string what_node = what;
			auto value_node = polar_node.find(what + "/value");
			if (value_node != nullptr)
			{
				what_node += "/value";
			};

			std::string xml_content = polar_node.at(what_node);
			std::stringstream xml_string(xml_content);
			std::string temp;
			std::vector<double> temp_output;

			while (std::getline(xml_string, temp, ';')) {
				try {
					temp_output.push_back(std::stod(temp));
				}
				catch (...)
				{
					std::cout<< "parser error occured - check polar xml \n";
				};
			}
			Eigen::Map<const types::VectorXd> output(temp_output.data(), temp_output.size());
			return output;
		}

		void XML_writer(node* target, const int& polar_xml_version)
		{
			std::string line;
			std::string name;

			line = std::to_string(this->conditions.freestream_M);
			name = "machnumber";
			write_node(target, polar_xml_version, name, line);

			line = std::to_string(this->conditions.altitude);
			name = "altitude";
			write_node(target, polar_xml_version, name, line);

			auto join_values = [](auto const& container, auto getter) {
				std::ostringstream oss;
				for (auto const& elem : container) {
					oss << getter(elem) << ';';
				}
				std::string result = oss.str();
				if (!result.empty()) result.pop_back(); // remove trailing ';'
				return result;
				};

			name = "Cd";
			line = join_values(this->CD, [](auto const& d) { return d.CD; });
			write_node(target, polar_xml_version, name, line);

			name = "Cd0";
			line = join_values(this->CD, [](auto const& d) { return d.CD0; });
			write_node(target, polar_xml_version, name, line);

			name = "Cd_i";
			line = join_values(this->CD, [](auto const& d) { return d.CD_i; });
			write_node(target, polar_xml_version, name, line);

			name = "Cd_f";
			line = join_values(this->CD, [](auto const& d) { return d.CD_f; });
			write_node(target, polar_xml_version, name, line);

			name = "Cd_w";
			line = join_values(this->CD, [](auto const& d) { return d.CD_w; });
			write_node(target, polar_xml_version, name, line);

			name = "Cl";
			line = join_values(this->CL, [](auto const& val) { return val; });
			write_node(target, polar_xml_version, name, line);

			name = "Cm";
			line = join_values(this->CM_y, [](auto const& val) { return val; });
			write_node(target, polar_xml_version, name, line);

			name = "Alpha";
			line = join_values(this->alpha, [](auto const& val) { return val; });
			write_node(target, polar_xml_version, name, line);
		}

	private:
		void PolarV1(const node& polar_node)
		{
			auto value_node = polar_node.find("machnumber/value");
			if (value_node != nullptr)
			{
				this->conditions = Flight_Condition(polar_node.at("altitude/value"), polar_node.at("machnumber/value"));
			}
			else
			{
				this->conditions = Flight_Condition(polar_node.at("altitude"), polar_node.at("machnumber"));
			}
			this->CL = XML_parser(polar_node, "Cl");
			this->alpha = XML_parser(polar_node, "Alpha");
			this->CM_y = XML_parser(polar_node, "Cm");

			auto CD_total_vector = XML_parser(polar_node, "Cd");

			for (auto CD_total : CD_total_vector)
			{
				this->CD.emplace_back(Drag(CD_total));
			}
		}

		void PolarV2(const node& polar_node)
		{
			auto value_node = polar_node.find("machnumber/value");
			if (value_node != nullptr)
			{
				this->conditions = Flight_Condition(polar_node.at("altitude/value"), polar_node.at("machnumber/value"));
			}
			else
			{
				this->conditions = Flight_Condition(polar_node.at("altitude"), polar_node.at("machnumber"));
			}
			this->CL = XML_parser(polar_node, "Cl");
			this->alpha = XML_parser(polar_node, "Alpha");
			this->CM_y = XML_parser(polar_node, "Cm");

			auto CD_0_vec = XML_parser(polar_node, "Cd0");
			auto CD_w_vec = XML_parser(polar_node, "Cd_w");
			auto CD_f_vec = XML_parser(polar_node, "Cd_f");
			auto CD_i_vec = XML_parser(polar_node, "Cd_i");
			for (size_t i = 0; i < CD_i_vec.size(); ++i)
			{
				this->CD.emplace_back(Drag(
					CD_w_vec[i],
					CD_f_vec[i],
					CD_i_vec[i])
				);
			}
		}

		void write_node(node* target, const int& polar_xml_version, std::string& name, std::string& val)
		{
			std::string dummy = "-";
			auto tmp_node = Endnode<std::string>{name, dummy};
			tmp_node = val;
			tmp_node.update(*target);
		}
	};

	void create_polar_xml(std::filesystem::path filename, const int& polar_xml_version);

	struct Control_Surface
	{
	public:
		Control_Surface(const std::string& surface_ID, const double& deflection_angle) : surface_ID(surface_ID), deflection_angle(deflection_angle)
		{}

		std::string surface_ID = "0";
		double deflection_angle = 0.;
	};

	class Configuration
	{
	public:
		Configuration(const node& configuration_node, const int& polar_xml_version)
		{
			auto XML_polars = configuration_node.getVector("polars/polar");

			std::transform(XML_polars.begin(), XML_polars.end(), std::back_inserter(polars),
				[&polar_xml_version](auto& XML_polar) {
					return Polar((*XML_polar), polar_xml_version);
				});

			configuration_ID = configuration_node.getStringAttrib("ID");
		}

		std::vector<Control_Surface> surface_config = {};  // control surface deflections must be defined somewhere //
		std::vector<Polar> polars = {};
		std::string configuration_ID;
	};

	void evaluate_CL(types::PropertyType& container_ptr, const Polar& polar);

	void evaluate_CM(types::PropertyType& container_ptr, const Polar& polar);

	void evaluate_CD(types::PropertyType& container_ptr, const Polar& polar);

	class Component
	{
	public:
		Component() = default;

		Component(const node& component_node, const int& polar_xml_version, std::string method)
		{
			auto XML_configurations = component_node.getVector("configurations/configuration");

			std::transform(XML_configurations.begin(), XML_configurations.end(), std::back_inserter(this->configurations),
				[&polar_xml_version](auto& XML_configuration) {
					return Configuration(*XML_configuration, polar_xml_version);
				});

			this->component_ID = component_node.getStringAttrib("ID");

			auto area_v = Polar::XML_parser(component_node, "area");
			this->area = area_v(0);

			auto chord_node = component_node.find("chord");

			if (chord_node != nullptr)
			{
				auto chord_v = Polar::XML_parser(component_node, "chord");
				this->chord = chord_v(0);
			}
			else
			{
				auto chord_v = Polar::XML_parser(component_node, "length");
				this->chord = chord_v(0);
			}

			auto ac_node = component_node.find("reference_point");

			if (ac_node != nullptr)
			{
				this->reference_point = Polar::XML_parser(component_node, "reference_point");
			}
			else
			{
				this->reference_point = { this->chord / 4., 0., 0. };
			}

			if (method == "linearized")
			{
				linearized_matrix_system matrices(*this);
				this->interpolator = std::make_unique<linear::interpolation>(
					matrices.lhs_container[configurations[0].configuration_ID],
					matrices.rhs_container[configurations[0].configuration_ID],
					matrices.variables
				);
			}
			else
			{
				non_linearized_matrix_system matrices(*this);
				this->interpolator = std::make_unique<linear::interpolation>(
					matrices.lhs_container[configurations[0].configuration_ID],
					matrices.rhs_container[configurations[0].configuration_ID],
					matrices.variables
				);
			}
		}

		struct linearized_matrix_system
		{
			linearized_matrix_system(const Component& component)
			{
				std::ranges::for_each(
					component.configurations,
					[this](auto& configuration)
					{
						auto dim = configuration.polars.size();

						types::VectorXd Mach(dim);
						types::VectorXd altitude(dim);
						std::vector<types::PropertyType> properties = {};

						int i = 0;

						std::ranges::for_each(
							configuration.polars,
							[this, &Mach, &altitude, &i, &properties](auto& polar)
							{
								Mach(i) = polar.conditions.freestream_M;
								altitude(i) = polar.conditions.altitude;
								types::PropertyType polar_processing;
								polar_processing["alpha"] = polar.alpha;
								evaluate_CL(polar_processing, polar);
								evaluate_CM(polar_processing, polar);
								evaluate_CD(polar_processing, polar);
								properties.push_back(polar_processing);
								i++;
							});

						types::PropertyType lhs;

						this->variables.push_back("Mach");
						lhs["Mach"] = Mach;

						this->variables.push_back("h");
						lhs["h"] = altitude;
						
						this->lhs_container[configuration.configuration_ID] = lhs;
						this->rhs_container[configuration.configuration_ID] = properties;
					});
			}

			std::unordered_map<types::key, types::PropertyType> lhs_container;
			std::unordered_map<types::key, std::vector<types::PropertyType>> rhs_container;
			std::vector<types::key> variables = {};
		};

		struct non_linearized_matrix_system
		{
			non_linearized_matrix_system(const Component& component)
			{
				std::ranges::for_each(
					component.configurations,
					[this](auto& configuration)
					{
						std::vector<double> Mach = {};
						std::vector<double> altitude = {};
						std::vector<double> alpha = {};

						std::vector<types::PropertyType> properties = {};

						std::ranges::for_each(
							configuration.polars,
							[this, &Mach, &altitude, &alpha, &properties](auto& polar)
							{
								int i = 0;
								for (auto aoa : polar.alpha)
								{
									alpha.push_back(aoa);
									Mach.push_back(polar.conditions.freestream_M);
									altitude.push_back(polar.conditions.altitude);
									types::PropertyType polar_processing;
									polar_processing["CL"] = polar.CL(i);
									polar_processing["CM"] = polar.CM_y(i);
									polar_processing["CD"] = polar.CD[i].CD;

									properties.push_back(polar_processing);
									i++;
								}
							});

						types::PropertyType lhs;
						this->variables.push_back("Mach");
						lhs["Mach"] = vec_to_eigen(Mach);

						this->variables.push_back("h");
						lhs["h"] = vec_to_eigen(altitude);

						this->variables.push_back("alpha");
						lhs["alpha"] = vec_to_eigen(alpha);

						this->lhs_container[configuration.configuration_ID] = lhs;
						this->rhs_container[configuration.configuration_ID] = properties;

					});
			}

			std::unordered_map<types::key, types::PropertyType> lhs_container;
			std::unordered_map<types::key, std::vector<types::PropertyType>> rhs_container;
			std::vector<types::key> variables = {};
		};

		std::string component_ID = "0";
		std::vector<Configuration> configurations = {};

		std::shared_ptr<linear::interpolation> interpolator = std::make_unique<linear::interpolation>();
		Eigen::Matrix<double, 3, 1> reference_point = { 0., 0., 0. };
		double area = 100.;
		double chord = 1.;
		double incidence = 0.;
		std::vector<types::key> variables = {};

		void change_configuration(types::key& new_configuration);

		auto get_property(const types::key& for_property, types::PropertyType& at_conditions) -> double;
	};

	class Aircraft
	{
	public:

		struct trim_settings
		{
			std::string method = "linearized";
			std::string reference_wing_ID = "0";
			std::string adjustable_surface_ID = "0";
			double reference_wing_angle = 0.;
			double adjustable_surface_angle = 0.;
		};

		Aircraft(const std::shared_ptr<node>& polarXML, trim_settings settings) : polarXML(polarXML), settings(settings)
		{
			auto polar_xml_node = polarXML->at("polar-exchange-file");

			auto polar_xml_version = static_cast<int>(std::stod(polar_xml_node.getStringAttrib("version")));

			auto XML_components = polar_xml_node.getVector("components/component");

			for (const auto& XML_component : XML_components) {
				Component component(*XML_component, polar_xml_version, settings.method);
				this->components[component.component_ID] = component;
			}
		}

		Aircraft(const std::string path_polarXML, trim_settings settings) : settings(settings)
		{
			std::filesystem::path file = path_polarXML;

			this->polarXML = aixml::openDocument(file);

			auto polar_xml_node = polarXML->at("polar-exchange-file");

			auto polar_xml_version = static_cast<int>(std::stod(polar_xml_node.getStringAttrib("version")));

			auto XML_components = polar_xml_node.getVector("components/component");

			for (const auto& XML_component : XML_components) {
				Component component(*XML_component, polar_xml_version, settings.method);
				this->components[component.component_ID] = component;
			}
		}

		void change_settings(std::string reference_wing_ID, std::string adjustable_surface_ID, std::string method);

		void linearized_trim(const Flight_Condition& conditions, const double& weight, const std::vector<double>& cog_position);

		auto get_CL(const Flight_Condition& conditions, const double& weight) -> double;

		auto get_CD(const Flight_Condition& conditions, const double& weight) -> double;

		auto get_CL_CD(const Flight_Condition& conditions, const double& weight, const std::vector<double>& cog_position) -> double;

		std::shared_ptr<node> polarXML = {};
		std::unordered_map<std::string, Component> components;
		trim_settings settings;
	};

	class Propeller
	{
	public:

		Propeller(const std::string CSV_path, const double& diameter) : diameter(diameter)
		{
			std::ifstream file(CSV_path);
			std::vector<double> inclination_vec;
			std::vector<double> j_vec;
			std::vector<types::PropertyType> prop_vec;
			std::string line;

			while (std::getline(file, line)) {
				if (line.empty()) continue;
				std::vector<std::string> tokens;
				std::stringstream ss(line);
				std::string token;
				while (std::getline(ss, token, ','))
					tokens.push_back(token);
				if (tokens.size() < 5) continue;

				double inclination, j, ct, cp, eta;
				try {
					inclination = std::stod(tokens[0]);
					j = std::stod(tokens[1]);
					ct = std::stod(tokens[2]);
					cp = std::stod(tokens[3]);
					eta = std::stod(tokens[4]);
				}
				catch (const std::invalid_argument&) {
					continue;
				}

				inclination_vec.push_back(inclination);
				j_vec.push_back(j);

				types::PropertyType dummy;
				dummy["CT"] = ct;
				dummy["CP"] = cp;
				dummy["eta"] = eta;
				prop_vec.push_back(dummy);
			}
			types::PropertyType lhs;
			lhs["inclination"] = inclination_vec;
			lhs["j"] = j_vec;
			std::vector<types::key> variables = { "inclination", "j" };
			linear::interpolation engine(lhs, prop_vec, variables);
			this->interpolator = std::make_unique<linear::interpolation>(lhs, prop_vec, variables);
		}

		auto get_T(const Flight_Condition& condition, const double& RPM, const double& pitch) -> double;

		auto get_P(const Flight_Condition& condition, const double& RPM, const double& pitch) -> double;

		std::shared_ptr<linear::interpolation> interpolator = std::make_unique<linear::interpolation>();

		double diameter = 1.;
	};
}

#endif