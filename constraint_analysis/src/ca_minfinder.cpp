#include "constraint_analysis/ca_minfinder.h"


// ============================================================
// merged from: src/constraint_envelope_analyzer.cpp
// ============================================================
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
// merged from: src/robust_design_point_selector.cpp
// ============================================================
#include <limits>
#include <stdexcept>

namespace constraint_analysis
{
    robust_design_point robust_design_point_selector::select(
        const constraint_curve& envelope,
        const constraint_output& output,
        double landing_margin_ratio)
    {
        if (envelope.points.empty())
        {
            throw std::runtime_error("Envelope has no points.");
        }

        double min_allowed_ws = -std::numeric_limits<double>::infinity();
        double max_allowed_ws = std::numeric_limits<double>::infinity();

        for (const auto& vc : output.vertical_constraints)
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

        const double safe_ws_limit = max_allowed_ws * landing_margin_ratio;

        robust_design_point best;
        best.score = std::numeric_limits<double>::infinity();
        best.feasible = false;
        best.reason = "No feasible point found.";

        for (const auto& envelope_point : envelope.points)
        {
            const double ws = envelope_point.x;
            const double tw = envelope_point.y;

            if (ws < min_allowed_ws || ws > safe_ws_limit)
            {
                continue;
            }

            bool range_feasible = true;

            for (const auto& range_result : output.range_constraints)
            {
                bool found_point = false;
                bool local_feasible = false;

                for (const auto& range_point : range_result.points)
                {
                    if (range_point.wing_loading == ws)
                    {
                        found_point = true;
                        local_feasible = range_point.feasible;
                        break;
                    }
                }

                if (found_point)
                {
                    range_feasible = range_feasible && local_feasible;
                }
            }

            if (!range_feasible)
            {
                continue;
            }

            const double landing_usage =
                (max_allowed_ws < std::numeric_limits<double>::infinity())
                    ? ws / max_allowed_ws
                    : 0.0;

            const double landing_penalty =
                landing_usage > 0.85
                    ? 0.20 * (landing_usage - 0.85)
                    : 0.0;

            const double score = tw + landing_penalty;

            if (score < best.score)
            {
                best.wing_loading = ws;
                best.thrust_to_weight = tw;
                best.score = score;
                best.feasible = true;
                best.reason = "Selected minimum scored feasible point.";
            }
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
