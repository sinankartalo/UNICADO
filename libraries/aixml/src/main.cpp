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

#include "../aixml/node.h"

int main() {
    try {
        node doc = node::openDocument("A320.xml");
        node aixml = doc.at("AcftExchangeFile");
        std::vector<node> liftingSurfaces = aixml.getVector("Geometry/LiftingSurface");
        std::vector<double> r_surface_double = aixml.getDoubleVector("Geometry/LiftingSurface/r_Surface", 2);
        std::vector<bool> r_surface_bool = aixml.getBoolVector("Geometry/LiftingSurface/SurfaceRefPoint/r_Surface");
        std::vector<int> r_surface_int = aixml.getIntVector("Geometry/LiftingSurface/SurfaceRefPoint/r_Surface");

        for (double d : r_surface_double) {
            std::cout << d << std::endl;
        }
        for (int i : r_surface_int) {
            std::cout << i << std::endl;
        }
        for (bool b : r_surface_bool) {
            std::cout << b << std::endl;
        }

        if (node::saveDocument(doc)) {
            std::cout << "yay" << std::endl;
        }
    }
    catch(const std::string& s) {
        std::cout << s << std::endl;
    }
    catch(const char* s) {
        std::cout << s << std::endl;
    }
}
