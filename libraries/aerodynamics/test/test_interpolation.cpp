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
#include "interpolation/interpolation.h"
#include <random>

TEST(Interpolation, InitializationTest)
{
    map1 lhs;

    lhs["config"]["M"] = std::vector<double>{ 0.7, 0.8, 0.75, 0.82 };
    lhs["config"]["h"] = std::vector<double>{ 10000, 12000, 8000, 10000 };

    map2 rhs;

    std::mt19937 gen(42); //fixed seed
    std::uniform_real_distribution<double> dist(0.0, 1.);

    auto randomize_properties = [&]() {
        return std::unordered_map<std::string, value>{
            {"CL_alpha", dist(gen)},
            { "CL_0", dist(gen) },
            { "CM_alpha", dist(gen) },
            { "CM_0", dist(gen) },
            { "CD_0", dist(gen) },
            { "CD_PB", dist(gen) },
            { "K1", dist(gen) },
            { "K2", dist(gen) },
            { "K3", dist(gen) },
            { "K4", dist(gen) },
            { "alpha_PB", dist(gen) } };
        };

    auto M_vec = get_vector(lhs["config"]["M"]);
    auto h_vec = get_vector(lhs["config"]["h"]);

    for (int i = 0; i < M_vec.size(); i++)
    {
        rhs["config"][M_vec[i]][h_vec[i]] = randomize_properties();
    }

    linear::interpolation interp(lhs, rhs, "config");

    EXPECT_EQ(interp.Delaunay_triangulation.number_of_vertices(), 4);
}

TEST(Interpolation, TriangulationTest)
{
    std::vector<std::vector<double>> points = {
        {0.0, 0.0},
        {1.0, 0.0},
        {0.0, 1.0}
    };

    std::vector<properties> values(points.size());

    for (int i = 0; i < values.size(); ++i) {
        properties p{};
        p.CL_alpha = static_cast<double>(i);
        p.CL_0 = static_cast<double>(i) + 0.5;
        values[i] = p;
    }

    auto delaunay = linear::interpolation::triangulation(points, values);

    EXPECT_EQ(delaunay.current_dimension(), 2);

    EXPECT_EQ(delaunay.number_of_vertices(), points.size());

    int found = 0;

    for (auto vertex = delaunay.finite_vertices_begin(); vertex != delaunay.finite_vertices_end(); ++vertex) {
        const auto prop = vertex->data();

        auto it = std::find_if(values.begin(), values.end(),
            [&](const properties& p) {
                return std::fabs(p.CL_alpha - prop.CL_alpha) < 1e-9 &&
                    std::fabs(p.CL_0 - prop.CL_0) < 1e-9;
            });
        EXPECT_TRUE(it != values.end());
        ++found;
    }

    EXPECT_EQ(found, points.size());
}

TEST(Interpolation, CalculateBarycentricCoordinates2DTest)
{
    std::vector<Point_CGAL> vertices = {
        Point_CGAL({0.0, 0.0}),
        Point_CGAL({10.0, 0.0}),
        Point_CGAL({0.0, 10.0})
    };


    Point_CGAL test_point({2.5, 2.5});

    auto coords = linear::interpolation::calculate_barycentric_coordinates(vertices, test_point);

    ASSERT_EQ(coords.size(), 3);
    EXPECT_NEAR(coords[0], 0.5, 1e-9);
    EXPECT_NEAR(coords[1], 0.25, 1e-9);
    EXPECT_NEAR(coords[2], 0.25, 1e-9);
}

TEST(Interpolation, CalculateBarycentricCoordinates3DTest)
{
    std::vector<Point_CGAL> vertices = {
        Point_CGAL({0.0, 0.0, 0.0}),
        Point_CGAL({10.0, 0.0, 0.0}),
        Point_CGAL({0.0, 10.0, 0.0}),
        Point_CGAL({0.0, 0.0, 10.0})
    };

    Point_CGAL test_point({2.5, 2.5, 2.5});

    auto coords = linear::interpolation::calculate_barycentric_coordinates(vertices, test_point);

    ASSERT_EQ(coords.size(), 4);
    EXPECT_NEAR(coords[0], 0.25, 1e-9);
    EXPECT_NEAR(coords[1], 0.25, 1e-9);
    EXPECT_NEAR(coords[2], 0.25, 1e-9);
    EXPECT_NEAR(coords[3], 0.25, 1e-9);
}

TEST(Interpolation, LocatePointTest)
{
    std::vector<std::vector<double>> points = {
        {0.0, 0.0},
        {10.0, 0.0},
        {0.0, 10.0},
        {-5.0, 0.0},
        {-10.0, 0.0},
        {0.0, -10.0}
    };

    std::vector<properties> values(points.size());

    for (int i = 0; i < values.size(); ++i) {
        properties p{};
        p.CL_alpha = static_cast<double>(i);
        p.CL_0 = static_cast<double>(i) + 0.5;
        values[i] = p;
    }

    auto delaunay = linear::interpolation::triangulation(points, values);

    std::vector<double> conditions = { 2.5, 2.5 };

    Point_CGAL point(conditions);

    auto simplex_it = delaunay.locate(point);
    auto& simplex = *simplex_it;

    std::vector<Point_CGAL> vertices = {};
    std::vector<properties> vertex_properties = {};

    for (int i = 0; i <= point.size(); i++)
    {
        vertices.push_back(simplex.vertex(i)->point());
        vertex_properties.push_back(simplex.vertex(i)->data());
    }

    ASSERT_EQ(vertices.size(), 3);

    auto coords = linear::interpolation::calculate_barycentric_coordinates(vertices, point);

    ASSERT_EQ(coords.size(), 3);
    EXPECT_NEAR(coords[0], 0.5, 1e-9);
    EXPECT_NEAR(coords[1], 0.25, 1e-9);
    EXPECT_NEAR(coords[2], 0.25, 1e-9);
}

TEST(Interpolation, LocatePointWithInitTest)
{
    map1 lhs;

    lhs["config"]["M"] = std::vector<double>{ 0.0, 10., 0.0, -5.0, -10., 0. };
    lhs["config"]["h"] = std::vector<double>{ 0., 0., 10., 0., 0., -10. };

    map2 rhs;

    auto M_vec = get_vector(lhs["config"]["M"]);
    auto h_vec = get_vector(lhs["config"]["h"]);

    std::vector<std::unordered_map<std::string, value>> values(M_vec.size());
    for (int i = 0; i < values.size(); ++i) {
        std::unordered_map<std::string, value> p{};
        p["CL_alpha"] = static_cast<double>(i);
        p["CL_0"] = static_cast<double>(i) + 0.5;
        p["CM_alpha"] = static_cast<double>(i) + 1.;
        p["CM_0"] = static_cast<double>(i) + 1.5;
        p["CD_PB"] = static_cast<double>(i) + 2.;
        p["K1"] = static_cast<double>(i) + 2.5;
        p["K2"] = static_cast<double>(i) + 3.;
        p["K3"] = static_cast<double>(i) + 3.5;
        p["K4"] = static_cast<double>(i) + 4.;
        p["alpha_PB"] = static_cast<double>(i) + 4.5;
        values[i] = p;
    }

    for (int i = 0; i < M_vec.size(); i++)
    {
        rhs["config"][M_vec[i]][h_vec[i]] = values[i];
    }

    linear::interpolation interp(lhs, rhs, "config");

    std::vector<double> conditions = { 1.0, 2.5 };

    interp.conditions = conditions;

    interp.update();

    conditions = { 2.5, 2.5 };

    interp.conditions = conditions;

    interp.update();

    auto coords = interp.barycentric_coordinates;

    ASSERT_EQ(coords.size(), 3);
    EXPECT_NEAR(coords[0], 0.5, 1e-9);
    EXPECT_NEAR(coords[1], 0.25, 1e-9);
    EXPECT_NEAR(coords[2], 0.25, 1e-9);
}