#include "constraint_analysis/ca_minfinder.h"


// ============================================================
// merged from: src/constraint_envelope_analyzer.cpp
// ============================================================
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace constraint_analysis
{
    constraint_curve constraint_envelope_analyzer::build_envelope(const constraint_output& output)
    {
        if (output.curves.empty())
        {
            throw std::runtime_error("No constraint curves available to build envelope.");
        }

        const std::size_t point_count = output.curves.front().points.size();

        for (const auto& curve : output.curves)
        {
            if (curve.points.size() != point_count)
            {
                throw std::runtime_error("All curves must have the same number of points.");
            }
        }

        constraint_curve envelope;
        envelope.name = "constraint_envelope";

        for (std::size_t i = 0; i < point_count; ++i)
        {
            const double x = output.curves.front().points[i].x;
            double y_max = output.curves.front().points[i].y;

            for (const auto& curve : output.curves)
            {
                if (curve.points[i].y > y_max)
                {
                    y_max = curve.points[i].y;
                }
            }

            envelope.points.push_back({x, y_max});
        }

        return envelope;
    }
}


// ============================================================
// merged from: src/design_point_finder.cpp
// ============================================================
#include <stdexcept>

namespace constraint_analysis
{
    design_point design_point_finder::find_minimum_point(const constraint_curve& envelope)
    {
        if (envelope.points.empty())
        {
            throw std::runtime_error("Envelope has no points.");
        }

        design_point best;
        best.wing_loading = envelope.points.front().x;
        best.thrust_to_weight = envelope.points.front().y;

        for (const auto& point : envelope.points)
        {
            if (point.y < best.thrust_to_weight)
            {
                best.wing_loading = point.x;
                best.thrust_to_weight = point.y;
            }
        }

        return best;
    }


    namespace
    {
        double linear_interpolate(
            const curve_point& left,
            const curve_point& right,
            double x)
        {
            const double dx = right.x - left.x;
            if (std::abs(dx) < 1.0e-12)
            {
                return left.y;
            }

            const double ratio = (x - left.x) / dx;
            return left.y + ratio * (right.y - left.y);
        }

        double quadratic_interpolate(
            const curve_point& p0,
            const curve_point& p1,
            const curve_point& p2,
            double x)
        {
            const double d0 = (p0.x - p1.x) * (p0.x - p2.x);
            const double d1 = (p1.x - p0.x) * (p1.x - p2.x);
            const double d2 = (p2.x - p0.x) * (p2.x - p1.x);

            if (std::abs(d0) < 1.0e-12 ||
                std::abs(d1) < 1.0e-12 ||
                std::abs(d2) < 1.0e-12)
            {
                return linear_interpolate(p1, p2, x);
            }

            const double l0 = ((x - p1.x) * (x - p2.x)) / d0;
            const double l1 = ((x - p0.x) * (x - p2.x)) / d1;
            const double l2 = ((x - p0.x) * (x - p1.x)) / d2;

            return p0.y * l0 + p1.y * l1 + p2.y * l2;
        }

        double interpolated_curve_y(
            const constraint_curve& curve,
            std::size_t interval_index,
            double x)
        {
            const std::size_t point_count = curve.points.size();

            // With only two available points, quadratic interpolation is not
            // possible, so ordinary linear interpolation is used.
            if (point_count < 3)
            {
                return linear_interpolate(
                    curve.points[interval_index],
                    curve.points[interval_index + 1],
                    x);
            }

            // Select three neighbouring samples around the current interval.
            // At the left edge use points 0,1,2; elsewhere use i-1,i,i+1.
            std::size_t first_index = 0;
            if (interval_index > 0)
            {
                first_index = interval_index - 1;
            }
            if (first_index + 2 >= point_count)
            {
                first_index = point_count - 3;
            }

            return quadratic_interpolate(
                curve.points[first_index],
                curve.points[first_index + 1],
                curve.points[first_index + 2],
                x);
        }

        double interpolated_envelope_y(
            const constraint_output& output,
            std::size_t interval_index,
            double x)
        {
            double envelope_y = -std::numeric_limits<double>::infinity();

            for (const auto& curve : output.curves)
            {
                const double y = interpolated_curve_y(curve, interval_index, x);
                envelope_y = std::max(envelope_y, y);
            }

            return envelope_y;
        }

        design_point find_interpolated_minimum_impl(
            const constraint_output& output,
            double min_allowed_ws,
            double max_allowed_ws)
        {
            if (output.curves.empty())
            {
                throw std::runtime_error(
                    "No constraint curves available for interpolated minimum search.");
            }

            const std::size_t point_count = output.curves.front().points.size();
            if (point_count < 2)
            {
                throw std::runtime_error(
                    "At least two W/S points are required for interpolation.");
            }

            for (const auto& curve : output.curves)
            {
                if (curve.points.size() != point_count)
                {
                    throw std::runtime_error(
                        "All curves must have the same number of points.");
                }
            }

            design_point best;
            double best_y = std::numeric_limits<double>::infinity();
            bool found = false;

            // Each original W/S interval is scanned finely. The individual
            // constraints are reconstructed with local quadratic interpolation,
            // then their maximum is taken to form the envelope. This captures a
            // smooth minimum between grid points, which piecewise-linear
            // interpolation cannot detect when one active curve is U-shaped.
            constexpr std::size_t subdivisions_per_interval = 1000;

            for (std::size_t i = 0; i + 1 < point_count; ++i)
            {
                const double interval_left = output.curves.front().points[i].x;
                const double interval_right = output.curves.front().points[i + 1].x;
                const double clipped_left = std::max(interval_left, min_allowed_ws);
                const double clipped_right = std::min(interval_right, max_allowed_ws);

                if (clipped_left > clipped_right)
                {
                    continue;
                }

                for (std::size_t step = 0; step <= subdivisions_per_interval; ++step)
                {
                    const double ratio =
                        static_cast<double>(step) /
                        static_cast<double>(subdivisions_per_interval);
                    const double x =
                        clipped_left + ratio * (clipped_right - clipped_left);
                    const double y = interpolated_envelope_y(output, i, x);

                    if (!found || y < best_y)
                    {
                        best.wing_loading = x;
                        best.thrust_to_weight = y;
                        best_y = y;
                        found = true;
                    }
                }
            }

            if (!found)
            {
                throw std::runtime_error(
                    "No feasible interpolated design point was found.");
            }

            return best;
        }
    }

    design_point design_point_finder::find_interpolated_minimum_point(
        const constraint_output& output)
    {
        return find_interpolated_minimum_impl(
            output,
            -std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity());
    }

    design_point design_point_finder::find_interpolated_feasible_minimum_point(
        const constraint_output& output,
        const std::vector<vertical_constraint>& vertical_constraints)
    {
        double min_allowed_ws = -std::numeric_limits<double>::infinity();
        double max_allowed_ws = std::numeric_limits<double>::infinity();

        for (const auto& vc : vertical_constraints)
        {
            if (vc.is_upper_limit)
            {
                max_allowed_ws = std::min(max_allowed_ws, vc.x_limit);
            }
            else
            {
                min_allowed_ws = std::max(min_allowed_ws, vc.x_limit);
            }
        }

        return find_interpolated_minimum_impl(
            output,
            min_allowed_ws,
            max_allowed_ws);
    }
}


// ============================================================
// merged from: src/feasible_design_point_finder.cpp
// ============================================================
#include <limits>
#include <stdexcept>

namespace constraint_analysis
{
    design_point feasible_design_point_finder::find_feasible_minimum_point(
        const constraint_curve& envelope,
        const std::vector<vertical_constraint>& vertical_constraints)
    {
        if (envelope.points.empty())
        {
            throw std::runtime_error("Envelope has no points.");
        }

        double min_allowed_ws = -std::numeric_limits<double>::infinity();
        double max_allowed_ws = std::numeric_limits<double>::infinity();

        for (const auto& vc : vertical_constraints)
        {
            if (vc.is_upper_limit)
            {
                if (vc.x_limit < max_allowed_ws)
                {
                    max_allowed_ws = vc.x_limit;
                }
            }
            else
            {
                if (vc.x_limit > min_allowed_ws)
                {
                    min_allowed_ws = vc.x_limit;
                }
            }
        }

        bool found_feasible_point = false;
        design_point best;

        for (const auto& point : envelope.points)
        {
            if (point.x >= min_allowed_ws && point.x <= max_allowed_ws)
            {
                if (!found_feasible_point || point.y < best.thrust_to_weight)
                {
                    best.wing_loading = point.x;
                    best.thrust_to_weight = point.y;
                    found_feasible_point = true;
                }
            }
        }

        if (!found_feasible_point)
        {
            throw std::runtime_error("No feasible design point found within vertical constraints.");
        }

        return best;
    }
}


// ============================================================
// merged from: src/active_constraint_analyzer.cpp
// ============================================================
#include <stdexcept>

namespace constraint_analysis
{
    std::vector<active_constraint_point> active_constraint_analyzer::analyze(const constraint_output& output)
    {
        if (output.curves.empty())
        {
            throw std::runtime_error("No constraint curves available for active constraint analysis.");
        }

        const std::size_t point_count = output.curves.front().points.size();

        for (const auto& curve : output.curves)
        {
            if (curve.points.size() != point_count)
            {
                throw std::runtime_error("All curves must have the same number of points.");
            }
        }

        std::vector<active_constraint_point> result;

        for (std::size_t i = 0; i < point_count; ++i)
        {
            active_constraint_point point;
            point.x = output.curves.front().points[i].x;
            point.y = output.curves.front().points[i].y;
            point.active_constraint_name = output.curves.front().name;

            for (const auto& curve : output.curves)
            {
                if (curve.points[i].y > point.y)
                {
                    point.y = curve.points[i].y;
                    point.active_constraint_name = curve.name;
                }
            }

            result.push_back(point);
        }

        return result;
    }
}
