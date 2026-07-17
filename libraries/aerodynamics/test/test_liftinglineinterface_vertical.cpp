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
#include "interfaces/lifting_line/lifting_line_interface.h"
#include "interfaces/lifting_line/lifting_line_geometry_interface.h"
#include <random>

using Wing = geom2::MultisectionSurface<geom2::AirfoilSection>;

TEST(LiftingLineVetrical, LiftingLineTestVertical)
{
    std::filesystem::path stubs_path{ CMAKE_TEST_STUBS_DIR };
    
    auto acXML_path = stubs_path / "acXML/test_AC_vertical.xml";
    auto aircraft_xml = aixml::openDocument(acXML_path);

    auto aircraft_geometry = lifting_line::create_aircraft_geometry(aircraft_xml, stubs_path/"airfoils");

    auto conf_path = stubs_path / "LiftingLine/lili_conf.xml";
    auto lili_conf = aixml::openDocument(conf_path);

    auto exe_path = stubs_path / "LiftingLine";

    std::vector<Wing> lifting_surfaces = aircraft_geometry->wings;
    lifting_surfaces.insert(lifting_surfaces.end(), aircraft_geometry->empennage.begin(), aircraft_geometry->empennage.end());

    for (auto lifting_surface : lifting_surfaces)
    {
        lifting_surface.is_symmetric = true;
        lifting_line::exe_interface::create_input_for_lifting_surface(aircraft_xml, std::make_shared<Wing>(lifting_surface), exe_path, lili_conf->at("case"));
        std::string file_name = lifting_surface.name;
        lifting_line::exe_interface::exec_lifting_line(file_name, exe_path.generic_string(), "LIFTING_LINE_WINDOWS_64BIT");
        lifting_surface.is_symmetric = false;
    }
}