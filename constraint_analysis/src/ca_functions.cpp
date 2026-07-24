#include "constraint_analysis/ca_functions.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

// ============================================================
// merged from: src/constraint_utilities.cpp
// ============================================================
namespace constraint_analysis
{
    double constraint_utilities::compute_dynamic_pressure(double density, double speed)
    {
        return 0.5 * density * speed * speed;
    }

    double constraint_utilities::compute_lift_coefficient(double wing_loading, double q, double load_factor)
    {
        return load_factor * wing_loading / q;
    }

    double constraint_utilities::compute_drag_coefficient(double cd_0, double k, double cl)
    {
        return cd_0 + k * cl * cl;
    }
}

// ============================================================
// Propeller power-loading constraint analysis
// ============================================================
namespace constraint_analysis
{
    namespace
    {
        struct propeller_deck_row
        {
            double pitch_deg = 0.0;
            double advance_ratio = 0.0;
            double thrust_coefficient = 0.0;
            double power_coefficient = 0.0;
            double efficiency = 0.0;
        };

        const std::vector<propeller_deck_row>& read_propeller_deck(
            const std::string& deck_path)
        {
            // The UNICADO Propeller class uses a general two-dimensional
            // Delaunay interpolator.  This deck, however, consists of three
            // independent constant-pitch curves.  Cache the raw rows so the
            // constraint analysis can interpolate on the selected 1-D curve
            // without repeatedly reading the CSV or querying a hull boundary.
            static std::unordered_map<
                std::string, std::vector<propeller_deck_row>> cache;

            const auto cached = cache.find(deck_path);
            if (cached != cache.end())
                return cached->second;

            std::ifstream file(deck_path);
            if (!file)
                throw std::runtime_error(
                    "Cannot open propeller deck: " + deck_path);

            std::vector<propeller_deck_row> rows;
            std::string line;
            while (std::getline(file, line))
            {
                if (line.empty())
                    continue;

                // prop.csv is UTF-8 with a BOM. Remove it before parsing the
                // first pitch value so the J=0 row is not silently discarded.
                if (line.size() >= 3 &&
                    static_cast<unsigned char>(line[0]) == 0xEF &&
                    static_cast<unsigned char>(line[1]) == 0xBB &&
                    static_cast<unsigned char>(line[2]) == 0xBF)
                {
                    line.erase(0, 3);
                }

                std::stringstream stream(line);
                std::string token;
                std::vector<double> values;
                while (std::getline(stream, token, ','))
                    values.push_back(std::stod(token));

                if (values.size() < 5)
                    throw std::runtime_error(
                        "Invalid row in propeller deck: " + line);

                rows.push_back({
                    values[0], values[1], values[2], values[3], values[4]});
            }

            if (rows.empty())
                throw std::runtime_error(
                    "Propeller deck contains no numeric rows: " + deck_path);

            return cache.emplace(deck_path, std::move(rows)).first->second;
        }

        propeller_deck_row interpolate_propeller_pitch_slice(
            const std::string& deck_path,
            double pitch_deg,
            double advance_ratio)
        {
            const auto& rows = read_propeller_deck(deck_path);
            const propeller_deck_row* previous = nullptr;

            for (const auto& row : rows)
            {
                if (std::abs(row.pitch_deg - pitch_deg) > 1.0e-9)
                    continue;

                if (std::abs(row.advance_ratio - advance_ratio) < 1.0e-12)
                    return row;

                if (previous != nullptr &&
                    previous->advance_ratio <= advance_ratio &&
                    advance_ratio <= row.advance_ratio)
                {
                    const double fraction =
                        (advance_ratio - previous->advance_ratio) /
                        (row.advance_ratio - previous->advance_ratio);
                    return {
                        pitch_deg,
                        advance_ratio,
                        previous->thrust_coefficient +
                            fraction * (row.thrust_coefficient -
                                        previous->thrust_coefficient),
                        previous->power_coefficient +
                            fraction * (row.power_coefficient -
                                        previous->power_coefficient),
                        previous->efficiency +
                            fraction * (row.efficiency -
                                        previous->efficiency)};
                }

                previous = &row;
            }

            throw std::runtime_error(
                "Propeller operating point is outside the selected pitch "
                "slice in deck: pitch=" + std::to_string(pitch_deg) +
                " deg, J=" + std::to_string(advance_ratio));
        }

        double power_loading_from_thrust_loading(
            double thrust_to_weight,
            const propeller_operating_point& operating_point)
        {
            if (operating_point.thrust_N <= 0.0 ||
                operating_point.shaft_power_W <= 0.0)
            {
                throw std::runtime_error(
                    "Propeller deck returned non-positive thrust or power.");
            }

            // P/W = (T/W)*(P/T).  P/T is taken from the same CT/CP
            // interpolation used by aerodynamics::Propeller.
            return thrust_to_weight *
                operating_point.shaft_power_W / operating_point.thrust_N;
        }
    }

    propeller_constraint_analysis::propeller_constraint_analysis(
        const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    propeller_operating_point propeller_constraint_analysis::evaluate(
        const constraint_input& input,
        double altitude_m,
        double speed_ms,
        const propeller_setting& setting) const
    {
        if (input.propeller.model == nullptr)
        {
            throw std::runtime_error("Propeller operating point has no model.");
        }
        if (input.propeller.diameter_m <= 0.0 || setting.rpm <= 0.0 ||
            speed_ms < 0.0)
        {
            throw std::runtime_error(
                "Propeller diameter/RPM must be positive and speed non-negative.");
        }

        propeller_operating_point point;
        point.altitude_m = altitude_m;
        point.speed_ms = speed_ms;
        point.density_kg_m3 = atmosphere_.getDensity(altitude_m);
        point.rpm = setting.rpm;
        point.pitch_deg = setting.pitch_deg;

        const double rotations_per_second = setting.rpm / 60.0;
        point.advance_ratio =
            speed_ms / (rotations_per_second * input.propeller.diameter_m);

        // The three pitch slices in the supplied deck do not share the same
        // J range.  Keep test operation on an actual slice and inside that
        // slice instead of silently extrapolating.
        double minimum_j = 0.0;
        double maximum_j = 0.0;
        if (std::abs(setting.pitch_deg - 15.0) < 1.0e-9)
        {
            minimum_j = 0.0;
            maximum_j = 1.05;
        }
        else if (std::abs(setting.pitch_deg - 30.0) < 1.0e-9)
        {
            minimum_j = 0.5;
            maximum_j = 1.5;
        }
        else if (std::abs(setting.pitch_deg - 45.0) < 1.0e-9)
        {
            minimum_j = 0.75;
            maximum_j = 2.8;
        }
        else
        {
            throw std::runtime_error(
                "Test propeller deck supports pitch 15, 30, or 45 deg.");
        }

        if (point.advance_ratio < minimum_j ||
            point.advance_ratio > maximum_j)
        {
            throw std::runtime_error(
                "Propeller operating point is outside the selected pitch slice.");
        }

        const auto deck_row = interpolate_propeller_pitch_slice(
            input.propeller.deck_path,
            setting.pitch_deg,
            point.advance_ratio);
        point.thrust_coefficient = deck_row.thrust_coefficient;
        point.power_coefficient = deck_row.power_coefficient;
        point.efficiency = deck_row.efficiency;

        const double diameter = input.propeller.diameter_m;
        point.thrust_N =
            point.thrust_coefficient * point.density_kg_m3 *
            std::pow(rotations_per_second, 2.0) * std::pow(diameter, 4.0);
        point.shaft_power_W =
            point.power_coefficient * point.density_kg_m3 *
            std::pow(rotations_per_second, 3.0) * std::pow(diameter, 5.0);

        if (!std::isfinite(point.thrust_N) ||
            !std::isfinite(point.shaft_power_W) ||
            point.thrust_N <= 0.0 || point.shaft_power_W <= 0.0)
        {
            throw std::runtime_error(
                "Propeller deck produced an invalid operating point.");
        }

        return point;
    }

    constraint_curve propeller_constraint_analysis::compute_takeoff_constraint(
        const constraint_input& input) const
    {
        constraint_curve curve;
        curve.name = "propeller_takeoff_constraint";

        for (double ws = input.wing_loading_min;
             ws <= input.wing_loading_max;
             ws += input.wing_loading_step)
        {
            curve.points.push_back({
                ws,
                solve_takeoff_ground_roll(input, ws).required_shaft_power_to_weight_W_N
            });
        }

        return curve;
    }

    propeller_takeoff_result propeller_constraint_analysis::solve_takeoff_ground_roll(
        const constraint_input& input,
        double wing_loading_N_m2,
        bool retain_steps) const
    {
        if (wing_loading_N_m2 <= 0.0 || input.takeoff.runway_m <= 0.0 ||
            input.aircraft.takeoff_weight_N <= 0.0)
        {
            throw std::runtime_error("Integrated propeller takeoff received invalid input.");
        }

        constexpr int integration_steps = 240;
        constexpr int bisection_iterations = 70;
        const double g = 9.80665;
        const double rho = atmosphere_.getDensity(input.takeoff.altitude_m);
        const double beta = input.takeoff.beta_to;
        const double weight_N = input.aircraft.takeoff_weight_N;
        const double cl_ground = 0.8 * input.aircraft.cl_max_takeoff;
        const double stall_speed_ms = std::sqrt(
            2.0 * beta * wing_loading_N_m2 /
            (rho * input.aircraft.cl_max_takeoff));
        const double takeoff_speed_ms =
            input.takeoff.speed_factor * stall_speed_ms;
        const double dv = takeoff_speed_ms / integration_steps;

        auto integrate = [&](double power_to_weight_W_N,
                             bool keep_steps) -> propeller_takeoff_result
        {
            propeller_takeoff_result result;
            result.wing_loading_N_m2 = wing_loading_N_m2;
            result.takeoff_speed_ms = takeoff_speed_ms;
            result.required_shaft_power_to_weight_W_N = power_to_weight_W_N;
            double distance_m = 0.0;

            if (keep_steps)
                result.steps.reserve(integration_steps);

            for (int index = 0; index < integration_steps; ++index)
            {
                const double speed_ms = (index + 0.5) * dv;
                const double advance_ratio =
                    speed_ms /
                    ((input.propeller.takeoff.rpm / 60.0) *
                     input.propeller.diameter_m);
                std::vector<double> valid_pitches;
                if (advance_ratio <= 1.05)
                    valid_pitches.push_back(15.0);
                if (advance_ratio >= 0.5 && advance_ratio <= 1.5)
                    valid_pitches.push_back(30.0);
                if (advance_ratio >= 0.75 && advance_ratio <= 2.8)
                    valid_pitches.push_back(45.0);
                if (valid_pitches.empty())
                    throw std::runtime_error(
                        "No valid test-deck pitch slice for integrated takeoff.");

                propeller_operating_point deck_point;
                double best_power_per_thrust =
                    std::numeric_limits<double>::infinity();
                for (double pitch_deg : valid_pitches)
                {
                    propeller_setting candidate = input.propeller.takeoff;
                    candidate.pitch_deg = pitch_deg;
                    const auto candidate_point = evaluate(
                        input, input.takeoff.altitude_m, speed_ms, candidate);
                    const double candidate_power_per_thrust =
                        candidate_point.shaft_power_W /
                        candidate_point.thrust_N;
                    if (candidate_power_per_thrust < best_power_per_thrust)
                    {
                        best_power_per_thrust = candidate_power_per_thrust;
                        deck_point = candidate_point;
                    }
                }
                const double power_per_thrust_ms =
                    deck_point.shaft_power_W / deck_point.thrust_N;
                const double thrust_to_weight =
                    power_to_weight_W_N / power_per_thrust_ms;
                const double q = 0.5 * rho * speed_ms * speed_ms;
                const double lift_to_weight =
                    q * cl_ground / wing_loading_N_m2;
                const double drag_to_weight =
                    q * input.takeoff.cd_ground / wing_loading_N_m2;
                const double rolling_to_weight =
                    input.takeoff.mu_ro *
                    std::max(0.0, beta - lift_to_weight);
                const double acceleration_ms2 =
                    g / beta *
                    (thrust_to_weight - drag_to_weight - rolling_to_weight);

                if (!std::isfinite(acceleration_ms2) ||
                    acceleration_ms2 <= 0.0)
                {
                    result.integrated_ground_roll_m =
                        std::numeric_limits<double>::infinity();
                    result.steps.clear();
                    return result;
                }

                distance_m += speed_ms * dv / acceleration_ms2;

                if (keep_steps)
                {
                    propeller_takeoff_step step;
                    step.speed_ms = speed_ms;
                    step.distance_m = distance_m;
                    step.acceleration_ms2 = acceleration_ms2;
                    step.lift_N = lift_to_weight * weight_N;
                    step.drag_N = drag_to_weight * weight_N;
                    step.rolling_resistance_N = rolling_to_weight * weight_N;
                    step.required_thrust_N = thrust_to_weight * weight_N;
                    step.required_total_shaft_power_W =
                        power_to_weight_W_N * weight_N;
                    step.deck_point = deck_point;
                    result.steps.push_back(step);
                }
            }

            result.integrated_ground_roll_m = distance_m;
            return result;
        };

        double lower_W_N = 0.0;
        double upper_W_N = 1.0;
        while (integrate(upper_W_N, false).integrated_ground_roll_m >
               input.takeoff.runway_m)
        {
            upper_W_N *= 2.0;
            if (upper_W_N > 1.0e5)
                throw std::runtime_error("Integrated propeller takeoff could not be bracketed.");
        }

        for (int iteration = 0; iteration < bisection_iterations; ++iteration)
        {
            const double trial_W_N = 0.5 * (lower_W_N + upper_W_N);
            if (integrate(trial_W_N, false).integrated_ground_roll_m >
                input.takeoff.runway_m)
                lower_W_N = trial_W_N;
            else
                upper_W_N = trial_W_N;
        }

        return integrate(upper_W_N, retain_steps);
    }

    constraint_curve propeller_constraint_analysis::compute_airborne_constraint(
        const constraint_input& input,
        const std::string& name,
        double altitude_m,
        double speed_ms,
        double beta,
        double load_factor,
        double climb_rate_ms,
        double acceleration_ms2) const
    {
        constraint_curve curve;
        curve.name = name;

        const double rho = atmosphere_.getDensity(altitude_m);
        const double q =
            constraint_utilities::compute_dynamic_pressure(rho, speed_ms);
        const auto operating_point = evaluate(
            input, altitude_m, speed_ms, input.propeller.continuous);

        for (double ws = input.wing_loading_min;
             ws <= input.wing_loading_max;
             ws += input.wing_loading_step)
        {
            mattingly_airborne_case_input airborne;
            airborne.wing_loading = ws;
            airborne.dynamic_pressure = q;
            airborne.alpha = 1.0;
            airborne.beta = beta;
            airborne.k1 = input.aircraft.polar.k;
            airborne.k2 = 0.0;
            airborne.cd0 = input.aircraft.polar.cd_0;
            airborne.cdr = 0.0;
            airborne.load_factor = load_factor;
            airborne.climb_rate = climb_rate_ms;
            airborne.velocity = speed_ms;
            airborne.acceleration = acceleration_ms2;

            const auto thrust_result = mattingly_airborne_case::compute(airborne);
            curve.points.push_back({
                ws,
                power_loading_from_thrust_loading(
                    thrust_result.thrust_to_weight_sl,
                    operating_point)
            });
        }

        return curve;
    }

    constraint_curve propeller_constraint_analysis::compute_acceleration_constraint(
        const constraint_input& input) const
    {
        return compute_airborne_constraint(
            input, "propeller_acceleration_constraint",
            input.acceleration.altitude_m, input.acceleration.speed_ms,
            input.acceleration.beta_acceleration, 1.0, 0.0,
            input.acceleration.acceleration_ms2);
    }

    constraint_curve propeller_constraint_analysis::compute_cruise_constraint(
        const constraint_input& input) const
    {
        return compute_airborne_constraint(
            input, "propeller_cruise_constraint",
            input.cruise.altitude_m, input.cruise.speed_ms,
            input.cruise.beta_cruise, 1.0, 0.0, 0.0);
    }

    constraint_curve propeller_constraint_analysis::compute_climb_constraint(
        const constraint_input& input) const
    {
        return compute_airborne_constraint(
            input, "propeller_climb_constraint",
            input.climb.altitude_m, input.climb.speed_ms,
            input.climb.beta_climb, 1.0, input.climb.roc_ms, 0.0);
    }

    constraint_curve propeller_constraint_analysis::compute_turn_constraint(
        const constraint_input& input) const
    {
        return compute_airborne_constraint(
            input, "propeller_turn_constraint",
            input.turn.altitude_m, input.turn.speed_ms,
            input.turn.beta_turn, input.turn.load_factor, 0.0, 0.0);
    }
}


#include <cmath>
#include <numbers>

// ============================================================
// merged from: src/mattingly_takeoff_ground_roll.cpp
// ============================================================
#include <cmath>
#include <stdexcept>

namespace constraint_analysis
{
    mattingly_takeoff_ground_roll_result mattingly_takeoff_ground_roll::compute(
        const mattingly_takeoff_ground_roll_input& input)
    {
        if (input.wing_loading <= 0.0)
        {
            throw std::runtime_error("Takeoff ground-roll error: wing loading must be positive.");
        }

        if (input.ground_roll_m <= 0.0)
        {
            throw std::runtime_error("Takeoff ground-roll error: ground-roll distance must be positive.");
        }

        if (input.density <= 0.0)
        {
            throw std::runtime_error("Takeoff ground-roll error: density must be positive.");
        }

        if (input.alpha <= 0.0)
        {
            throw std::runtime_error("Takeoff ground-roll error: alpha must be positive.");
        }

        if (input.beta <= 0.0)
        {
            throw std::runtime_error("Takeoff ground-roll error: beta must be positive.");
        }

        if (input.cl_max <= 0.0)
        {
            throw std::runtime_error("Takeoff ground-roll error: CLmax must be positive.");
        }

        if (input.k_to <= 0.0)
        {
            throw std::runtime_error("Takeoff ground-roll error: k_to must be positive.");
        }

        constexpr double g = 9.80665;

        mattingly_takeoff_ground_roll_result result;

        /*
         * Effective takeoff ground-roll coefficient:
         *
         * xi_TO = C_D + C_DR - mu_TO C_L
         *
         * This term appears because aerodynamic drag resists acceleration,
         * while lift reduces the normal force and therefore reduces rolling
         * friction.
         */
        result.xi_to =
            input.cd
            + input.cdr
            - input.mu * input.cl;

        const double ws = input.wing_loading;
        const double sg = input.ground_roll_m;
        const double rho = input.density;
        const double alpha = input.alpha;
        const double beta = input.beta;
        const double cl_max = input.cl_max;
        const double k_to = input.k_to;
        const double mu = input.mu;
        const double xi = result.xi_to;

        /*
         * Special limiting case:
         *
         * When xi_TO is very close to zero, the logarithmic form becomes
         * numerically ill-conditioned. In that limit, the equation reduces
         * to a simpler acceleration/friction expression.
         */
        if (std::abs(xi) < 1.0e-8)
        {
            const double acceleration_term =
                (beta * beta * k_to * k_to * ws)
                / (g * rho * cl_max * sg);

            result.thrust_to_weight_sl =
                (mu * beta + acceleration_term)
                / alpha;

            return result;
        }

        /*
         * Rearranged logarithmic takeoff ground-roll relation.
         *
         * This returns the required sea-level static thrust loading:
         *
         *      T_SL / W_TO
         *
         * as a function of:
         *      W_TO/S, S_G, rho, CLmax, k_TO, mu, alpha, beta, xi_TO
         */
        const double exponent =
            (sg * rho * g * xi)
            / (beta * ws);

        const double exp_term = std::exp(exponent);

        const double drag_friction_term =
            (xi * k_to * k_to * beta / cl_max)
            * (exp_term / (exp_term - 1.0));

        result.thrust_to_weight_sl =
            (mu * beta + drag_friction_term)
            / alpha;

        return result;
    }
}


// ============================================================
// merged from: src/mattingly_landing_braking_roll.cpp
// ============================================================
#include <stdexcept>

namespace constraint_analysis
{
    mattingly_landing_braking_roll_result mattingly_landing_braking_roll::compute(
        const mattingly_landing_braking_roll_input& input)
    {
        if (input.ground_roll_m <= 0.0)
        {
            throw std::runtime_error("Landing braking-roll error: ground-roll distance must be positive.");
        }

        if (input.density <= 0.0)
        {
            throw std::runtime_error("Landing braking-roll error: density must be positive.");
        }

        if (input.beta <= 0.0)
        {
            throw std::runtime_error("Landing braking-roll error: beta must be positive.");
        }

        if (input.cl_max <= 0.0)
        {
            throw std::runtime_error("Landing braking-roll error: CLmax must be positive.");
        }

        if (input.k_landing <= 0.0)
        {
            throw std::runtime_error("Landing braking-roll error: k_landing must be positive.");
        }

        constexpr double g = 9.80665;

        mattingly_landing_braking_roll_result result;

        /*
         * Mattingly Eq. 2.33 landing/braking-roll relation:
         *
         *              beta (W_TO/S)       /        xi_L        \
         *      S_B = ------------------- ln|1 + ----------------|
         *                rho g xi_L        \    mu CLmax/k_TD^2 /
         *
         *      xi_L = CD + CDR - mu CL
         *
         * Solving for takeoff wing loading gives the vertical
         * matching-chart constraint used below.
         */
        result.xi_landing =
            input.cd + input.cdr - input.mu * input.cl;

        const double braking_lift_term =
            input.mu * input.cl_max
            /
            (input.k_landing * input.k_landing);

        if (braking_lift_term <= 0.0)
        {
            throw std::runtime_error(
                "Landing braking-roll error: mu*CLmax/k_landing^2 must be positive.");
        }

        const double logarithm_argument =
            1.0 + result.xi_landing / braking_lift_term;

        if (logarithm_argument <= 0.0)
        {
            throw std::runtime_error(
                "Landing braking-roll error: logarithm argument must be positive.");
        }

        double xi_over_log = 0.0;
        constexpr double xi_tolerance = 1.0e-10;

        if (std::abs(result.xi_landing) < xi_tolerance)
        {
            // lim[xi -> 0] xi / ln(1 + xi/A) = A
            xi_over_log = braking_lift_term;
        }
        else
        {
            const double logarithm = std::log(logarithm_argument);

            if (std::abs(logarithm) < xi_tolerance)
            {
                throw std::runtime_error(
                    "Landing braking-roll error: logarithm is too close to zero.");
            }

            xi_over_log = result.xi_landing / logarithm;
        }

        result.wing_loading_limit =
            input.ground_roll_m
            * input.density
            * g
            * xi_over_log
            /
            input.beta;

        if (!std::isfinite(result.wing_loading_limit)
            || result.wing_loading_limit <= 0.0)
        {
            throw std::runtime_error(
                "Landing braking-roll error: wing-loading limit must be finite and positive.");
        }

        return result;
    }
}



// ============================================================
// UNICADO engine helper functions
// ============================================================
namespace constraint_analysis
{
    namespace
    {
        void require_engine(const constraint_input& input)
        {
            if (input.engine == nullptr)
            {
                throw std::runtime_error(
                    "UNICADO Engine is required. Run the program with an engine directory path.");
            }
        }

        double installed_thrust_lapse(
            const constraint_input& input,
            const atmosphere& atm,
            double mach,
            double altitude_m,
            const std::string& thrust_rating)
        {
            require_engine(input);

            return input.engine->get_thrust_lapse(
                thrust_rating,
                atm,
                mach,
                altitude_m);
        }

        double engine_tsfc_1_per_s(
            const constraint_input& input,
            double altitude_m,
            double mach)
        {
            require_engine(input);

            OperatingPoint op;
            op.N = 1.0;
            op.altitude = altitude_m;
            op.Mach = mach;

            input.engine->set_operating_point(op);

            // UNICADO Engine::get_tsfc() returns kg/(N*s).
            // The Breguet implementation below uses c in 1/s, so multiply by g0.
            constexpr double g0 = 9.80665;
            return input.engine->get_tsfc() * g0;
        }
    }
}

// ============================================================
// merged from: src/jet_constraint_analysis.cpp
// ============================================================
#include <cmath>

namespace constraint_analysis
{
    jet_constraint_analysis::jet_constraint_analysis(const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    constraint_curve jet_constraint_analysis::compute_takeoff_constraint(const constraint_input& input) const
    {
        constraint_curve curve;
        curve.name = "jet_takeoff_constraint";

        const double rho = atmosphere_.getDensity(input.takeoff.altitude_m);

        for (double ws = input.wing_loading_min;
            ws <= input.wing_loading_max;
            ws += input.wing_loading_step)
        {
            mattingly_takeoff_ground_roll_input case_input;

            case_input.wing_loading = ws;
            case_input.ground_roll_m = input.takeoff.runway_m;
            case_input.density = rho;

            const double mach_takeoff = 0.0;
            case_input.alpha = installed_thrust_lapse(
                input,
                atmosphere_,
                mach_takeoff,
                input.takeoff.altitude_m,
                "takeoff");
            case_input.beta = input.takeoff.beta_to;

            case_input.cl_max = input.aircraft.cl_max_takeoff;
            case_input.k_to = input.takeoff.speed_factor;

            case_input.cd = input.takeoff.cd_ground;
            case_input.cdr = 0.0;

            // Representative ground-roll lift coefficient.
            // For a first exact Case-6 implementation we use the traditional
            // ground-roll representative value before full CLmax is reached.
            case_input.cl = 0.8 * input.aircraft.cl_max_takeoff;

            case_input.mu = input.takeoff.mu_ro;

            const auto result =
                mattingly_takeoff_ground_roll::compute(case_input);

            curve.points.push_back({ws, result.thrust_to_weight_sl});
        }

        return curve;
    }

    vertical_constraint jet_constraint_analysis::compute_landing_constraint_limit(const constraint_input& input) const
    {
        vertical_constraint vc;
        vc.name = "jet_landing_limit";

        const double rho = atmosphere_.getDensity(input.landing.altitude_m);

        mattingly_landing_braking_roll_input case_input;

        case_input.ground_roll_m = input.landing.runway_m;
        case_input.density = rho;

        // Şimdilik landing için beta = 1.0 kabul ediyoruz.
        // Bir sonraki adımda input.landing.beta_landing ekleyip bunu mission weight fraction yapabiliriz.
        case_input.beta = input.landing.beta_landing;

        case_input.cl_max = input.aircraft.cl_max_landing;
        case_input.k_landing = input.landing.speed_factor;

        case_input.cd = input.landing.cd_brake;
        case_input.cdr = 0.0;

        // Braking sırasında lift genelde touchdown sonrası azalır.
        // Temsilî değer olarak 0.2*CLmax kullanıyoruz.
        // Eğer spoiler full deploy ise bu değer daha da düşük olabilir.
        case_input.cl = 0.2 * input.aircraft.cl_max_landing;

        case_input.mu = input.landing.mu_brake;

        const auto result =
            mattingly_landing_braking_roll::compute(case_input);

        vc.x_limit = result.wing_loading_limit;

        return vc;
    }


    vertical_constraint jet_constraint_analysis::compute_stall_speed_constraint_limit(const constraint_input& input) const
    {
        vertical_constraint vc;
        vc.name = "jet_stall_speed_limit";

        if (input.stall_speed.speed_limit_ms <= 0.0)
        {
            throw std::runtime_error("Stall speed limit error: speed_limit_ms must be positive.");
        }

        if (input.stall_speed.beta_stall <= 0.0)
        {
            throw std::runtime_error("Stall speed limit error: beta_stall must be positive.");
        }

        if (input.aircraft.cl_max_landing <= 0.0)
        {
            throw std::runtime_error("Stall speed limit error: landing CLmax must be positive.");
        }

        const double rho = atmosphere_.getDensity(input.stall_speed.altitude_m);
        const double v_stall_limit = input.stall_speed.speed_limit_ms;
        const double cl_max = input.aircraft.cl_max_landing;
        const double beta = input.stall_speed.beta_stall;

        /*
         * Stall-speed constraint:
         *
         *      V_stall = sqrt(2 W / (rho S CLmax))
         *
         * Because this matching chart uses takeoff wing loading W_TO/S,
         * while the stall condition may occur at a reduced mission weight,
         * W = beta * W_TO. Therefore:
         *
         *      beta * W_TO/S <= 0.5 rho V_stall_limit^2 CLmax
         *
         * and the vertical limit is:
         *
         *      W_TO/S <= 0.5 rho V_stall_limit^2 CLmax / beta
         */
        vc.x_limit =
            0.5 * rho * v_stall_limit * v_stall_limit * cl_max / beta;

        return vc;
    }



    namespace
    {
        constexpr double g0_ms2 = 9.80665;
        constexpr double ft_per_m = 3.280839895;

        double automatic_transport_gust_load_factor_limit()
        {
            // Default positive limit maneuver load factor for a transport-category sizing case.
            return 2.5;
        }

        double automatic_design_gust_velocity_ms(double altitude_m)
        {
            /*
             * Default discrete-gust velocity model for preliminary sizing.
             * 50 ft/s is used up to 20,000 ft, linearly reducing to 25 ft/s at 50,000 ft.
             * This keeps the input automatic while preserving the usual CS/FAR-25-style trend.
             */
            const double altitude_ft = altitude_m * ft_per_m;
            const double u_low_ms = 50.0 * 0.3048;
            const double u_high_ms = 25.0 * 0.3048;

            if (altitude_ft <= 20000.0)
            {
                return u_low_ms;
            }

            if (altitude_ft >= 50000.0)
            {
                return u_high_ms;
            }

            const double fraction = (altitude_ft - 20000.0) / (50000.0 - 20000.0);
            return u_low_ms + fraction * (u_high_ms - u_low_ms);
        }

        double automatic_effective_aspect_ratio(double aspect_ratio, double induced_drag_factor)
        {
            /*
             * Prefer the aspect ratio provided by the aircraft/aerodynamics data.
             * The current aerodynamics XML path may not provide AR, but it does
             * provide the induced drag factor k. In that case, estimate AR from
             *     k = 1 / (pi * e * AR)
             * using a typical preliminary Oswald efficiency e = 0.85. This avoids
             * requiring another manual gust input while keeping the calculation
             * tied to the aerodynamic polar.
             */
            if (aspect_ratio > 0.0)
            {
                return aspect_ratio;
            }

            constexpr double default_oswald_efficiency = 0.85;
            if (induced_drag_factor > 0.0)
            {
                return 1.0 / (std::numbers::pi * default_oswald_efficiency * induced_drag_factor);
            }

            // Last-resort preliminary transport-aircraft fallback.
            return 9.0;
        }

        double automatic_lift_curve_slope_per_rad(double aspect_ratio)
        {
            /*
             * Finite-wing estimate, unswept preliminary form:
             *     a = 2*pi*AR/(AR + 2)
             */
            return 2.0 * std::numbers::pi * aspect_ratio / (aspect_ratio + 2.0);
        }

        double automatic_mean_aerodynamic_chord_m(double wing_area_m2, double aspect_ratio)
        {
            /*
             * Equivalent rectangular wing estimate:
             *     b = sqrt(AR*S), c_bar = S/b = sqrt(S/AR)
             */
            if (wing_area_m2 <= 0.0 || aspect_ratio <= 0.0)
            {
                throw std::runtime_error(
                    "Gust limit error: wing area and aspect ratio are required "
                    "for automatic gust alleviation factor calculation.");
            }

            return std::sqrt(wing_area_m2 / aspect_ratio);
        }

        double automatic_gust_alleviation_factor(
            double actual_wing_loading,
            double rho,
            double mean_aerodynamic_chord_m,
            double lift_curve_slope_per_rad)
        {
            const double mu_g =
                2.0 * actual_wing_loading
                / (rho * mean_aerodynamic_chord_m * lift_curve_slope_per_rad * g0_ms2);

            return 0.88 * mu_g / (5.3 + mu_g);
        }
    }

    vertical_constraint jet_constraint_analysis::compute_gust_constraint_limit(const constraint_input& input) const
    {
        vertical_constraint vc;
        vc.name = "jet_gust_limit";
        vc.is_upper_limit = false;

        if (input.gust.speed_ms <= 0.0)
        {
            throw std::runtime_error("Gust limit error: speed_ms must be positive.");
        }

        if (input.gust.beta_gust <= 0.0)
        {
            throw std::runtime_error("Gust limit error: beta_gust must be positive.");
        }

        const double rho = atmosphere_.getDensity(input.gust.altitude_m);
        const double v = input.gust.speed_ms;
        const double beta = input.gust.beta_gust;

        const double u_de = automatic_design_gust_velocity_ms(input.gust.altitude_m);
        const double effective_aspect_ratio = automatic_effective_aspect_ratio(
            input.aircraft.aspect_ratio,
            input.aircraft.polar.k);
        const double a = automatic_lift_curve_slope_per_rad(effective_aspect_ratio);
        const double n_limit = automatic_transport_gust_load_factor_limit();
        if (input.aircraft.takeoff_weight_N <= 0.0)
        {
            throw std::runtime_error("Gust limit error: takeoff weight must be positive.");
        }

        if (rho <= 0.0 || u_de <= 0.0 || a <= 0.0 || n_limit <= 1.0)
        {
            throw std::runtime_error("Gust limit error: automatic gust parameters are invalid.");
        }

        /*
         * Discrete-gust load increment:
         *
         *      Delta n = K_g rho V a U_de / (2 W/S_actual)
         *
         * The chart x-axis is takeoff wing loading x = W_TO/S, while the
         * gust condition occurs at W/S_actual = beta*x.
         * K_g also depends on W/S_actual through the mass ratio, so the limit
         * is solved as a scalar root instead of using a fixed K_g value.
         */
        const auto delta_n = [&](double takeoff_wing_loading) -> double
        {
            const double actual_wing_loading = beta * takeoff_wing_loading;

            // For every candidate W/S, calculate its corresponding clean-sheet wing area:
            //     S = W_TO / (W_TO/S)
            // This keeps wing area an output of sizing rather than a polar-file input.
            const double candidate_wing_area_m2 =
                input.aircraft.takeoff_weight_N / takeoff_wing_loading;
            const double mean_chord = automatic_mean_aerodynamic_chord_m(
                candidate_wing_area_m2,
                effective_aspect_ratio);

            const double k_g = automatic_gust_alleviation_factor(
                actual_wing_loading,
                rho,
                mean_chord,
                a);

            return k_g * rho * v * a * u_de / (2.0 * actual_wing_loading);
        };

        const double target_delta_n = n_limit - 1.0;

        double lower = 1.0;
        double upper = 100000.0;

        while (delta_n(lower) < target_delta_n && lower > 1.0e-6)
        {
            lower *= 0.5;
        }

        while (delta_n(upper) > target_delta_n)
        {
            upper *= 2.0;

            if (upper > 1.0e8)
            {
                throw std::runtime_error("Gust limit error: could not bracket W/S limit.");
            }
        }

        for (int iteration = 0; iteration < 80; ++iteration)
        {
            const double mid = 0.5 * (lower + upper);

            if (delta_n(mid) > target_delta_n)
            {
                lower = mid;
            }
            else
            {
                upper = mid;
            }
        }

        vc.x_limit = 0.5 * (lower + upper);
        return vc;
    }


    constraint_curve jet_constraint_analysis::compute_max_mach_constraint(const constraint_input& input) const
    {
        constraint_curve curve;
        curve.name = "jet_max_mach_constraint";

        const double h = input.max_mach.altitude_m;
        const double mach = input.max_mach.mach;
        const double beta = input.max_mach.beta_max_mach;

        const double rho = atmosphere_.getDensity(h);
        const double a = atmosphere_.getSpeedOfSound(h);
        const double v = mach * a;
        const double q = constraint_utilities::compute_dynamic_pressure(rho, v);

        const double alpha = installed_thrust_lapse(
            input,
            atmosphere_,
            mach,
            h,
            "maximum_continuous");

        for (double ws = input.wing_loading_min;
            ws <= input.wing_loading_max;
            ws += input.wing_loading_step)
        {
            mattingly_airborne_case_input case_input;
            case_input.wing_loading = ws;
            case_input.dynamic_pressure = q;
            case_input.alpha = alpha;
            case_input.beta = beta;

            case_input.k1 = input.aircraft.polar.k;
            case_input.k2 = 0.0;
            case_input.cd0 = input.aircraft.polar.cd_0;
            case_input.cdr = 0.0;

            case_input.load_factor = 1.0;
            case_input.climb_rate = 0.0;
            case_input.velocity = v;
            case_input.acceleration = 0.0;

            const auto result = mattingly_airborne_case::compute(case_input);

            curve.points.push_back({ws, result.thrust_to_weight_sl});
        }

        return curve;
    }

    constraint_curve jet_constraint_analysis::compute_acceleration_constraint(const constraint_input& input) const
    {
        constraint_curve curve;
        curve.name = "jet_acceleration_constraint";

        const double h = input.acceleration.altitude_m;
        const double v = input.acceleration.speed_ms;
        const double acceleration = input.acceleration.acceleration_ms2;
        const double beta = input.acceleration.beta_acceleration;

        const double rho = atmosphere_.getDensity(h);
        const double q = constraint_utilities::compute_dynamic_pressure(rho, v);
        const double mach = v / atmosphere_.getSpeedOfSound(h);

        const double alpha = installed_thrust_lapse(
            input,
            atmosphere_,
            mach,
            h,
            "maximum_continuous");

        for (double ws = input.wing_loading_min;
            ws <= input.wing_loading_max;
            ws += input.wing_loading_step)
        {
            mattingly_airborne_case_input case_input;
            case_input.wing_loading = ws;
            case_input.dynamic_pressure = q;
            case_input.alpha = alpha;
            case_input.beta = beta;

            case_input.k1 = input.aircraft.polar.k;
            case_input.k2 = 0.0;
            case_input.cd0 = input.aircraft.polar.cd_0;
            case_input.cdr = 0.0;

            case_input.load_factor = 1.0;
            case_input.climb_rate = 0.0;
            case_input.velocity = v;
            case_input.acceleration = acceleration;

            const auto result = mattingly_airborne_case::compute(case_input);

            curve.points.push_back({ws, result.thrust_to_weight_sl});
        }

        return curve;
    }

    constraint_curve jet_constraint_analysis::compute_cruise_constraint(const constraint_input& input) const
    {
        constraint_curve curve;
        curve.name = "jet_cruise_constraint";

        const double h = input.cruise.altitude_m;
        const double v = input.cruise.speed_ms;
        const double beta = input.cruise.beta_cruise;

        const double rho = atmosphere_.getDensity(h);
        const double q = constraint_utilities::compute_dynamic_pressure(rho, v);
        const double mach = v / atmosphere_.getSpeedOfSound(h);

        const double alpha = installed_thrust_lapse(
            input,
            atmosphere_,
            mach,
            h,
            "maximum_continuous");

        for (double ws = input.wing_loading_min;
            ws <= input.wing_loading_max;
            ws += input.wing_loading_step)
        {
            mattingly_airborne_case_input case_input;
            case_input.wing_loading = ws;
            case_input.dynamic_pressure = q;
            case_input.alpha = alpha;
            case_input.beta = beta;

            case_input.k1 = input.aircraft.polar.k;
            case_input.k2 = 0.0;
            case_input.cd0 = input.aircraft.polar.cd_0;
            case_input.cdr = 0.0;

            case_input.load_factor = 1.0;
            case_input.climb_rate = 0.0;
            case_input.velocity = v;
            case_input.acceleration = 0.0;

            const auto result = mattingly_airborne_case::compute(case_input);

            curve.points.push_back({ws, result.thrust_to_weight_sl});
        }

        return curve;
    }

    constraint_curve jet_constraint_analysis::compute_climb_constraint(const constraint_input& input) const
    {
        constraint_curve curve;
        curve.name = "jet_climb_constraint";

        const double h = input.climb.altitude_m;
        const double v = input.climb.speed_ms;
        const double roc = input.climb.roc_ms;
        const double beta = input.climb.beta_climb;

        const double rho = atmosphere_.getDensity(h);
        const double q = constraint_utilities::compute_dynamic_pressure(rho, v);
        const double mach = v / atmosphere_.getSpeedOfSound(h);

        const double alpha = installed_thrust_lapse(
            input,
            atmosphere_,
            mach,
            h,
            "maximum_continuous");

        for (double ws = input.wing_loading_min;
            ws <= input.wing_loading_max;
            ws += input.wing_loading_step)
        {
            mattingly_airborne_case_input case_input;
            case_input.wing_loading = ws;
            case_input.dynamic_pressure = q;
            case_input.alpha = alpha;
            case_input.beta = beta;

            case_input.k1 = input.aircraft.polar.k;
            case_input.k2 = 0.0;
            case_input.cd0 = input.aircraft.polar.cd_0;
            case_input.cdr = 0.0;

            case_input.load_factor = 1.0;
            case_input.climb_rate = roc;
            case_input.velocity = v;
            case_input.acceleration = 0.0;

            const auto result = mattingly_airborne_case::compute(case_input);

            curve.points.push_back({ws, result.thrust_to_weight_sl});
        }

        return curve;
    }

    constraint_curve jet_constraint_analysis::compute_turn_constraint(const constraint_input& input) const
    {
        constraint_curve curve;
        curve.name = "jet_turn_constraint";

        const double h = input.turn.altitude_m;
        const double v = input.turn.speed_ms;
        const double n = input.turn.load_factor;
        const double beta = input.turn.beta_turn;

        const double rho = atmosphere_.getDensity(h);
        const double q = constraint_utilities::compute_dynamic_pressure(rho, v);
        const double mach = v / atmosphere_.getSpeedOfSound(h);

        const double alpha = installed_thrust_lapse(
            input,
            atmosphere_,
            mach,
            h,
            "maximum_continuous");

        for (double ws = input.wing_loading_min;
            ws <= input.wing_loading_max;
            ws += input.wing_loading_step)
        {
            mattingly_airborne_case_input case_input;
            case_input.wing_loading = ws;
            case_input.dynamic_pressure = q;
            case_input.alpha = alpha;
            case_input.beta = beta;

            case_input.k1 = input.aircraft.polar.k;
            case_input.k2 = 0.0;
            case_input.cd0 = input.aircraft.polar.cd_0;
            case_input.cdr = 0.0;

            case_input.load_factor = n;
            case_input.climb_rate = 0.0;
            case_input.velocity = v;
            case_input.acceleration = 0.0;

            const auto result = mattingly_airborne_case::compute(case_input);

            curve.points.push_back({ws, result.thrust_to_weight_sl});
        }

        return curve;
    }

    constraint_curve jet_constraint_analysis::compute_supercruise_constraint(const constraint_input& input) const
    {
        constraint_curve curve;
        curve.name = "jet_supercruise_constraint";

        const double h = input.supercruise.altitude_m;
        const double mach = input.supercruise.mach;
        const double beta = input.supercruise.beta_supercruise;

        const double rho = atmosphere_.getDensity(h);
        const double a = atmosphere_.getSpeedOfSound(h);
        const double v = mach * a;
        const double q = constraint_utilities::compute_dynamic_pressure(rho, v);

        const double alpha = installed_thrust_lapse(
            input,
            atmosphere_,
            mach,
            h,
            "maximum_continuous");

        for (double ws = input.wing_loading_min;
            ws <= input.wing_loading_max;
            ws += input.wing_loading_step)
        {
            mattingly_airborne_case_input case_input;
            case_input.wing_loading = ws;
            case_input.dynamic_pressure = q;
            case_input.alpha = alpha;
            case_input.beta = beta;

            case_input.k1 = input.aircraft.polar.k;
            case_input.k2 = 0.0;
            case_input.cd0 = input.aircraft.polar.cd_0;
            case_input.cdr = 0.0;

            case_input.load_factor = 1.0;
            case_input.climb_rate = 0.0;
            case_input.velocity = v;
            case_input.acceleration = 0.0;

            const auto result = mattingly_airborne_case::compute(case_input);

            curve.points.push_back({ws, result.thrust_to_weight_sl});
        }

        return curve;
    }

}


// ============================================================
// merged from: src/range_constraint_analysis.cpp
// ============================================================
#include <cmath>

namespace constraint_analysis
{
    range_constraint_analysis::range_constraint_analysis(const atmosphere& atmosphere)
        : atmosphere_(atmosphere)
    {
    }

    range_constraint_result range_constraint_analysis::compute_range_constraint(const constraint_input& input) const
    {
        range_constraint_result result;
        result.name = "jet_range_fuel_fraction_constraint";
        result.available_fuel_fraction = input.range.available_fuel_fraction;

        const double rho = atmosphere_.getDensity(input.range.altitude_m);
        const double v = input.range.speed_ms;
        const double q = constraint_utilities::compute_dynamic_pressure(rho, v);

        const double range_m = input.range.range_m;
        const double mach = v / atmosphere_.getSpeedOfSound(input.range.altitude_m);
        const double c = engine_tsfc_1_per_s(
            input,
            input.range.altitude_m,
            mach);
        const double beta_start = input.range.beta_start;

        for (double ws = input.wing_loading_min; ws <= input.wing_loading_max; ws += input.wing_loading_step)
        {
            const double cl = constraint_utilities::compute_lift_coefficient(ws, q);
            const double cd = constraint_utilities::compute_drag_coefficient(
                input.aircraft.polar.cd_0,
                input.aircraft.polar.k,
                cl);

            const double lift_to_drag = cl / cd;

            mattingly_jet_cruise_segment_input cruise_segment;
            cruise_segment.range_m = range_m;
            cruise_segment.velocity_ms = v;
            cruise_segment.tsfc_1_per_s = c;
            cruise_segment.lift_to_drag = lift_to_drag;

            const double cruise_fraction =
                mattingly_jet_mission_segment::cruise_weight_fraction(cruise_segment);

            const double beta_end =
                beta_start * cruise_fraction;

            const double required_fuel_fraction =
                beta_start - beta_end;

            const bool feasible =
                required_fuel_fraction <= input.range.available_fuel_fraction;

            result.points.push_back({
                ws,
                lift_to_drag,
                required_fuel_fraction,
                feasible
            });
        }

        return result;
    }
}
