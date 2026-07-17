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

#ifndef AERODYNAMICS_INTERPOLATOR_H_
#define AERODYNAMICS_INTERPOLATOR_H_

#include <string>
#include <cstdint>
#include <vector>
#include <variant>
#include <CGAL/Epick_d.h>
#include <CGAL/Delaunay_triangulation.h>
#include <CGAL/Triangulation_vertex.h>

using key = std::variant<int, std::string, double>;
using value = std::variant<double, std::vector<double>>;

struct properties
{
	double CL_alpha;
	double CL_0;
	double CM_alpha;
	double CM_0;
	double CD_0;
	double CD_PB;
	double K1;
	double K2;
	double K3;
	double K4;
	double alpha_PB;
	double value = 0.;

	properties() = default;

	properties(double CL_alpha, double CL_0, double CM_alpha, double CM_0, double CD_0, double CD_PB, double K1, double K2, double K3, double K4, double alpha_PB) :
		CL_alpha(CL_alpha), CL_0(CL_0), CM_alpha(CM_alpha), CM_0(CM_0), CD_0(CD_0), CD_PB(CD_PB), K1(K1), K2(K2), K3(K3), K4(K4), alpha_PB(alpha_PB)
	{};

	double& at(const std::string& name)
	{
		if (name == "CL_alpha") return CL_alpha;
		else if (name == "CL_0") return CL_0;
		else if (name == "CM_alpha") return CM_alpha;
		else if (name == "CM_0") return CM_0;
		else if (name == "CD_0") return CD_0;
		else if (name == "CD_PB") return CD_PB;
		else if (name == "K1") return K1;
		else if (name == "K2") return K2;
		else if (name == "K3") return K3;
		else if (name == "K4") return K4;
		else if (name == "alpha_PB") return alpha_PB;
		else if (name == "value") return value;
		else throw std::runtime_error("Unknown property name");
	}
};

using Kernel = CGAL::Epick_d<CGAL::Dynamic_dimension_tag>;
using Point_CGAL = Kernel::Point;

using PropertyType = properties;

using Vb = CGAL::Triangulation_vertex<Kernel, PropertyType, CGAL::Default>;
using Cb = CGAL::Triangulation_full_cell<Kernel, CGAL::No_full_cell_data, CGAL::Default>;
using TDS = CGAL::Triangulation_data_structure<CGAL::Dynamic_dimension_tag, Vb, Cb >;

using Delaunay = CGAL::Delaunay_triangulation<Kernel, TDS>;

inline auto get_scalar(const value& val) -> double
{
	if (auto double_ptr = std::get_if<double>(&val))
	{
		return *double_ptr;
	};

	auto vector_ptr = std::get_if<std::vector<double>>(&val);

	return (*vector_ptr)[0];
}

inline auto get_vector(const value& val) -> std::vector<double>
{
	if (auto double_ptr = std::get_if<double>(&val))
	{
		return { *double_ptr };
	};

	auto vector_ptr = std::get_if<std::vector<double>>(&val);

	return *vector_ptr;
}

struct f_hashmap {
	std::size_t operator() (const key& subkey) const {
		if (std::holds_alternative<int>(subkey))
		{
			return std::hash<int>{}(std::get<int>(subkey));
		}
		else if (std::holds_alternative<std::string>(subkey))
		{
			return std::hash<std::string>{}(std::get<std::string>(subkey));
		}
		else if (std::holds_alternative<double>(subkey))
		{
			return std::hash<double>{}(std::get<double>(subkey));
		}
		return 0;
	}
};

inline long long quantize(double d, double precision = 1e-6) {
	return static_cast<long long>(std::llround(d / precision));
}

struct double_equal {
	bool operator()(double a, double b) const {
		return quantize(a) == quantize(b);
	}
};

struct double_hash {
	std::size_t operator()(double d) const {
		return std::hash<long long>{}(quantize(d));
	}
};

using map1 = std::unordered_map<std::string, std::unordered_map<key, value, f_hashmap>>;
using map2 = std::unordered_map<std::string,
	std::unordered_map<double,
	std::unordered_map<double,
	std::unordered_map<std::string, value>,double_hash, double_equal>,double_hash, double_equal>>;

namespace linear
{
	struct interpolation
	{
		interpolation() : Delaunay_triangulation([&]
			{
				return Delaunay(1);
			}())
		{};

		interpolation(map1& left_hand_side, map2& right_hand_side, std::string configuration) :
			Delaunay_triangulation([&] {
			std::vector<std::vector<double>> lhs_matrix;
			std::vector<properties> rhs_properties;
			auto M_vec = get_vector(left_hand_side[configuration]["M"]);
			auto h_vec = get_vector(left_hand_side[configuration]["h"]);

			for (std::size_t i = 0; i < M_vec.size(); ++i) {
				lhs_matrix.push_back({ M_vec[i], h_vec[i] });

				auto& rhs_ptr = right_hand_side[configuration][M_vec[i]][h_vec[i]];
				
				rhs_properties.push_back(properties(
					get_scalar(rhs_ptr["CL_alpha"]),
					get_scalar(rhs_ptr["CL_0"]),
					get_scalar(rhs_ptr["CM_alpha"]),
					get_scalar(rhs_ptr["CM_0"]),
					get_scalar(rhs_ptr["CD_0"]),
					get_scalar(rhs_ptr["CD_PB"]),
					get_scalar(rhs_ptr["K1"]),
					get_scalar(rhs_ptr["K2"]),
					get_scalar(rhs_ptr["K3"]),
					get_scalar(rhs_ptr["K4"]),
					get_scalar(rhs_ptr["alpha_PB"]) ));
			}
			return triangulation(lhs_matrix, rhs_properties);
				}())
		{};

		static auto triangulation(std::vector<std::vector<double>>& points, std::vector<properties>& values) -> Delaunay;

		static auto calculate_barycentric_coordinates(const std::vector<Point_CGAL>& vertices, const Point_CGAL& point) -> std::vector<double>;

		void update();

		auto operator()(std::string for_property, std::vector<double>& at_conditions) -> double;

		Delaunay Delaunay_triangulation;
		std::vector<double> conditions = {};
		std::vector<Point_CGAL> vertices = {};
		std::vector<properties> vertex_properties = {};
		std::vector<double> barycentric_coordinates = {};
	};
}

#endif