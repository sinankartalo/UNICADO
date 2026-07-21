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

#include "aerodynamics/aerodynamics_v3.h"
#include "interpolation/interpolation_v2.h"
#include "solver/solver_v2.h"
#include <Eigen/Dense>
#include <aixml/endnode.h>
#include <aixml/node.h>
#include <atmosphere/atmosphere.h>
#include <runtimeInfo/runtimeInfo.h>
#include <standardFiles/functions.h>
#include <algorithm>
#include <cmath>
#include <climits>
#include <unordered_map>
#include <variant>

namespace aerodynamics
{
	auto vec_to_eigen(std::vector<double> vec) -> types::VectorXd
	{
		Eigen::Map<const types::VectorXd> map_vec(vec.data(), vec.size());
		return map_vec;
	}

	void create_polar_xml(std::filesystem::path filename, const int& polar_xml_version)
	{
		std::fstream polar_file;
		polar_file.open(filename, std::ios::out);
		polar_file << "<?xml version=\"1.0\"?>\n";
		polar_file << "<polar-exchange-file version=\"" << polar_xml_version << ".0\">\n";
		polar_file << "</polar-exchange-file>\n";

		polar_file.flush();
		polar_file.close();
	}

	void evaluate_CL(types::PropertyType& container_ptr, const Polar& polar)
	{
		container_ptr["CL_alpha"] = linear::get_slope(polar.CL, polar.alpha);
		container_ptr["CL_0"] = linear::get_intercept(polar.CL, polar.alpha, types::get_vector(container_ptr["CL_alpha"]));
	};

	void evaluate_CM(types::PropertyType& container_ptr, const Polar& polar)
	{
		container_ptr["CM_alpha"] = linear::get_slope(polar.CM_y, polar.alpha);
		container_ptr["CM_0"] = linear::get_intercept(polar.CM_y, polar.alpha, types::get_vector(container_ptr["CM_alpha"]));
	};

	void evaluate_CD(types::PropertyType& container_ptr, const Polar& polar)
	{
		std::vector<double> CD_vec_dummy = {};
		for (auto CD_obj : polar.CD)
		{
			CD_vec_dummy.push_back(CD_obj.CD);
		}

		auto CD_vec = vec_to_eigen(CD_vec_dummy);

		container_ptr["CD_0"] = *std::min_element(CD_vec.begin(), CD_vec.end());
		container_ptr["CD_PB"] = linear::get_break(CD_vec, polar.alpha, 10000.);

		if (types::get_scalar(container_ptr["CD_PB"]) < -999.)
		{
			container_ptr["CD_PB"] = CD_vec.tail(1);

			auto k_factors = linear::get_quadratic_coefficients(CD_vec, polar.CL);

			container_ptr["K2"] = k_factors[0];
			container_ptr["K1"] = k_factors[1];

			container_ptr["K3"] = 0.;
			container_ptr["K4"] = 0.;

			container_ptr["alpha_PB"] = 999.;
		}
		else
		{
			auto CD_0_it = std::find(CD_vec.begin(), CD_vec.end(), types::get_scalar(container_ptr["CD_0"]));
			auto CD_PB_it = std::find(CD_vec.begin(), CD_vec.end(), types::get_scalar(container_ptr["CD_PB"]));

			auto CD_0_idx = std::distance(CD_vec.begin(), CD_0_it);
			auto CD_PB_idx = std::distance(CD_vec.begin(), CD_PB_it);

			std::vector<double> CD_pre_PB(CD_vec.begin() + CD_0_idx, CD_vec.begin() + CD_PB_idx);
			std::vector<double> alpha_pre_PB(polar.alpha.begin() + CD_0_idx, polar.alpha.begin() + CD_PB_idx);

			std::vector<double> CD_post_PB(CD_vec.begin() + CD_PB_idx, CD_vec.end());
			std::vector<double> alpha_post_PB(polar.alpha.begin() + CD_PB_idx, polar.alpha.end());

			auto k_factors_pre_PB = linear::get_quadratic_coefficients(vec_to_eigen(CD_pre_PB), vec_to_eigen(alpha_pre_PB));

			container_ptr["K1"] = k_factors_pre_PB[0];
			container_ptr["K2"] = k_factors_pre_PB[1];

			auto k_factors_post_PB = linear::get_quadratic_coefficients(vec_to_eigen(CD_post_PB), vec_to_eigen(alpha_post_PB));

			container_ptr["K3"] = k_factors_post_PB[0];
			container_ptr["K4"] = k_factors_post_PB[1];

			container_ptr["alpha_PB"] = polar.alpha(CD_PB_idx);
		}

	}

	void Component::change_configuration(types::key& new_configuration)
	{
		linearized_matrix_system matrices(*this);

		this->interpolator = std::make_unique<linear::interpolation>(
			matrices.lhs_container[new_configuration],
			matrices.rhs_container[new_configuration],
			matrices.variables);
	}

	auto Component::get_property(const types::key& for_property, types::PropertyType& at_conditions) -> double
	{
		return this->interpolator->operator()(for_property, at_conditions);
	}

	void Aircraft::change_settings(std::string reference_wing_ID, std::string adjustable_surface_ID, std::string method)
	{
		this->settings.method = method;
		this->settings.adjustable_surface_ID = adjustable_surface_ID;
		this->settings.reference_wing_ID = reference_wing_ID;
	}

	auto Aircraft::get_CL(const Flight_Condition& conditions, const double& weight) -> double
	{
		return weight / (0.5 * conditions.density * pow(conditions.u, 2.) * this->components[this->settings.reference_wing_ID].area);
	};

	void Aircraft::linearized_trim(const Flight_Condition& conditions, const double& weight, const std::vector<double>& cog_position)
	{
		types::PropertyType v_conditions;
		v_conditions["Mach"] = conditions.freestream_M;
		v_conditions["h"] = conditions.altitude;

		types::MatrixXRd A = Eigen::ArrayXXd::Zero(2, 2);

		types::VectorXd b(2);
		b(0) = 0.;
		b(1) = 0.;

		auto main_wing = this->components[this->settings.reference_wing_ID];
		auto trim_surface = this->components[this->settings.adjustable_surface_ID];

		b(0) += get_CL(conditions, weight);

		for (auto& [name, component] : this->components)
		{
			auto CL_alpha = component.get_property("CL_alpha", v_conditions);
			auto CM_alpha = component.get_property("CL_alpha", v_conditions);
			auto CL_0 = component.get_property("CL_0", v_conditions);
			auto CM_0 = component.get_property("CM_0", v_conditions);

			A(0, 0) += CL_alpha * component.area / main_wing.area;
			b(0) += -CL_0 * component.area / main_wing.area - CL_alpha * (-main_wing.incidence) * component.area / main_wing.area;
			A(1, 0) += CM_alpha + CL_alpha * (cog_position[0] / main_wing.chord - component.reference_point[0] / main_wing.chord) * component.area / main_wing.area;
			b(1) += -(CM_0 + CL_0 * (cog_position[0] / main_wing.chord - component.reference_point[0] / main_wing.chord)) * component.area / main_wing.area;
			b(1) += -(CM_alpha +
				CL_alpha * (cog_position[0] / main_wing.chord - component.reference_point[0] / main_wing.chord)) * (-main_wing.incidence) * component.area / main_wing.area;

			if (component.component_ID != this->settings.adjustable_surface_ID)
			{
				b(0) += -CL_alpha * component.incidence * component.area / main_wing.area;
				b(0) += -(CM_alpha +
					CL_alpha * (cog_position[0] / main_wing.chord - component.reference_point[0] / main_wing.chord)) * component.incidence * component.area / main_wing.area;
			}
		}

		A(0, 1) = trim_surface.get_property("CL_alpha", v_conditions) * trim_surface.area / main_wing.area;
		A(1, 1) = (trim_surface.get_property("CM_alpha", v_conditions) +
			trim_surface.get_property("CL_alpha", v_conditions) *
			(cog_position[0] / main_wing.chord - trim_surface.reference_point[0] / main_wing.chord)) * trim_surface.area / main_wing.area;

		linear::system sys(A, b);

		auto angles = sys();

		this->settings.reference_wing_angle = angles(0);
		this->settings.adjustable_surface_angle = angles(1);
	}

	auto Aircraft::get_CD(const Flight_Condition& conditions, const double& weight) -> double
	{
		types::PropertyType v_conditions;
		v_conditions["Mach"] = conditions.freestream_M;
		v_conditions["h"] = conditions.altitude;

		double CD = 0.;
		double CL = 0.;
		auto main_wing = this->components[this->settings.reference_wing_ID];

		for (const auto& [key, temp] : this->components) {
			if (key == this->settings.reference_wing_ID)
			{
				CL = this->components[key].get_property("CL_0", v_conditions) +
					this->components[key].get_property("CL_alpha", v_conditions) * this->settings.reference_wing_angle;

				if (this->settings.reference_wing_angle < this->components[key].get_property("alpha_PB", v_conditions))
				{
					CD += this->components[key].get_property("CD_0", v_conditions) +
						this->components[key].get_property("K1", v_conditions) * CL +
						this->components[key].get_property("K2", v_conditions) * pow(CL, 2.);
				}
				else
				{
					CD += this->components[key].get_property("CD_PB", v_conditions) +
						this->components[key].get_property("K3", v_conditions) * CL +
						this->components[key].get_property("K4", v_conditions) * pow(CL, 2.);
				}
			}
			else if (key == this->settings.adjustable_surface_ID)
			{
				CL = this->components[key].get_property("CL_0", v_conditions) +
					this->components[key].get_property("CL_alpha", v_conditions) *
					(this->settings.reference_wing_angle - main_wing.incidence + this->settings.adjustable_surface_angle);

				if ((this->settings.reference_wing_angle - main_wing.incidence + this->settings.adjustable_surface_angle) < this->components[key].get_property("alpha_PB", v_conditions))
				{
					CD += (this->components[key].get_property("CD_0", v_conditions) +
						this->components[key].get_property("K1", v_conditions) * CL +
						this->components[key].get_property("K2", v_conditions) * pow(CL, 2.)) * this->components[key].area / main_wing.area;
				}
				else
				{
					CD += (this->components[key].get_property("CD_PB", v_conditions) +
						this->components[key].get_property("K3", v_conditions) * CL +
						this->components[key].get_property("K4", v_conditions) * pow(CL, 2.)) * this->components[key].area / main_wing.area;
				}
			}
			else
			{
				CL = this->components[key].get_property("CL_0", v_conditions) +
					this->components[key].get_property("CL_alpha", v_conditions) *
					(this->settings.reference_wing_angle - main_wing.incidence + this->components[key].incidence);

				if ((this->settings.reference_wing_angle - main_wing.incidence + this->components[key].incidence) < this->components[key].get_property("alpha_PB", v_conditions))
				{
					CD += (this->components[key].get_property("CD_0", v_conditions) +
						this->components[key].get_property("K1", v_conditions) * CL +
						this->components[key].get_property("K2", v_conditions) * pow(CL, 2.)) * this->components[key].area / main_wing.area;
				}
				else
				{
					CD += (this->components[key].get_property("CD_PB", v_conditions) +
						this->components[key].get_property("K3", v_conditions) * CL +
						this->components[key].get_property("K4", v_conditions) * pow(CL, 2.)) * this->components[key].area / main_wing.area;
				}
			}
		}

		return CD;
	};

	auto Aircraft::get_CL_CD(const Flight_Condition& conditions, const double& weight, const std::vector<double>& cog_position) -> double
	{
		this->linearized_trim(conditions, weight, cog_position);
		return this->get_CL(conditions, weight)/this->get_CD(conditions, weight);
	};

	auto Propeller::get_T(
		const Flight_Condition& condition,
		const double& RPM,
		const double& pitch
	) -> double
	{
		const double rho = condition.density;
		const double v = condition.u;
		const double n = RPM / 60.0;
		const double J = v / (n * this->diameter);

		std::cout << "J = " << J << "\n";

		types::PropertyType conditions;
		conditions["inclination"] = pitch;
		conditions["j"] = J;

		const types::key property = "CT";
		const double Ct =
			this->interpolator->operator()(property, conditions);

		const double thrust =
			Ct
			* rho
			* std::pow(n, 2.0)
			* std::pow(this->diameter, 4.0);

		std::cout << "CT = " << Ct
				<< " T = " << thrust
				<< "\n";

		return thrust;
	};

	auto Propeller::get_P(
		const Flight_Condition& condition,
		const double& RPM,
		const double& pitch
	) -> double
	{
		const double rho = condition.density;
		const double v = condition.u;
		const double n = RPM / 60.0;
		const double J = v / (n * this->diameter);

		std::cout << "J = " << J << "\n";

		types::PropertyType conditions;
		conditions["inclination"] = pitch;
		conditions["j"] = J;

		const types::key property = "CP";
		const double Cp =
			this->interpolator->operator()(property, conditions);

		const double power =
			Cp
			* rho
			* std::pow(n, 3.0)
			* std::pow(this->diameter, 5.0);

		std::cout << "CP = " << Cp
				<< " P = " << power
				<< "\n";

		return power;
	}
}