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

#include "aircraftGeometry2/airfoil_surface.h"
#include "aircraftGeometry2/fuselage.h"
#include "aircraftGeometry2/hull_surface.h"
#include "aircraftGeometry2/io/convert.h"
#include "aircraftGeometry2/processing/transform.h"
#include <CGAL/Surface_mesh/IO/PLY.h>
#include <aixml/node.h>

/* === Main === */
int main(int argc, char **argv)
{
    using namespace geom2;
    /* Check whether the path to the XML is given in the arguments */
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << "<mode> <path to XML file>" << std::endl;
        return EXIT_FAILURE;
    }

    /* Read the XML file */
    std::filesystem::path xml_file{argv[2]};
    auto AcXml = aixml::openDocument(xml_file);

    /* Print XML name */
    std::cout << "XML file: " << AcXml->name << std::endl;

    /* Print the mode */
    std::cout << "Mode: " << argv[1] << std::endl;

    try
    {
        /* Setup the factories */
        geom2::AirfoilSurfaceFactory surface_factory(AcXml, xml_file.parent_path());
        geom2::WingFactory wing(AcXml, xml_file.parent_path());
        geom2::HullFactory hull_factory(AcXml, xml_file.parent_path());
        geom2::FuselageFactory fuselage(AcXml, xml_file.parent_path());

        /* Check the execution mode */
        std::string mode{argv[1]};
        if (mode == "convert")
        {
            /* Create a wing */
            auto wing_surface = wing.create("LiftingSurface@MainWing");
            wing_surface.normal = Direction_3(0, -1, 0);

            /* Create a fin */
            auto fin_surface = surface_factory.create("VerticalSurface@Fin");
            fin_surface.normal = Direction_3(0, 0, -1);

            /* Create Fuselage */
            auto fuselage_surface = fuselage.create("Fuselage@Fuselage");
            fuselage_surface.normal = Direction_3(1, 0, 0);

            /* Create Pylon */
            auto pylon_surface = surface_factory.create("Pylon@InnerRightWingPylon");
            pylon_surface.normal = Direction_3(0, 0, -1);

            /* Create Nacelle */
            auto nacelle_surface = hull_factory.create("Nacelle@InnerRightWingNacelle");
            nacelle_surface.normal = Direction_3(1, 0, 0);

            /* Create a spar */
            // geom2::SparFactory spar(AcXml, xml_file.parent_path());
            // auto spar_surface = spar.create("LiftingSurface@MainWing");
            // spar_surface.normal = Direction_3(0, -1, 0);
            // mesh = geom2::transform::to_mesh(spar_surface);
            // CGAL::IO::write_PLY("./spar.ply", mesh);

            /* Export the geometry to the new XMl format */
            auto xml_export = std::make_shared<node>("./export.xml");
            auto insert_into = xml_export->appendChild("aircraft_exchange_file", true);
            io::AixmlConverter(xml_export->at("aircraft_exchange_file"),
                               {"component_design/wing/specific/geometry/aerodynamic_surface", "0", ""})(io::Wing(wing_surface));
            io::AixmlConverter(xml_export->at("aircraft_exchange_file"),
                               {"component_design/fuselage/specific/geometry/fuselage", "0", ""})(io::Fuselage(fuselage_surface));
            io::AixmlConverter(xml_export->at("aircraft_exchange_file"),
                               {"component_design/empennage/specific/geometry/aerodynamic_surface", "0", ""})(io::Wing(fin_surface));
            io::AixmlConverter(xml_export->at("aircraft_exchange_file"),
                               {"component_design/propulsion/specific/propulsion@0/nacelle", "0", ""})(io::Hull(nacelle_surface));
            io::AixmlConverter(xml_export->at("aircraft_exchange_file"),
                               {"component_design/propulsion/specific/propulsion@0/pylon", "0", ""})(io::AirfoilSurface(pylon_surface));
            aixml::saveDocument(*xml_export, 3);
        }
        else if (mode == "mesh")
        {
            /* Create a wing */
            auto wing_surface = wing.create("wing/specific/geometry/aerodynamic_surface@0");
            auto mesh = geom2::transform::to_mesh(wing_surface);
            CGAL::IO::write_PLY("./wing.ply", mesh);

            /* Create a fin */
            auto fin_surface = wing.create("empennage/specific/geometry/aerodynamic_surface@0");
            mesh = geom2::transform::to_mesh(fin_surface);
            CGAL::IO::write_PLY("./fin.ply", mesh);

            /* Create Fuselage */
            auto fuselage_surface = fuselage.create("fuselage/specific/geometry/fuselage@0");
            mesh = geom2::transform::to_mesh(fuselage_surface);
            CGAL::IO::write_PLY("./fuselage.ply", mesh);

            /* Create Nacelle */
            auto nacelle_surface = hull_factory.create("propulsion/specific/propulsion@0/nacelle@0");
            mesh = geom2::transform::to_mesh(nacelle_surface);
            CGAL::IO::write_PLY("./nacelle.ply", mesh);

            /* Create Pylon */
            auto pylon_surface = surface_factory.create("propulsion/specific/propulsion@0/pylon@0");
            mesh = geom2::transform::to_mesh(pylon_surface);
            CGAL::IO::write_PLY("./pylon.ply", mesh);
        }
        else
        {
            throw std::string("Unknown mode: " + mode);
        }
    }
    catch (std::string &exception)
    {
        std::cout << exception << std::endl;
    }
}