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

#include "aircraftGeometry2/geometry/factory.h"
#include <memory>
#include "aircraftGeometry2/geometry/acxml.h"

/* === Functions === */
namespace geom2
{
    /* === Factory === */
    template<typename Surface>
    Factory<Surface>::Factory(std::shared_ptr<node> input, const std::filesystem::path& data_dir)
    {
        /*
         * Check whether the old version 2.0 node exists and
         * and select the surface builder accordingly.
         */
        node* root = input->find("AcftExchangeFile");
        if (root != nullptr)
        {
            /* -> ACXML Version 2.0 */
            this->builder = std::make_unique<AIXMLv2>(input, data_dir);
        } else {
            /* -> ACXML Version 3.0 */
            this->builder = std::make_unique<AIXMLv3>(input, data_dir);
        }
    }

    /* Valid Factory specializations */
    template class Factory<MultisectionSurface<PolygonSection>>;
    template class Factory<MultisectionSurface<AirfoilSection>>;
    // template class Factory<ControlDeviceContainer>;
}; // namespace geom2
