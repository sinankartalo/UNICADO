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

#include <aircraftGeometry2/geometry/acxml.h>
#include <aircraftGeometry2/geometry/factory.h>
#include <aircraftGeometry2/io/dat.h>
#include <aircraftGeometry2/processing/measure.h>
#include <aircraftGeometry2/processing/transform.h>
#include <algorithm>
#include <string>
#include <string_view>

/* === Using Directives ===*/
using std::literals::string_literals::operator""s;

/* === Constant Node Paths ===*/
constexpr std::string_view path_geometry_v2 = "AcftExchangeFile/Geometry/";
constexpr std::string_view path_geometry_v3 = "aircraft_exchange_file/component_design/";

namespace geom2
{
    // === AIXML Version 2.0 ===
    AIXMLv2::AIXMLv2(std::shared_ptr<node> AcXml, const std::filesystem::path &data_dir)
        : aircraft(AcXml), data_dir(data_dir)
    {
        /* Try inferring the geometry data from the AcXml file if data_dir is not valid */
        if (!std::filesystem::exists(data_dir))
        {
            std::filesystem::path XmlPath = this->aircraft->name;
            XmlPath = XmlPath.parent_path().make_preferred();
            if (std::filesystem::exists(XmlPath))
                this->data_dir = std::filesystem::absolute(XmlPath);
            else
                this->data_dir = std::filesystem::current_path();
        }
    }

    auto AIXMLv2::build_hull(const std::string_view name) -> MultisectionSurface<PolygonSection>
    {
        /* Create the surface to build */
        MultisectionSurface<PolygonSection> hull;
        SectionBuilder<PolygonSection> section_builder;

        /* Create the base path where to find the geometry */
        std::string node_path{path_geometry_v2};
        node_path += name;

        /* Set the origin */
        double x_ref = this->aircraft->at(node_path + "/NacelleRefPoint/r_Nacelle");
        double y_ref = this->aircraft->at(node_path + "/NacelleRefPoint/y_Nacelle");
        double z_ref = this->aircraft->at(node_path + "/NacelleRefPoint/h_Nacelle");
        hull.origin = {x_ref, y_ref, z_ref};

        /* === Sections ===*/
        auto segments = this->aircraft->at(node_path).getVector("NacelleParameters/NacelleSegment");

        /* Inlet Section */
        auto& inlet = this->aircraft->at(node_path + "/NacelleParameters/InletSegment");
        std::filesystem::path shape_file = this->data_dir / std::string(inlet.at("SegmentPointData"));
        PolygonSection shape{io::read_dat_file(shape_file)};
        shape.set_width(inlet.at("w_Inlet"));
        shape.set_height(inlet.at("h_Inlet"));
        shape.name = shape_file.stem().string();
        section_builder.insert_back(shape, {0, 0, 0});
        double z_offset_to_next = inlet.at("l_Inlet");

        /* Other Sections */
        for (auto &segment : segments)
        {
            shape_file = this->data_dir / std::string(segment->at("InnerSegmentPointData"));
            shape = PolygonSection{io::read_dat_file(shape_file)};
            shape.set_width(segment->at("w_o_Segment"));
            shape.set_height(segment->at("h_o_Segment"));
            shape.name = shape_file.stem().string();
            section_builder.insert_back(shape, {0, 0, z_offset_to_next});
            z_offset_to_next = segment->at("l_Segment");
        }

        /* Outlet Section */
        auto& outlet = this->aircraft->at(node_path + "/NacelleParameters/ExitSegment");
        shape_file = this->data_dir / std::string(outlet.at("SegmentPointData"));
        shape = PolygonSection{io::read_dat_file(shape_file)};
        shape.set_width(outlet.at("w_Exit"));
        shape.set_height(outlet.at("h_Exit"));
        shape.name = shape_file.stem().string();
        section_builder.insert_back(shape, {0, 0, z_offset_to_next});

        /* Get the sections from the builder and return the resulting surface */
        hull.sections = section_builder.get_result();
        return hull;
    }

    auto AIXMLv2::build_fuselage(const std::string_view name) -> MultisectionSurface<PolygonSection>
    {
        /* Create the surface to build */
        MultisectionSurface<PolygonSection> fuselage;
        SectionBuilder<PolygonSection> section_builder;

        /* Create the base path where to find the geometry */
        std::string node_path{path_geometry_v2};
        node_path += name;

        /* Set the origin */
        double x_ref = this->aircraft->at(node_path + "/FuselageRefPoint/r_Fuselage");
        double y_ref = this->aircraft->at(node_path + "/FuselageRefPoint/y_Fuselage");
        double z_ref = this->aircraft->at(node_path + "/FuselageRefPoint/h_Fuselage");
        fuselage.origin = {x_ref, y_ref, z_ref};

        /* === Sections ===*/
        Polygon_2 shape{};
        Vector_3 offset{0, 0, 0};
        node_path += "/FuselageParameters";

        /* Lambda which inserts the sections */
        auto insert_section = [this, &section_builder](node const *const segment)
        {
            std::filesystem::path shape_file = this->data_dir / std::string(segment->at("SegmentPointData"));
            PolygonSection section{io::read_dat_file(shape_file)};
            section.set_width(segment->at("w_Segment"));
            section.set_height(segment->at("h_Segment"));
            section.name = shape_file.stem().string();
            const double length = segment->at("l_Segment");
            const double y_offset = segment->at("deltah_Segment") - section_builder.peek().back().origin.y();
            section_builder.insert_back(section, {0, y_offset, length});
        };

        /* Tip Section */
        double y_tip = this->aircraft->at(node_path + "/NoseDescription/TipSegment/h_Tip");
        shape.push_back({0, 0});
        section_builder.insert_back(shape, {0, y_tip, 0});

        /* Nose Sections */
        auto segments = this->aircraft->at(node_path).getVector("NoseDescription/NoseSegment");
        std::for_each(segments.begin(), segments.end(), insert_section);

        /* Mid Sections */
        segments = this->aircraft->at(node_path).getVector("MidSectionDescription/MidSectionSegment");
        std::for_each(segments.begin(), segments.end(), insert_section);

        /* Tail Sections */
        segments = this->aircraft->at(node_path).getVector("TailDescription/TailSegment");
        std::for_each(segments.begin(), segments.end(), insert_section);

        /* Get the sections from the builder and return the resulting surface */
        fuselage.sections = section_builder.get_result();
        fuselage.sections.front().name = "ellipse"; // Set the name of the tip section
        return fuselage;
    }

    auto AIXMLv2::build_airfoil_surface(const std::string_view name) -> MultisectionSurface<AirfoilSection>
    {
        /* Create the surface to build */
        MultisectionSurface<AirfoilSection> wing;
        SectionBuilder<AirfoilSection> section_builder;

        /* Create the base path where to find the geometry */
        std::string node_path{path_geometry_v2};
        node_path += name;

        /* Set the base properties */
        wing.origin = this->get_point(node_path + "/SurfaceRefPoint");

        /* === Sections ===*/
        auto segments = this->aircraft->at(node_path)
                            .getVector("SurfaceParameters/SurfaceSegment");

        /* First Section */
        Vector_3 offset{0, 0, 0};
        std::filesystem::path airfoil_file = this->data_dir / std::string(segments[0]->at("InnerProfile_Segment"));
        AirfoilSection airfoil{io::read_airfoil(airfoil_file)};
        airfoil.name = airfoil_file.stem().string();
        airfoil.set_chord_length(segments[0]->at("l_i_Segment"));
        airfoil.set_twist_angle(segments[0]->at("epsilon_Segment") * detail::to_radians);
        section_builder.insert_back(airfoil, offset);

        /* Other Sections */
        for (auto &segment : segments)
        {
            airfoil_file = this->data_dir / std::string(segment->at("InnerProfile_Segment"));
            airfoil = AirfoilSection{io::read_airfoil(airfoil_file)};
            airfoil.name = airfoil_file.stem().string();
            airfoil.set_chord_length(segment->at("l_o_Segment"));
            airfoil.set_twist_angle(segment->at("epsilon_Segment") * detail::to_radians);
            offset = detail::get_offset(
                segment->at("s_Segment"),
                segment->at("nu_Segment") * detail::to_radians,
                segment->at("phi_Segment") * detail::to_radians);
            section_builder.insert_back(airfoil, offset);
        }

        /* Get the sections from the builder and return the resulting surface */
        wing.sections = section_builder.get_result();
        return wing;
    }

    auto AIXMLv2::build_spar(const std::string_view name) -> MultisectionSurface<PolygonSection>
    {
        /* Create the surface to build */
        MultisectionSurface<PolygonSection> spar;
        SectionBuilder<PolygonSection> section_builder;

        /* Create the base path where to find the geometry */
        std::string node_path{path_geometry_v2};
        node_path += name;
        // node_path += "/SurfaceParameters/HalfSurfaceDescription";

        /* Set the origin */
        spar.origin = this->get_point(node_path + "/SurfaceRefPoint");

        /* === Sections ===*/
        node_path += "/SurfaceParameters/HalfSurfaceDescription";
        auto segments = this->aircraft->at(node_path).getVector("HalfSurfaceSegment");
        /* First section */
        double x_front = segments[0]->at("l_rel_i_FrontSpar");
        double x_rear = segments[0]->at("l_rel_i_RearSpar");
        Polygon_2 shape{};
        shape.push_back({x_front, 0});
        shape.push_back({x_rear, 0});
        section_builder.insert_back(shape, {0, 0, 0});
        /* Other sections */
        for (auto &segment : segments)
        {
            x_front = segment->at("l_rel_o_FrontSpar");
            x_rear = segment->at("l_rel_o_RearSpar");
            shape = Polygon_2{};
            shape.push_back({x_front, 0});
            shape.push_back({x_rear, 0});
            section_builder.insert_back(shape, {0, 0, static_cast<double>(segment->at("s_Segment"))});
        }

        /* Get the sections from the builder and return the resulting surface */
        spar.sections = section_builder.get_result();
        return spar;
    }

    auto AIXMLv2::build_control_device(const std::string_view name) -> MultisectionSurface<PolygonSection>
    {
        /* Create the surfaces */
        MultisectionSurface<PolygonSection> flap;

        /* Create the base path where to find the geometry */
        std::string node_path{path_geometry_v2};
        node_path += name;

        /* Check the type of this flap based on its available parameters */
        auto device = this->aircraft->at(node_path);
        const bool is_TE_device = device.find("l_rel_TE_i") != nullptr;

        /* Set the geometry parameters */
        double x_inner_front{0.0};
        double x_inner_rear{0.0};
        double x_outer_front{0.0};
        double x_outer_rear{0.0};
        double span_inner{0.0};
        double span_outer{0.0};
        if (is_TE_device)
        {
            x_inner_front = device.at("l_rel_i");
            x_inner_rear = 1.0 - device.at("l_rel_TE_i");
            x_outer_front = device.at("l_rel_o");
            x_outer_rear = 1.0 - device.at("l_rel_TE_o");
            span_inner = device.at("s_rel_i");
            span_outer = device.at("s_rel_o");
        }
        else
        {
            x_inner_rear = device.at("l_rel_i");
            x_outer_rear = device.at("l_rel_o");
            span_inner = device.at("s_rel_i");
            span_outer = device.at("s_rel_o");
        }

        /* Create the polygon */
        Polygon_2 shape_inner{};
        shape_inner.push_back({x_inner_front, 0});
        shape_inner.push_back({x_inner_rear, 0});
        Polygon_2 shape_outer{};
        shape_outer.push_back({x_outer_front, 0});
        shape_outer.push_back({x_outer_rear, 0});

        /* Create the flap surface */
        flap.sections.emplace_back(shape_inner);
        flap.sections.emplace_back(shape_outer);

        /* Set the section origins */
        flap.sections[0].origin = {0, 0, -span_inner};
        flap.sections[1].origin = {0, 0, -span_outer};

        /* Return the flap */
        return flap;
    }

    auto AIXMLv2::build_wing(const std::string_view name) -> MultisectionSurface<AirfoilSection>
    {
        /* Create the surface to build */
        MultisectionSurface<AirfoilSection> wing;
        SectionBuilder<AirfoilSection> section_builder;

        /* Create the base path where to find the geometry */
        std::string node_path{path_geometry_v2};
        node_path += name;

        /* Set the base properties */
        wing.origin = this->get_point(node_path + "/SurfaceRefPoint");
        wing.is_symmetric = this->aircraft->at(node_path + "/SurfaceParameters/Symmetric");

        /* === Sections ===*/
        auto segments = this->aircraft->at(node_path)
                            .getVector("SurfaceParameters/HalfSurfaceDescription/HalfSurfaceSegment");

        /* First Section */
        Vector_3 offset{0, 0, 0};
        std::filesystem::path airfoil_file = this->data_dir / std::string(segments[0]->at("InnerProfile_Segment"));
        AirfoilSection airfoil{io::read_airfoil(airfoil_file)};
        airfoil.name = airfoil_file.stem().string();
        airfoil.set_chord_length(segments[0]->at("l_i_Segment"));
        airfoil.set_twist_angle(segments[0]->at("epsilon_Segment") * detail::to_radians);
        section_builder.insert_back(airfoil, offset);

        /* Other Sections */
        for (auto &segment : segments)
        {
            airfoil_file = this->data_dir / std::string(segment->at("InnerProfile_Segment"));
            airfoil = AirfoilSection{io::read_airfoil(airfoil_file)};
            airfoil.name = airfoil_file.stem().string();
            airfoil.set_chord_length(segment->at("l_o_Segment"));
            airfoil.set_twist_angle(segment->at("epsilon_Segment") * detail::to_radians);
            offset = detail::get_offset(
                segment->at("s_Segment"),
                segment->at("nu_Segment") * detail::to_radians,
                segment->at("phi_Segment") * detail::to_radians);
            section_builder.insert_back(airfoil, offset);
        }

        /* Get the sections from the builder and return the resulting surface */
        wing.sections = section_builder.get_result();
        return wing;
    }

    auto AIXMLv2::get_point(std::string_view id) const -> Point_3
    {
        /* Get the liftingSurface node with the id */
        const node &entity = this->aircraft->at(std::string{id});

        /* Get the reference point coordinates */
        double x_ref = entity.at("r_Surface");
        double y_ref = entity.at("y_Surface");
        double z_ref = entity.at("h_Surface");
        return Point_3{x_ref, y_ref, z_ref};
    }

    // === AIXML Version 3.0 ===
    AIXMLv3::AIXMLv3(std::shared_ptr<node> AcXml, const std::filesystem::path &data_dir)
        : aircraft(AcXml), data_dir(data_dir)
    {
    }

    auto AIXMLv3::build_hull(const std::string_view name) -> MultisectionSurface<PolygonSection>
    {
        /* Create the surface to build */
        MultisectionSurface<PolygonSection> nacelle;
        SectionBuilder<PolygonSection> section_builder;

        /* Create the base path where to find the geometry */
        std::string nacelle_path{path_geometry_v3};
        nacelle_path += name;

        /* Set the origin */
        nacelle.origin = this->get_point(nacelle_path);
        nacelle.normal = this->get_direction(nacelle_path + "/normal");

        ///* Change the coordinate systems */
        //nacelle.ac_to_global(Point_3{ 0.0, 0.0, 0.0 });

        /* === Sections ===*/
        auto sections = this->aircraft->at(nacelle_path).getVector("sections/section");
        for (const auto &section : sections)
        {
            /* Get the geometry of the section */
            std::filesystem::path geometry_file = this->data_dir / (std::string(section->at("profile/value")) + ".dat");
            PolygonSection shape{io::read_dat_file(geometry_file)};
            shape.name = geometry_file.stem().string();

            /* Set the origin of the section */
            shape.origin = {
                double{section->at("origin/x/value")},
                double{section->at("origin/y/value")},
                double{section->at("origin/z/value")}};

            /* Set the section properties */
            shape.set_width(section->at("width/value"));
            shape.set_height(section->at("height/value"));

            /* Add the section to the surface */
            nacelle.sections.emplace_back(shape);
        }

        /* Get the sections from the builder and return the resulting nacelle */
        return nacelle;
    }

    auto AIXMLv3::build_fuselage(const std::string_view name) -> MultisectionSurface<PolygonSection>
    {
        /* Create the surface to build */
        MultisectionSurface<PolygonSection> fuselage;
        SectionBuilder<PolygonSection> section_builder;

        /* Create the base path where to find the geometry */
        std::string fuselage_path{path_geometry_v3};
        fuselage_path += name;

        /* Set the entity properties of the surface */
        fuselage.name = std::string(this->aircraft->at(fuselage_path + "/name/value"));
        fuselage.origin = this->get_point(fuselage_path);
        fuselage.normal = this->get_direction(fuselage_path + "/direction");
        
        /* === Sections ===*/
        auto sections = this->aircraft->at(fuselage_path).getVector("sections/section");
        std::filesystem::path geometry_file{};
        PolygonSection shape{};
        Vector_3 offset{0, 0, 0};
        for (const auto &section : sections)
        {
            /* Get the sections dimensions */
            const double height_upper = section->at("upper_height/value");
            const double height_lower = section->at("lower_height/value");
            const double width = section->at("width/value");

            /* Check whether the width is near 0 */
            const bool is_tip = (std::abs(width) < 1e-6);

            /* Get the section shape */
            Polygon_2 poly;
            if (not is_tip)
            {
                geometry_file = this->data_dir / (std::string(section->at("section_shape/value")) + ".dat");
                if (geometry_file.filename().string().starts_with("ellipse"))
                {
                    /* Assume an ellipse was given as the section shape */
                    auto ellipse_upper = build::ellipse(width, 2 * height_upper).get_contour(true);
                    auto ellipse_lower = build::ellipse(width, 2 * height_lower).get_contour(true);
                    poly.insert(poly.vertices_end(), ellipse_upper.vertices_begin(), ellipse_upper.right_vertex() - 1);
                    poly.insert(poly.vertices_end(), ellipse_lower.right_vertex(), ellipse_lower.vertices_end() - 1);
                } else {
                    /* Assume a dat file was given as the section shape */
                    poly = io::read_dat_file(geometry_file);
                }
            } else {
                /* Insert one tip point */
                poly.push_back({0, 0});
            }
            shape = PolygonSection{poly};
            shape.name = std::string(section->at("name/value"));

            /* Set the origin of the section */
            shape.origin = {
                double{section->at("origin/x/value")},
                double{section->at("origin/y/value")},
                double{section->at("origin/z/value")}};

            /* Set the section properties */
            shape.set_height(height_upper + height_lower);
            shape.set_width(width);

            /* Add the section to the surface */
            fuselage.sections.emplace_back(shape);
        }

        /* Return the resulting nacelle */
        return fuselage;
    }

    auto AIXMLv3::build_airfoil_surface(const std::string_view name) -> MultisectionSurface<AirfoilSection>
    {
        /* Create the surface to build */
        MultisectionSurface<AirfoilSection> surface;

        /* Get the base path where to find the geometry */
        std::string node_path{path_geometry_v3};
        node_path += name;

        /* Set the entity properties of the surface */
        surface.origin = this->get_point(node_path + "/position");
        surface.normal = this->get_direction(node_path + "/normal");

        ///* Change the coordinate systems */
        //surface.ac_to_global(Point_3{ 0.0, 0.0, 0.0 });

        surface.is_symmetric = false;

        /* === Sections ===*/
        auto sections = this->aircraft->at(node_path).getVector("sections/section");
        std::filesystem::path airfoil_file{};
        AirfoilSection airfoil{};
        Vector_3 offset{0, 0, 0};
        for (const auto &section : sections)
        {
            /* Read the profile dat file*/
            airfoil_file = this->data_dir / (std::string(section->at("profile/value")) + ".dat");
            airfoil = AirfoilSection{io::read_airfoil(airfoil_file)};
            airfoil.name = airfoil_file.stem().string();

            /* Set the origin of the section */
            airfoil.origin = {
                double{section->at("origin/x/value")},
                double{section->at("origin/y/value")},
                double{section->at("origin/z/value")}};

            /* Set the section properties */
            airfoil.set_chord_length(section->at("chord_length/value"));
            airfoil.set_twist_angle(section->at("geometric_twist/value"));

            /* Add the section to the surface */
            surface.sections.emplace_back(airfoil);
        }

        /* Return the resulting surface */
        return surface;
    }

    auto AIXMLv3::build_spar(const std::string_view name) -> MultisectionSurface<PolygonSection>
    {
        /* Create the surface to build */
        MultisectionSurface<PolygonSection> spar;

        /* Get the base path where to find the geometry */
        std::string node_path{path_geometry_v3};
        node_path += name;

        /* Lambda which inserts the section */
        auto insert_section = [&spar](const node &position)
        {
            Polygon_2 shape;
            shape.push_back({double{position.at("chord/from/value")}, 0.0});
            shape.push_back({double{position.at("chord/to/value")}, 0.0});
            spar.sections.emplace_back(shape);
            spar.sections.back().origin = {0.0, 0.0, double{position.at("spanwise/value")}}; // Assume the XML has to correct extrusion direction
        };

        /* Insert the sections */
        insert_section(this->aircraft->at(node_path + "/position/inner_position"));
        insert_section(this->aircraft->at(node_path + "/position/outer_position"));

        /* Return the resulting nacelle */
        return spar;
    }

    auto AIXMLv3::build_control_device(const std::string_view name) -> MultisectionSurface<PolygonSection>
    {
        /* Defer this call to build_spar */
        return this->build_spar(name);
    }

    auto AIXMLv3::build_wing(const std::string_view name) -> MultisectionSurface<AirfoilSection>
    {
        /* Create the surface to build */
        MultisectionSurface<AirfoilSection> wing;

        /* Get the base path where to find the geometry */
        std::string node_path{path_geometry_v3};
        node_path += name;

        /* Set the entity properties of the surface */
        wing.name = std::string(this->aircraft->at(node_path + "/name/value"));
        wing.origin = this->get_point(node_path + "/position");
        wing.normal = this->get_direction(node_path + "/parameters/direction");

        ///* Change the coordinate systems */
        //wing.ac_to_global(Point_3{ 0.0, 0.0, 0.0 });

        wing.is_symmetric = isTrue(this->aircraft->at(node_path + "/parameters/symmetric/value"));

        /* === Sections ===*/
        auto sections = this->aircraft->at(node_path).getVector("parameters/sections/section");
        std::filesystem::path airfoil_file{};
        AirfoilSection airfoil{};
        Vector_3 offset{0, 0, 0};
        for (const auto &section : sections)
        {
            /* Read the profile dat file*/
            airfoil_file = this->data_dir / (std::string(section->at("profile/value")) + ".dat");
            airfoil = AirfoilSection{io::read_airfoil(airfoil_file)};
            airfoil.name = airfoil_file.stem().string();

            /* Set the origin of the section */
            airfoil.origin = {
                double{section->at("chord_origin/x/value")},
                double{section->at("chord_origin/y/value")},
                double{section->at("chord_origin/z/value")}};

            /* Set the section properties */
            airfoil.set_chord_length(section->at("chord_length/value"));
            airfoil.set_twist_angle(section->at("geometric_twist/value"));
            airfoil.scale_thickness(section->at("scale_thickness/value"));

            /* Add the section to the surface */
            wing.sections.emplace_back(airfoil);
        }

        /* Return the resulting surface */
        return wing;
    }

    auto AIXMLv3::get_point(std::string_view id) const -> Point_3
    {
        /* Get the liftingSurface node with the id */
        const node &entity = this->aircraft->at(std::string{id});

        /* Get the reference point of the fuselage */
        double x_ref = entity.at("x/value");
        double y_ref = entity.at("y/value");
        double z_ref = entity.at("z/value");
        return Point_3{x_ref, y_ref, z_ref};
    }

    auto AIXMLv3::get_direction(std::string_view id) const -> Direction_3
    {
        /* Get the liftingSurface node with the id */
        const node &entity = this->aircraft->at(std::string{id});

        /* Get the reference point of the fuselage */
        double x_ref = entity.at("x/value");
        double y_ref = entity.at("y/value");
        double z_ref = entity.at("z/value");
        return Direction_3{x_ref, y_ref, z_ref};
    }
}; // namespace geom2
