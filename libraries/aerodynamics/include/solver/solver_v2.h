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

#ifndef AERODYNAMICS_LINEAR_SOLVER_V2_H_
#define AERODYNAMICS_LINEAR_SOLVER_V2_H_

#include <string>
#include <cstdint>
#include <vector>
#include "interpolation/interpolation_v2.h"
#include "interpolation/data_types.h"

namespace linear
{
	auto get_slope(const types::VectorXd& y, const types::VectorXd& x) -> types::VectorXd;

	auto get_intercept(const types::VectorXd& y, const types::VectorXd& x, const types::VectorXd& dx_dy) -> types::VectorXd;

	auto get_quadratic_coefficients(const types::VectorXd& y, const types::VectorXd& x) -> types::VectorXd;

	auto get_break(const types::VectorXd& y, const types::VectorXd& x, const double factor) -> double;

	struct system
	{
		system(const types::MatrixXRd& A, types::VectorXd& b) : _A(A), _b(b)
		{}

		static auto det(types::MatrixXRd& matrix) -> double;

		static auto inverse(types::MatrixXRd& matrix) -> types::MatrixXRd;

		auto operator()()->types::VectorXd;

		types::MatrixXRd _A;
		types::VectorXd _b;
	};
}

#endif