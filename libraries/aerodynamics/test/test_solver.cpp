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

#include <gtest/gtest.h>
#include "solver/solver.h"
#include "interpolation/interpolation.h"

TEST(Slope, SlopeCalculationTest)
{
	std::vector<double> x = { 0., 1., 2., 3., 4., 5., 6., 7., 8. };
	std::vector<double> y(x.size());
	for (int i = 0; i < x.size(); i++)
	{
		y[i] = pow(x[i], 2.) + 2. * x[i] + 1.;
	}
	std::vector<double> dy_dx = get_vector(linear::get_slope(y, x));

	for (int i = 0; i < dy_dx.size(); i++)
	{
		EXPECT_NEAR(dy_dx[i], ((2. * x[i] + 2.) + (2. * x[i + 1] + 2.)) / 2., 1e-3);
	}
}

TEST(Intercept, InterceptCalculationTest)
{
	std::vector<double> x = { 0., 1., 2., 3., 4., 5., 6., 7., 8. };
	std::vector<double> y(x.size());
	for (int i = 0; i < x.size(); i++)
	{
		y[i] = pow(x[i], 2.) + 2. * x[i] + 1.;
	}
	auto dy_dx = get_vector(linear::get_slope(y, x));
	auto intercept = get_vector(linear::get_intercept(y, x, dy_dx));
	for (int i = 0; i < dy_dx.size(); i++)
	{
		EXPECT_NEAR(intercept[i], y[i] - dy_dx[i] * x[i], 1e-3);
	}
}

TEST(System, SystemInitializationTest)
{
	std::vector<std::vector<double>> A = {{1., 0., 0.}, { 0., 1., 0. }, { 0., 0., 1. }};
	std::vector<double> b = { 1., 2., 3. };
	auto sys = linear::system(A, b);

	ASSERT_EQ(sys._A[0].size(), 3);
}

TEST(System, SystemMinorTest3x3)
{
	std::vector<std::vector<double>> A = { {1., 0., 0.}, { 0., 2., 0. }, { 0., 0., 3. } };
	auto minor = linear::system::minor(A, 0, 0);

	std::vector<std::vector<double>> expected = { {2. , 0.}, {0., 3.} };
	EXPECT_EQ(minor,expected);
}

TEST(System, SystemDetTest3x3)
{
	std::vector<std::vector<double>> A = {
		{ 2.0, 5.0, 7.0 },
		{ 6.0, 3.0, 4.0 },
		{ 5.0, -2.0, -3.0 }
	};
	auto determinant = linear::system::det(A);

	EXPECT_EQ(determinant, -1.);
}

TEST(System, SystemDetTest4x4)
{
	std::vector<std::vector<double>> A = {
		{ 1.0, 2.0, 3.0, 4.0 },
		{ 5.0, 6.0, 7.0, 8.0 },
		{ 2.0, 6.0, 4.0, 8.0 },
		{ 3.0, 1.0, 1.0, 2.0 }
	};
	auto determinant = linear::system::det(A);

	EXPECT_EQ(determinant, 72.);
}

TEST(System, SystemAdjTest3x3)
{
	std::vector<std::vector<double>> A = {
		{ 2.0, 5.0, 7.0 },
		{ 6.0, 3.0, 4.0 },
		{ 5.0, -2.0, -3.0 }
	};
	auto adjoint = linear::system::adj(A);

	std::vector<std::vector<double>> expected = {
		{ -1.0,  1.0,   -1.0 },
		{ 38.0, -41.0,   34.0 },
		{ -27.0, 29.0,  -24.0 }
	};

	EXPECT_EQ(adjoint, expected);
}

TEST(System, SystemAdjTest4x4)
{
	std::vector<std::vector<double>> A = {
		{ 1.0, 2.0, 3.0, 4.0 },
		{ 5.0, 6.0, 7.0, 8.0 },
		{ 2.0, 6.0, 4.0, 8.0 },
		{ 3.0, 1.0, 1.0, 2.0 }
	};
	auto adjoint = linear::system::adj(A);

	std::vector<std::vector<double>> expected = {
		{ -12.0,  4.0,  -4.0,    24.0 },
		{ -60.0,  20.0,  16.0,  -24.0 },
		{  12.0,  20.0, -20.0,  -24.0 },
		{  42.0, -26.0,  8.0,   24.0 }
	};

	EXPECT_EQ(adjoint, expected);
}

TEST(System, SystemInverseTest3x3)
{
	std::vector<std::vector<double>> A = {
		{ 2.0, 5.0, 7.0 },
		{ 6.0, 3.0, 4.0 },
		{ 5.0, -2.0, -3.0 }
	};
	auto inverse = linear::system::inverse(A);

	std::vector<std::vector<double>> expected = {
		{ 1.00, -1.00,  1.00 },
		{ -38.00, 41.00, -34.00 },
		{ 27.00, -29.00, 24.00 }
	};

	for (size_t i = 0; i < expected.size(); ++i)
	{
		for (size_t j = 0; j < expected[i].size(); ++j)
		{
			EXPECT_NEAR(inverse[i][j], expected[i][j], 1e-2);
		}
	}
}

TEST(System, SystemInverseTest4x4)
{
	std::vector<std::vector<double>> A = {
		{ 1.0, 2.0, 3.0, 4.0 },
		{ 5.0, 6.0, 7.0, 8.0 },
		{ 2.0, 6.0, 4.0, 8.0 },
		{ 3.0, 1.0, 1.0, 2.0 }
	};
	auto inverse = linear::system::inverse(A);

	std::vector<std::vector<double>> expected = {
		{ -0.1667,  0.05555, -0.05555,  0.3333 },
		{  -0.8333, 0.2777,  0.2222, -0.3333 },
		{ 0.1666,  0.2777,  -0.2777, -0.3333 },
		{ 0.5833,  -0.3611,  0.1111,  0.3333 }
	};

	for (size_t i = 0; i < expected.size(); ++i)
	{
		for (size_t j = 0; j < expected[i].size(); ++j)
		{
			EXPECT_NEAR(inverse[i][j], expected[i][j], 1e-2);
		}
	}
}

TEST(System, SystemSolverTest3x3)
{
	std::vector<std::vector<double>> A = {
		{ 2.0, 5.0, 7.0 },
		{ 6.0, 3.0, 4.0 },
		{ 5.0, -2.0, -3.0 }
	};
	
	std::vector<double> b = { 33.0, 24.0, -8.0 };

	auto sys = linear::system(A, b);

	auto x = sys();

	for (int i = 0; i < A.size(); i++)
	{
		double lhs = 0.;

		for (int j = 0; j < A[i].size(); j++)
		{
			lhs += A[i][j] * x[j];
		}
		EXPECT_NEAR(lhs, b[i], 1e-2);
	}
}

TEST(System, SystemSolverTest4x4)
{
	std::vector<std::vector<double>> A = {
		{ 1.0, 2.0, 3.0, 4.0 },
		{ 5.0, 6.0, 7.0, 8.0 },
		{ 2.0, 6.0, 4.0, 8.0 },
		{ 3.0, 1.0, 1.0, 2.0 }
	};

	std::vector<double> b = { 30.0, 70.0, 58.0, 16.0 }; 

	auto sys = linear::system(A, b);

	auto x = sys();

	for (int i = 0; i < (int)A.size(); i++)
	{
		double lhs = 0.0;
		for (int j = 0; j < (int)A[i].size(); j++)
		{
			lhs += A[i][j] * x[j];
		}
		EXPECT_NEAR(lhs, b[i], 1e-2);
	}
}