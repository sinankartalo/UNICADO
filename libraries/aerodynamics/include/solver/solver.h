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

#ifndef AERODYNAMICS_LINEAR_SOLVER_H_
#define AERODYNAMICS_LINEAR_SOLVER_H_

#include <string>
#include <cstdint>
#include <vector>
#include "interpolation/interpolation.h"

namespace linear
{
	auto get_slope(const std::vector<double>& y, const std::vector<double> x) -> value;

	auto get_intercept(const std::vector<double>& y, const std::vector<double> x, const std::vector<double> dx_dy) -> value;

	auto get_quadratic_coefficients(const std::vector<double>& y, const std::vector<double>& x) -> std::vector<double>;

	auto get_break(const std::vector<double>& y, const std::vector<double>& x, const double factor) -> value;

	struct system
	{
		system(const std::vector<std::vector<double>>& A, const std::vector<double>& b) : _A(A), _b(b)
		{}

		static auto minor(std::vector<std::vector<double>> matrix, int row, int col) -> std::vector<std::vector<double>>;

		static auto det(std::vector<std::vector<double>>& matrix) -> double;

		static auto adj(std::vector<std::vector<double>>& matrix) -> std::vector<std::vector<double>>;

		static auto inverse(std::vector<std::vector<double>>& matrix) -> std::vector<std::vector<double>>;

		auto operator()()->std::vector<double>;

		std::vector<std::vector<double>> _A;
		std::vector<double> _b;
	};
}

#endif