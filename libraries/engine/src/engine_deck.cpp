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

#include "engine/engine_deck.h"
#include <algorithm>
#include <boost/multi_array.hpp>
#include <cmath>
#include <ranges>
#include <runtimeInfo/runtimeInfo.h>
#include <standardFiles/functions.h>
#include <unitConversion/constants.h>

/* === Functions === */
auto DeckValue::is_within_range(const OperatingPoint &operating_point) const -> bool
{
    /* Check the three components */
    const bool N_within_range = operating_point.N >= data_.N.back() && operating_point.N <= data_.N.front();
    if(!N_within_range) {
        std::ostringstream oss;
        oss << "The N1 is out of range. N1: " << operating_point.N;
        throw std::out_of_range(oss.str());
    }
    const bool Mach_within_range = operating_point.Mach >= data_.Mach.front() && operating_point.Mach <= data_.Mach.back();
    if(!Mach_within_range) {
        throw std::out_of_range("The mach number is out of range.");
    }
    const bool FL_within_range = operating_point.altitude >= data_.FL.front() && operating_point.altitude <= data_.FL.back();
    if(!FL_within_range) {
        throw std::out_of_range("The FL is out of range.");
    }
    /* Return the result */
    return (N_within_range && Mach_within_range && FL_within_range);

}

auto DeckValue::get_value_at(const OperatingPoint &operating_point) const -> double
{
    /* Check whether input is within the range of the deck value */
    if (!this->is_within_range(operating_point))
    {
        throw std::out_of_range("The operating point is out of range.");
    }

    /* Get the iterators of the data closest to the given operating point */
    const auto N_bound = std::ranges::lower_bound(data_.N, operating_point.N, std::greater{}); // The N values are in reverse order
    const auto N1 = N_bound == data_.N.cbegin() ? N_bound + 1 : N_bound;
    const auto N0 = N1 - 1;
    const auto Mach_bound = std::ranges::lower_bound(data_.Mach, operating_point.Mach);
    const auto Mach1 = Mach_bound == data_.Mach.cbegin() ? Mach_bound + 1 : Mach_bound;
    const auto Mach0 = Mach1 - 1;
    const auto FL_bound = std::ranges::lower_bound(data_.FL, operating_point.altitude);
    const auto FL1 = FL_bound == data_.FL.cbegin() ? FL_bound + 1 : FL_bound;
    const auto FL0 = FL1 - 1;

    /* Get the indices of the data in the value array */
    const auto index_N0 = std::distance(data_.N.cbegin(), N0);
    const auto index_N1 = index_N0 + 1;
    const auto index_Mach0 = std::distance(data_.Mach.cbegin(), Mach0);
    const auto index_Mach1 = index_Mach0 + 1;
    const auto index_FL0 = std::distance(data_.FL.cbegin(), FL0);
    const auto index_FL1 = index_FL0 + 1;

    /* Get the interpolation factors */
    const auto factor_N = (operating_point.N - *N0) / (*N1 - *N0);
    const auto factor_Mach = (operating_point.Mach - *Mach0) / (*Mach1 - *Mach0);
    const auto factor_FL = (operating_point.altitude - *FL0) / (*FL1 - *FL0);

    /* Get the corner values for the interpolation from the value data
     * => The index_range is a half-open range, meaning the last index
     * is not included in the result, therefore the range end is n+1.
     */
    using range = boost::multi_array_types::index_range;
    const auto boundary_indices =
        boost::indices[range(index_N0, index_N1 + 1)]
                      [range(index_FL0, index_FL1 + 1)]
                      [range(index_Mach0, index_Mach1 + 1)];
    const auto corner_points = data_.values[boundary_indices];

    /* === Trilinear interpolation === */
    /* Interpolate the N dimension */
    boost::multi_array<double, 2> N_interpolated(boost::extents[2][2]);
    for (std::size_t i = 0; i < 2; ++i)
    {
        for (std::size_t j = 0; j < 2; ++j)
        {
            N_interpolated[i][j] = corner_points[0][i][j] * (1 - factor_N) + corner_points[1][i][j] * factor_N;
        }
    }

    /* Interpolate the FL dimension */
    boost::multi_array<double, 1> FL_interpolated(boost::extents[2]);
    for (std::size_t i = 0; i < 2; ++i)
    {
        FL_interpolated[i] = N_interpolated[0][i] * (1 - factor_FL) + N_interpolated[1][i] * factor_FL;
    }

    /* Interpolate the Mach dimension */
    const auto value = FL_interpolated[0] * (1 - factor_Mach) + FL_interpolated[1] * factor_Mach;

    /* Return the result */
    return value;
}

auto DeckValue::lower_boundary() const -> OperatingPoint
{
    OperatingPoint minimal_op{};

    minimal_op.N = data_.N.back();
    minimal_op.Mach = data_.Mach.front();
    minimal_op.altitude = data_.FL.front();

    return minimal_op;
}

auto DeckValue::upper_boundary() const -> OperatingPoint
{
    OperatingPoint maximal_op{};

    maximal_op.N = data_.N.front();
    maximal_op.Mach = data_.Mach.back();
    maximal_op.altitude = data_.FL.back();

    return maximal_op;
}

