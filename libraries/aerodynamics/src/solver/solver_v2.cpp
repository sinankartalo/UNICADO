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

#include "solver/solver_v2.h"
#include "interpolation/data_types.h"
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
    auto get_slope(const types::VectorXd& y, const types::VectorXd& x) -> types::VectorXd
    {
        const int n = static_cast<int>(y.size()) - 1;
        return (y.tail(n) - y.head(n)).cwiseQuotient(x.tail(n) - x.head(n));
    }

    auto get_intercept(const types::VectorXd& y, const types::VectorXd& x, const types::VectorXd& dx_dy) -> types::VectorXd
    {
        const int n = static_cast<int>(dx_dy.size());
        return y.head(n) - dx_dy.cwiseProduct(x.head(n));
    }

    auto get_quadratic_coefficients(const types::VectorXd& y, const types::VectorXd& x) -> types::VectorXd
    {
        const int n = static_cast<int>(x.size());
        const auto xa = x.array();
        const auto ya = y.array();

        const auto x2 = xa.square();
        const double Sx = xa.sum();
        const double Sx2 = x2.sum();
        const double Sx3 = (x2 * xa).sum();
        const double Sx4 = x2.square().sum();
        const double Sy = ya.sum();
        const double Sxy = (xa * ya).sum();
        const double Sx2y = (x2 * ya).sum();

        types::MatrixXRd A(3, 3);
        A << Sx4, Sx3, Sx2,
            Sx3, Sx2, Sx,
            Sx2, Sx, static_cast<double>(n);

        types::VectorXd b(3);
        b << Sx2y, Sxy, Sy;

        if (std::abs(A.determinant()) < 0.001)
            return types::VectorXd::Zero(3);

        return A.colPivHouseholderQr().solve(b);
    }

    auto get_break(const types::VectorXd& y, const types::VectorXd& x, const double factor) -> double
    {
        const int n = static_cast<int>(y.size());
        const types::VectorXd slopes = get_slope(y, x);

        for (int i = 1; i < n - 2; i++)
        {
            if (std::abs(slopes[i]) > std::abs(factor * slopes[i - 1]) + 1.0)
                return y[i];
        }
        return -9999.0;
    }

	auto system::det(types::MatrixXRd& matrix) -> double
	{
		return matrix.determinant();
	}

	auto system::inverse(types::MatrixXRd& matrix) -> types::MatrixXRd
	{
		return matrix.inverse();
	}

	auto system::operator()() ->  types::VectorXd
	{
		return this->_A.inverse()*this->_b;
	}
}