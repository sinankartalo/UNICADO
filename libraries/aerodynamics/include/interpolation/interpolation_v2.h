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

#ifndef AERODYNAMICS_INTERPOLATOR_V2_H_
#define AERODYNAMICS_INTERPOLATOR_V2_H_

#include <string>
#include <cstdint>
#include <vector>
#include <variant>
#include <CGAL/Epick_d.h>
#include <CGAL/Delaunay_triangulation.h>
#include <CGAL/Triangulation_vertex.h>
#include <Eigen/Dense>
#include "data_types.h"

namespace linear
{
	struct interpolation
	{
		interpolation() : Delaunay_triangulation([&]
			{
				return types::Delaunay(1);
			}())
		{};

		interpolation(types::PropertyType& LHS_container, std::vector<types::PropertyType>& RHS_container, std::vector<types::key> variables) :
			Delaunay_triangulation([&] {
			types::MatrixXRd lhs_matrix;

			lhs_matrix.conservativeResize(RHS_container.size(), variables.size());

			int i = 0;

			for (const auto& x_i : variables)
			{
				auto iterator = LHS_container.find(x_i);

				// if (iterator == props.end()) { i++; continue; };

				auto vec = types::get_vector(iterator->second);

				lhs_matrix.col(i) = vec; 
				i++;
			}

			return triangulation(lhs_matrix, RHS_container);
				}()), variables(variables)
		{};

		static auto triangulation(types::MatrixXRd& points, std::vector<types::PropertyType>& values) -> types::Delaunay;

		static auto calculate_barycentric_coordinates(const std::vector<types::Point_CGAL>& vertex, const types::Point_CGAL& point) -> types::VectorXd;

		void update();

		auto operator()(types::key for_property, types::PropertyType& at_conditions) -> double;

		types::Delaunay Delaunay_triangulation;
		std::vector<types::key> variables;
		types::PropertyType conditions;
		std::vector<types::Point_CGAL> vertices = {};
		std::vector<types::PropertyType> vertex_properties = {};
		types::VectorXd barycentric_coordinates = {};
	};
}

#endif