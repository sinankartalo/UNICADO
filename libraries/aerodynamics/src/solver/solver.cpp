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
	auto get_slope(const std::vector<double>& y, const std::vector<double> x) -> value
	{
		std::vector<double> slopes = {};

		for (int i = 0; i < y.size() - 1; i++) {
			slopes.push_back((y[i + 1] - y[i]) / (x[i + 1] - x[i]));
		};

		return slopes;
	};

	auto get_intercept(const std::vector<double>& y, const std::vector<double> x, const std::vector<double> dx_dy) -> value
	{
		std::vector<double> intercepts = {};

		for (int i = 0; i < dx_dy.size(); i++) {
			intercepts.push_back(y[i] - dx_dy[i] * x[i]);
		};

		return intercepts;
	}

	auto get_quadratic_coefficients(const std::vector<double>& y, const std::vector<double>& x) -> std::vector<double>
	{
		std::vector<std::vector<double>> A;
		std::vector<double> b;
		const int n = static_cast<int>(x.size());

		double Sx = 0, Sx2 = 0, Sx3 = 0, Sx4 = 0;
		double Sy = 0, Sxy = 0, Sx2y = 0;

		for (int i = 0; i < n; ++i)
		{
			double xi = x[i];
			double yi = y[i];
			double xi2 = xi * xi;

			Sx += xi;
			Sx2 += xi2;
			Sx3 += xi2 * xi;
			Sx4 += xi2 * xi2;
			Sy += yi;
			Sxy += xi * yi;
			Sx2y += xi2 * yi;
		}

		A = {
			{Sx4, Sx3, Sx2},
			{Sx3, Sx2, Sx},
			{Sx2, Sx,  static_cast<double>(n)}
		};

		b = { Sx2y, Sxy, Sy };

		system sys(A, b);

		if (system::det(A) < 0.001)
		{
			return { 0., 0., 0. };
		}
		else
		{
			return sys();
		}
	}

	auto get_break(const std::vector<double>& y, const std::vector<double>& x, const double factor) -> value
	{
		for (int i = 1; i < y.size() - 2; i++) {
			if (std::abs((y[i + 1] - y[i]) / (x[i + 1] - x[i])) > std::abs(factor * (y[i] - y[i-1]) / (x[i] - x[i-1])) + 1.)
			{
				return y[i];
			}
		};
		return -9999.;
	}

	auto system::minor(std::vector<std::vector<double>> matrix, int row, int col) -> std::vector<std::vector<double>>
	{
		int n = matrix.size();
		std::vector<std::vector<double>> minor_mat(n - 1, std::vector<double>(n - 1));

		int row_hit = 0;
		int col_hit = 0;

		for (int i = 0; i < n; i++)
		{
			col_hit = 0;
			if (i != row)
			{
				for (int j = 0; j < n; j++)
				{
					if (j != col)
					{
						minor_mat[i - row_hit][j - col_hit] = matrix[i][j];
					}
					else { col_hit = 1; }
				}
			}
			else { row_hit = 1; }
		}
		return minor_mat;
	}

	auto system::det(std::vector<std::vector<double>>& matrix) -> double
	{
		int n = matrix[0].size();
		switch (n)
		{
		case 1:
			return matrix[0][0];
		case 2:
			return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
		default:
			double determinant = 0.;
			for (int idx = 0; idx < n; idx++)
			{
				double sign = (idx % 2 == 0) ? 1. : -1.;
				auto minor_mat = system::minor(matrix, 0, idx);

				determinant += sign * matrix[0][idx] * det(minor_mat);
			}
			return determinant;
		}
	}

	auto system::adj(std::vector<std::vector<double>>& matrix) -> std::vector<std::vector<double>>
	{
		int n = matrix.size();
		std::vector<std::vector<double>> adjugate(n, std::vector<double>(n));

		if (n == 1)
		{
			adjugate[0][0] = 1.;
			return adjugate;
		}

		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
			{
				double sign = ((i + j) % 2 == 0) ? 1.: -1.;
				auto minor_mat = minor(matrix, i, j);
				adjugate[j][i] = sign * det(minor_mat);
			}
		}

		return adjugate;
	}

	auto system::inverse(std::vector<std::vector<double>>& matrix) -> std::vector<std::vector<double>>
	{
		double determinant = det(matrix);
		auto adjoint = adj(matrix);

		int n = matrix.size();
		std::vector<std::vector<double>> inverse_mat(n, std::vector<double>(n));

		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
			{
				inverse_mat[i][j] = adjoint[i][j] / determinant;
			}
		}

		return inverse_mat;
	}

	auto system::operator()() ->  std::vector<double>
	{
		auto A_inverse = system::inverse(this->_A);

		int dimension = this->_A[0].size();

		std::vector<double> solution(dimension);
		
		for (int i = 0; i < dimension; i++)
		{
			double tmp = 0.;
			for (int j = 0; j < dimension; j++)
			{
				tmp += A_inverse[i][j] * this->_b[j];
			}

			solution[i] = tmp;
		}

		return solution;
	}
}