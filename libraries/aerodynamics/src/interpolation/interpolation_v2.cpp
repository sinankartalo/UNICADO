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


#include "interpolation/interpolation_v2.h"
#include "solver/solver_v2.h"
#include "interpolation/data_types.h"
#include <CGAL/Epick_d.h>
#include <CGAL/Delaunay_triangulation.h>
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

namespace linear
{
	auto interpolation::triangulation(types::MatrixXRd& points, std::vector<types::PropertyType>& values) -> types::Delaunay
	{
		types::Delaunay triangulated_mat(points.cols());
		int corr = 0;
		for (int i = 0; i < points.rows(); i++) {
			types::Point_CGAL p_CGAL(points.row(i).data(), points.row(i).data() + points.cols());
			auto handle = triangulated_mat.insert(p_CGAL);
			handle->data() = values[i];
			if(i+1 != triangulated_mat.number_of_vertices()+corr)
			{
				std::cout << "duplicate found " << points.row(i)(0) << " " << points.row(i)(1) << " " << points.row(i)(2) << " " << "\n";
				corr++;
			}
		}
		
		return triangulated_mat;
	}

	auto interpolation::calculate_barycentric_coordinates(const std::vector<types::Point_CGAL>& vertex, const types::Point_CGAL& point) -> types::VectorXd
	{
		auto dimension = point.size();

		types::MatrixXRd A = Eigen::ArrayXXd::Ones(dimension + 1, dimension + 1);

		types::VectorXd b(dimension + 1);
		b(0) = 1.0;

		for (int i = 0; i < dimension; i++)
		{
			b(i+1) = point[i];
		}

		for (int i = 0; i < dimension; i++)
		{
			for (int j = 0; j < dimension + 1; j++)
			{
				A(i+1,j) = vertex[j][i];
			}
		}

		system sys(A, b);

		auto coordinates = sys();

		return coordinates;
	};

	void interpolation::update()
	{
		this->vertices = {};
		this->vertex_properties = {};

		std::vector<double> conditions_vec(this->variables.size());
		int i = 0;
		for (const auto& x_i : variables)
		{
			auto iterator = this->conditions.find(x_i);

			// if (iterator == this->conditions.end()) { i++; continue; };

			conditions_vec[i] = types::get_scalar(iterator->second);

			i++;
		}
		
		types::Point_CGAL point(conditions_vec);

		auto simplex_it = this->Delaunay_triangulation.locate(point);
		auto& simplex = *simplex_it;

		for (int i = 0; i <= point.size(); i++)
		{
			this->vertices.push_back(simplex.vertex(i)->point());
			this->vertex_properties.push_back(simplex.vertex(i)->data());
		}
		this->barycentric_coordinates = calculate_barycentric_coordinates(this->vertices, point);
	}

	auto interpolation::operator()(types::key for_property, types::PropertyType& at_conditions) -> double
	{
		if (this->conditions.size() == 0)
		{
			this->conditions = at_conditions;
			update();
		}
		else
		{
			bool needs_update = false;
			for (const auto& [k, v] : at_conditions)
			{
				auto it = this->conditions.find(k);
				if (it == this->conditions.end() || it->second != v)
				{
					needs_update = true;
					break;
				}
			}
			if (needs_update)
			{
				this->conditions = at_conditions;
				update();
			}
		}

		double out = 0.;

		for (int i = 0; i < this->barycentric_coordinates.size(); i++)
		{
			out += this->barycentric_coordinates(i) * types::get_scalar(this->vertex_properties[i].at(for_property));
		}

		return out;
	}
}