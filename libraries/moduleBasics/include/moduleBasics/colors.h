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

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_COLORS_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_COLORS_H_

#include <array>
#include <queue>
#include <tuple>
#include <vector>
#include <algorithm>

namespace colors {

/* Alias for rgb color */
using rgb_color = std::tuple<int, int, int>;

/* Helper method to create RGB colors */
constexpr rgb_color make_color(int r, int g, int b) {
  return std::make_tuple(r, g, b);
}

/* Predefined colors */
constexpr rgb_color tub_red = make_color(197, 14, 30);
constexpr rgb_color tubs_red = make_color(189, 30, 60);
constexpr rgb_color tuhh_turquoise = make_color(0, 193, 212);
constexpr rgb_color rwth_blue = make_color(1, 81, 157);
constexpr rgb_color tum_blue = make_color(0, 101, 189);
constexpr rgb_color ustutt_black = make_color(0, 0, 0);
constexpr rgb_color additional_orange = make_color(255, 128, 0);
constexpr rgb_color additional_green = make_color(0, 153, 0);
constexpr rgb_color additional_purple = make_color(153, 0, 153);

/* Unicado color palette */
constexpr std::array<rgb_color, 9> unicado_color_range = {ustutt_black, tum_blue, tub_red, tuhh_turquoise, tubs_red, rwth_blue,
                                                            additional_orange, additional_green, additional_purple};

static size_t color_index = 0;
/**
 * Next color - automatic color incrementation ordered by unicado logo
 *
 * @return  rgb_color[return description]
 */
inline const rgb_color& next_color() {
  const rgb_color& color = unicado_color_range[color_index];
  ++color_index;
  color_index %= unicado_color_range.size();
  return color;
}

inline void reset_next_color() {
  color_index = 0;
}

/**
 * Blend two rgb colors by blending factor (default blending 0.5)
 *
 * @return  blended rgb_color
 */
inline const rgb_color blend(const rgb_color& c1, const rgb_color& c2, const double blending_factor = 0.5) {
  return make_color((std::get<0>(c1) * blending_factor + std::get<0>(c2) * (1 - blending_factor)), (std::get<1>(c1) * blending_factor + std::get<1>(c2) * (1 - blending_factor)),
                    (std::get<2>(c1) * blending_factor + std::get<2>(c2) * (1 - blending_factor)));
}

/**
 * Adjust brightness of color
 *
 * @param   double      factor  Brightness factor
 *
 * @return  rgb_color          brightness adjusted color
 */
inline const rgb_color adjust_brightness(const rgb_color& color, double factor) {
  return make_color(std::min(255, static_cast<int>(std::get<0>(color) * factor)), std::min(255, static_cast<int>(std::get<1>(color) * factor)),
                    std::min(255, static_cast<int>(std::get<2>(color) * factor)));
}


/**
 * [array description]
 * @param rgb_color color as an rgb_color element (tuple with 3 ints)
 * @return  std::array<float, 4> matplot++ color array
 */
inline std::array<float, 4> to_matplot_color(const rgb_color& color, float visibility = 0.0f) {
  return {static_cast<float>(std::max(0.f, std::min(1.f, visibility))),
          static_cast<float>(std::get<0>(color) / 255.0f),
          static_cast<float>(std::get<1>(color) / 255.0f),
          static_cast<float>(std::get<2>(color) / 255.0f)};
}

/**
 * Transforms unicado color order into 2D vector for matplot++ colororder command
 *
 * @param   void  void  none
 *
 * @return  std::vector<std::vector<double>> colororder
 */
inline std::vector<std::vector<double>> unicado_to_matplot_colororder(void) {
  std::vector<std::vector<double>> colorvector;
  for (auto& color_palette : unicado_color_range) {
    const auto [r, g, b] = color_palette;
    colorvector.push_back({static_cast<double>(r)/255., static_cast<double>(g)/255., static_cast<double>(b)/255.});
  }
  return colorvector;
}


} // namespace colors

#endif // MODULEBASICS_INCLUDE_MODULEBASICS_COLORS_H_
