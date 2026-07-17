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


#include "interpolation/interpolation.h"
#include "solver/solver.h"
#include <CGAL/Epick_d.h>
#include <CGAL/Delaunay_triangulation.h>
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
	auto interpolation::triangulation(std::vector<std::vector<double>>& points, std::vector<properties>& values) -> Delaunay
	{
		const int dimension = points[0].size();

		Delaunay triangulate(dimension);

		for (int i = 0; i < points.size(); i++) {
			Point_CGAL p_CGAL(points[i]);
			auto handle = triangulate.insert(p_CGAL);
			handle->data() = values[i];
		}

		return triangulate;
	}

	auto interpolation::calculate_barycentric_coordinates(const std::vector<Point_CGAL>& vertices, const Point_CGAL& point) -> std::vector<double>
	{
		auto dimension = point.size();

		std::vector<std::vector<double>> A(dimension + 1, std::vector<double>(dimension + 1));
		std::fill(A[0].begin(), A[0].end(), 1.0);

		std::vector<double> b(dimension + 1);
		b[0] = 1;
		for (int i = 0; i < dimension; i++)
		{
			b[i + 1] = point[i];
		}

		for (int i = 0; i < dimension; i++)
		{
			for (int j = 0; j < dimension + 1; j++)
			{
				A[i + 1][j] = vertices[j][i];
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

		Point_CGAL point(this->conditions);

		auto simplex_it = this->Delaunay_triangulation.locate(point);
		auto& simplex = *simplex_it;

		for (int i = 0; i <= point.size(); i++)
		{
			this->vertices.push_back(simplex.vertex(i)->point());
			this->vertex_properties.push_back(simplex.vertex(i)->data());
		}

		this->barycentric_coordinates = calculate_barycentric_coordinates(this->vertices, point);
	}

	auto interpolation::operator()(std::string for_property, std::vector<double>& at_conditions) -> double
	{
		if (this->conditions.size() == 0)
		{
			this->conditions = at_conditions;
			update();
		}
		else
		{
			for (int i = 0; i < at_conditions.size(); i++)
			{
				if (this->conditions[i] != at_conditions[i])
				{
					this->conditions = at_conditions;
					update();
				}
			}
		}

		double out = 0.;

		for (int i = 0; i < this->barycentric_coordinates.size(); i++)
		{
			out += this->barycentric_coordinates[i] * this->vertex_properties[i].at(for_property);
		}

		return out;
	}
}