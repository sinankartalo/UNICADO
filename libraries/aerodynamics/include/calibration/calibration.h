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

#ifndef AERODYNAMICS_CALIBRATION_H_
#define AERODYNAMICS_CALIBRATION_H_

#include "interpolation/interpolation_v2.h"
#include "interpolation/data_types.h"
#include "solver/solver_v2.h"
#include "aerodynamics/aerodynamics_v3.h"
#include <sstream>
#include <string>
#include <cstdint>
#include <vector>
#include <atmosphere/atmosphere.h>
#include <variant>  
#include <aixml/node.h>
#include <aixml/endnode.h>

namespace aerodynamics
{
	class Tuning
	{
		Tuning() = default;

		std::shared_ptr<node> tuningXML = {};
	};

	class Calibration
	{
	public:
		Calibration() = default;

		Calibration(Aircraft aircraft, Aircraft ref_aircraft)
		{};

		Calibration(Aircraft aircraft, Tuning tuning_parameters)
		{};

	};
}

#endif