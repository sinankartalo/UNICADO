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

#ifndef AIRCRAFTGEOMETRY2_SRC_IMPORTER_ACXML_H_
#define AIRCRAFTGEOMETRY2_SRC_IMPORTER_ACXML_H_

 /* === Includes === */
#include <filesystem>
#include <memory>
#include <string_view>
#include <aixml/node.h>
#include <vector>
#include "../airfoil_surface.h"
#include "../fuselage.h"
#include "../hull_surface.h"
#include "surface.h"
#include "builder.h"
#include "entity3d.h"
#include "factory.h"
#include "section.h"
#include "../io/convert.h"
#include "../io/dat.h"
#include "../processing/transform.h"
#include "../processing/measure.h"
#include "acxml.h"

namespace geom2
{
    using AeroSurface = MultisectionSurface<AirfoilSection>;
    using Spar = MultisectionSurface<PolygonSection>;
    using ControlDevice = MultisectionSurface<PolygonSection>;

    auto get_position = [](const node& component) -> Point_3
        {
            return Point_3{ double{component.at("position/x/value")},
                double{component.at("position/y/value")},
                double{component.at("position/z/value")} };
        };

    auto get_normal = [](const node& component) -> Direction_3
        {
            return Direction_3{ double{component.at("direction/x/value")},
                double{component.at("direction/y/value")},
                double{component.at("direction/z/value")} };
        };

    bool check_aerodynamic_surface_type(const std::string& type){
        if (type != "wing" && type != "empennage"){
            return false;
        } else {
            return true;
        }
    }

    auto import_aerodynamic_surfaces(const std::shared_ptr<node>& acxml, const std::filesystem::path& data_dir,
        const std::string& type = "wing") -> std::vector<AeroSurface>
    {   
        if (!check_aerodynamic_surface_type(type)){
            std::cerr << "Warning: Unknwon aerodynamic surface type." << std::endl;
            return {};
        }

        auto acxml_aerodynamic_surface = acxml->getVector("aircraft_exchange_file/component_design/" + type + "/specific/geometry/aerodynamic_surface");
        auto aerodynamic_surface_position = get_position(acxml->at("aircraft_exchange_file/component_design/" + type));

        WingFactory airfoil_factory{ acxml, data_dir };

        std::vector<AeroSurface> aerodynamic_surfaces;

        std::transform(acxml_aerodynamic_surface.begin(), acxml_aerodynamic_surface.end(), std::back_inserter(aerodynamic_surfaces),
            [&airfoil_factory, &type](auto& aerodynamic_surface) {
                return airfoil_factory.create(type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface->getStringAttrib("ID"));
            });

        auto offset = aerodynamic_surface_position - CGAL::ORIGIN;

        std::ranges::for_each(
            aerodynamic_surfaces,
            [&offset](auto& aerodynamic_surface)
            { aerodynamic_surface.origin += offset; });

        return aerodynamic_surfaces;
    }

    auto import_spars(const std::shared_ptr<node>& acxml, const std::filesystem::path& data_dir,
        const std::string& aerodynamic_surface_ID, const std::string& type = "wing") -> std::vector<Spar>
    {
        if (!check_aerodynamic_surface_type(type)){
            std::cerr << "Warning: Unknwon aerodynamic surface type." << std::endl;
            return {};
        }

        std::vector<Spar> spars;

        auto acxml_spars = acxml->getVector("aircraft_exchange_file/component_design/" + type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface_ID + "/parameters/spars/spar");
        auto aerodynamic_surface_ref_position = get_position(acxml->at("aircraft_exchange_file/component_design/" + type));
        auto aerodynamic_surface_position = get_position(acxml->at("aircraft_exchange_file/component_design/" + type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface_ID));
        auto aerodynamic_surface_normal = get_normal(acxml->at("aircraft_exchange_file/component_design/" + type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface_ID));

        /* Create the factory */
        SparFactory spar_factory{ acxml, data_dir };

        std::transform(acxml_spars.begin(), acxml_spars.end(), std::back_inserter(spars),
            [&spar_factory, &aerodynamic_surface_ID, &type](auto& spar) {
                auto read_spar = spar_factory.create(type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface_ID + "/spars/spar@" + spar->getStringAttrib("ID"));
                read_spar.name = spar->getStringAttrib("description");
                return read_spar;
            });

        auto aerodynamic_surface_offset = aerodynamic_surface_ref_position - CGAL::ORIGIN;
        auto spar_offset = aerodynamic_surface_position - CGAL::ORIGIN;

        std::ranges::for_each(
            spars,
            [&aerodynamic_surface_offset, &spar_offset, &aerodynamic_surface_normal](auto& spar)
            { 
                spar.origin += aerodynamic_surface_offset;
                spar.origin += spar_offset;
                spar.normal = -aerodynamic_surface_normal;
            });

        return spars;
    }

    auto import_control_devices(const std::shared_ptr<node>& acxml, const std::filesystem::path& data_dir,
        const std::string& aerodynamic_surface_ID, const std::string& type = "wing") -> std::vector<ControlDevice>
    {   
        if (!check_aerodynamic_surface_type(type)){
            std::cerr << "Warning: Unknwon aerodynamic surface type." << std::endl;
            return {};
        }

        std::vector<ControlDevice> control_devices;

        auto acxml_control_devices = acxml->getVector("aircraft_exchange_file/component_design/" + type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface_ID + "/parameters/control_devices/control_device");
        auto aerodynamic_surface_ref_position = get_position(acxml->at("aircraft_exchange_file/component_design/" + type));
        auto aerodynamic_surface_position = get_position(acxml->at("aircraft_exchange_file/component_design/" + type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface_ID));
        auto aerodynamic_surface_normal = get_normal(acxml->at("aircraft_exchange_file/component_design/" + type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface_ID));

        /* Create the factory */
        ControlDeviceFactory control_device_factory{ acxml, data_dir };

        std::transform(acxml_control_devices.begin(), acxml_control_devices.end(), std::back_inserter(control_devices),
            [&control_device_factory, &aerodynamic_surface_ID, &type](auto& control_device) {
                auto read_control_device = control_device_factory.create(type + "/specific/geometry/aerodynamic_surface@" + aerodynamic_surface_ID + "/control_devices/control_device@" + control_device->getStringAttrib("ID"));
                read_control_device.name = control_device->getStringAttrib("description");
                return read_control_device;
            });

        auto aerodynamic_surface_offset = aerodynamic_surface_ref_position - CGAL::ORIGIN;
        auto control_device_offset = aerodynamic_surface_position - CGAL::ORIGIN;

        std::ranges::for_each(
            control_devices,
            [&aerodynamic_surface_offset, &control_device_offset, &aerodynamic_surface_normal](auto& control_device)
            { 
                control_device.origin += aerodynamic_surface_offset;
                control_device.origin += control_device_offset;
                control_device.normal = -aerodynamic_surface_normal;
            });

        return control_devices;
    }

    auto import_external_nacelle_design(const std::shared_ptr<node>& external_geometry, const std::filesystem::path& data_dir) -> MultisectionSurface<PolygonSection>
    {
        /* Create the surface to build */
        MultisectionSurface<PolygonSection> nacelle;
        SectionBuilder<PolygonSection> section_builder;

        /* Set the origin */
        nacelle.origin = {
                double{external_geometry->at("nacelle/position/x/value")},
                double{external_geometry->at("nacelle/position/y/value")},
                double{external_geometry->at("nacelle/position/z/value")} };
        nacelle.normal = {
                double{external_geometry->at("nacelle/normal/x/value")},
                double{external_geometry->at("nacelle/normal/y/value")},
                double{external_geometry->at("nacelle/normal/z/value")} };

        /* === Sections ===*/
        auto sections = external_geometry->getVector("nacelle/sections/section");
        for (const auto& section : sections)
        {

            /* Get the geometry of the section */
            std::filesystem::path geometry_file = data_dir / (std::string(section->at("profile/value")) + ".dat");
            PolygonSection shape{ io::read_dat_file(geometry_file) };
            shape.name = geometry_file.stem().string();

            /* Set the origin of the section */
            shape.origin = {
                double{section->at("origin/x/value")},
                double{section->at("origin/y/value")},
                double{section->at("origin/z/value")} };

            /* Set the section properties */
            shape.set_width(section->at("width/value"));
            shape.set_height(section->at("height/value"));

            /* Add the section to the surface */
            nacelle.sections.emplace_back(shape);
        }

        return nacelle;
    }

    auto scale_nacelle(MultisectionSurface<PolygonSection>& nacelle_geometry, const double& area_scale_factor) -> MultisectionSurface<PolygonSection>
    {
        std::ranges::for_each(
            nacelle_geometry.sections,
            [&area_scale_factor](auto& section)
            {
                auto transformation = Kernel::Aff_transformation_3(CGAL::SCALING, std::sqrt(area_scale_factor));
                section.origin = transformation(section.origin);
                section.set_width(section.get_contour().bbox().x_span() * std::sqrt(area_scale_factor));
                section.set_height(section.get_contour().bbox().y_span() * std::sqrt(area_scale_factor));
            });

        return nacelle_geometry;
    }

}
#endif