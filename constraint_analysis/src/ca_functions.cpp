#include "constraint_analysis/ca_functions.h"


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
         * Effective landing/braking deceleration coefficient:
         *
         * xi_L =
         *     mu
         *   - mu * (CL / CLmax)
         *   + (CD + CDR) / CLmax
         *
         * Interpretation:
         * - braking friction provides the main stopping force
         * - residual lift reduces normal force and therefore braking friction
         * - aerodynamic drag contributes additional deceleration
         */
        result.xi_landing =
            input.mu
            - input.mu * (input.cl / input.cl_max)
            + (input.cd + input.cdr) / input.cl_max;

        if (result.xi_landing <= 0.0)
        {
            throw std::runtime_error("Landing braking-roll error: xi_landing must be positive.");
        }

        /*
         * Rearranged landing/braking ground-roll relation:
         *
         *      S_L =
         *      beta^2 k_L^2 (W_TO/S)
         *      /
         *      (rho g CLmax xi_L)
         *
         * Therefore:
         *
         *      W_TO/S =
         *      S_L rho g CLmax xi_L
         *      /
         *      (beta^2 k_L^2)
         *
         * The result is a vertical constraint in the W/S - T/W diagram.
         */
        result.wing_loading_limit =
            (input.ground_roll_m
             * input.density
             * g
             * input.cl_max
             * result.xi_landing)
            /
            (input.beta
             * input.beta
             * input.k_landing
             * input.k_landing);

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
        const double mean_chord = automatic_mean_aerodynamic_chord_m(
            input.aircraft.wing_area_m2,
            effective_aspect_ratio);

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
