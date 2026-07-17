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


#ifndef LILIGEOINTERFACE_LILIGEOINTERFACE_H_
#define LILIGEOINTERFACE_LILIGEOINTERFACE_H_

#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <windows.h>
#include <aircraftGeometry2/airfoil_surface.h>
#include <aircraftGeometry2/fuselage.h>
#include <aircraftGeometry2/hull_surface.h>
#include <aircraftGeometry2/geometry/surface.h>
#include <aircraftGeometry2/geometry/builder.h>
#include <aircraftGeometry2/geometry/entity3d.h>
#include <aircraftGeometry2/geometry/factory.h>
#include <aircraftGeometry2/geometry/section.h>
#include <aircraftGeometry2/io/convert.h>
#include <aircraftGeometry2/io/dat.h>
#include <aircraftGeometry2/processing/transform.h>
#include <aircraftGeometry2/processing/measure.h>

namespace lifting_line
{
    struct Quartet
    {
        geom2::Kernel::Point_2 TL;
        geom2::Kernel::Point_2 BL;
        geom2::Kernel::Point_2 TR;
        geom2::Kernel::Point_2 BR;

        Quartet(const geom2::Kernel::Point_2& TL, const geom2::Kernel::Point_2& BL, const geom2::Kernel::Point_2& TR, const geom2::Kernel::Point_2& BR) :
            TL(TL), BL(BL), TR(TR), BR(BR)
        {};
    };

    using Fuselage = geom2::MultisectionSurface<geom2::PolygonSection>;
    using Wing = geom2::MultisectionSurface<geom2::AirfoilSection>;
    using Nacelle = geom2::MultisectionSurface<geom2::PolygonSection>;
    using Pylon = geom2::MultisectionSurface<geom2::AirfoilSection>;

    struct Aircraft
    {
        std::vector<Fuselage> fuselages;                        /** Fuselage(s) of the aircraft. */
        std::vector<Wing> wings;                                /** Wings of the aircraft. */
        std::vector<Wing> empennage;                            /** Empennage(s) of the aircraft. */
        std::vector<Nacelle> nacelle;                           /** Nacelle(s) of the propulsion assembly. */
        std::vector<Pylon> pylon;                               /** Pylon(s) of the propulsion assembly. */
    };

    inline auto get_ref_area(const std::shared_ptr<Aircraft> aircraft_geometry) -> double
    {
        std::vector<double> wing_areas = {};
        for (auto wing : aircraft_geometry->wings)
        {
            wing_areas.push_back(geom2::measure::reference_area(wing));
        }
        return *std::max_element(wing_areas.begin(), wing_areas.end());
    }

    inline auto get_root_chord(const std::shared_ptr<Aircraft> aircraft_geometry) -> double
    {
        std::vector<double> root_chords = {};
        for (auto wing : aircraft_geometry->wings)
        {
            root_chords.push_back(geom2::measure::chord(wing, 0.0));
        }
        return *std::max_element(root_chords.begin(), root_chords.end());
    }

    inline auto get_half_span(const std::shared_ptr<Aircraft> aircraft_geometry) -> double
    {
        std::vector<double> spans = {};
        for (auto wing : aircraft_geometry->wings)
        {
            spans.push_back(geom2::measure::span(wing));
        }
        return *std::max_element(spans.begin(), spans.end());
    }

    inline auto get_ac_cog(const std::shared_ptr<node>& aircraft_xml) -> std::vector<double>
    {
        std::vector<double> cog = {
            aircraft_xml->at("aircraft_exchange_file/analysis/masses_cg_inertia/maximum_takeoff_mass/mass_properties/center_of_gravity/x/value"),
            aircraft_xml->at("aircraft_exchange_file/analysis/masses_cg_inertia/maximum_takeoff_mass/mass_properties/center_of_gravity/y/value"),
            aircraft_xml->at("aircraft_exchange_file/analysis/masses_cg_inertia/maximum_takeoff_mass/mass_properties/center_of_gravity/z/value") };

        return cog;
    }

    inline auto get_ac_inertia(const std::shared_ptr<node>& aircraft_xml) -> std::vector<double>
    {
        std::vector<double> I_sym = {
            aircraft_xml->at("aircraft_exchange_file/analysis/masses_cg_inertia/model/maximum_takeoff_mass/mass_properties/inertia/j_xx/value"),
            aircraft_xml->at("aircraft_exchange_file/analysis/masses_cg_inertia/model/maximum_takeoff_mass/mass_properties/inertia/j_yy/value"),
            aircraft_xml->at("aircraft_exchange_file/analysis/masses_cg_inertia/model/maximum_takeoff_mass/mass_properties/inertia/j_zz/value") };

        return I_sym;
    }

    inline auto create_aircraft_geometry(const std::shared_ptr<node>& aircraft_xml, std::filesystem::path geom_directories) -> std::shared_ptr<Aircraft>
    {
        /* Lambda to extract the component position reference */
        auto get_position = [](const node& component) -> geom2::Point_3
            {
                return geom2::Point_3{ double{component.at("position/x/value")},
                    double{component.at("position/y/value")},
                    double{component.at("position/z/value")} };
            };

        /* Create the aircraft geometry */
        std::shared_ptr<Aircraft> aircraft_geometry;
        aircraft_geometry = std::make_shared<Aircraft>();

        /* Create the surface builders */
        geom2::WingFactory wing_factory{ aircraft_xml, geom_directories };
        geom2::FuselageFactory fuselage_factory{ aircraft_xml, geom_directories };
        geom2::AirfoilSurfaceFactory airfoil_factory{ aircraft_xml, geom_directories };
        geom2::HullFactory hull_factory{ aircraft_xml, geom_directories };

        try
        {
            const auto wings = aircraft_xml->getVector("aircraft_exchange_file/component_design/wing/specific/geometry/aerodynamic_surface");
            const auto wing_position = get_position(aircraft_xml->at("aircraft_exchange_file/component_design/wing"));

            std::transform(wings.begin(), wings.end(), std::back_inserter(aircraft_geometry->wings),
                [&wing_factory](const auto& wing)
                { return wing_factory.create(
                    "wing/specific/geometry/aerodynamic_surface@" + wing->getStringAttrib("ID")); });

            /* Convert the origins of the wings to the global coordinate system and add them to the mesh */
            auto offset = wing_position - CGAL::ORIGIN;
            geom2::Kernel::Aff_transformation_2 mirror_2d = geom2::Kernel::Aff_transformation_2(1, 0, 0, -1);

            std::ranges::for_each(
                aircraft_geometry->wings,
                [&offset, mirror_2d](auto& wing)
                { wing.origin += offset; });
        }
        catch (...)
        {
            aircraft_geometry->wings = {};
            std::cout << "Wing geometry not defined in the acXML\n";
        }
            
        try
        {
            const auto fuselages = aircraft_xml->getVector("aircraft_exchange_file/component_design/fuselage/specific/geometry/fuselage");
            const auto fuselage_position = get_position(aircraft_xml->at("aircraft_exchange_file/component_design/fuselage"));

            std::transform(fuselages.begin(), fuselages.end(), std::back_inserter(aircraft_geometry->fuselages),
                [&fuselage_factory](const auto& fuselage)
                { return fuselage_factory.create(
                    "fuselage/specific/geometry/fuselage@" + fuselage->getStringAttrib("ID")); });

            /* Convert the origins of the fuselages to the global coordinate system and add them to the mesh */
            auto offset = fuselage_position - CGAL::ORIGIN;
            std::ranges::for_each(
                aircraft_geometry->fuselages,
                [&offset](auto& fuselage)
                { fuselage.origin += offset; });
        }
        catch (...)
        {
            aircraft_geometry->fuselages = {};
            std::cout << "Fuselage geometry not defined in the acXML\n";
        }
            
        try
        {
            const auto empennage = aircraft_xml->getVector("aircraft_exchange_file/component_design/empennage/specific/geometry/aerodynamic_surface");
            const auto empennage_position = get_position(aircraft_xml->at("aircraft_exchange_file/component_design/empennage"));

            std::transform(empennage.begin(), empennage.end(), std::back_inserter(aircraft_geometry->empennage),
                [&wing_factory](const auto& empennage)
                { return wing_factory.create(
                    "empennage/specific/geometry/aerodynamic_surface@" + empennage->getStringAttrib("ID")); });

            /* Convert the origins of the empennage to the global coordinate system and add them to the mesh */
            auto offset = empennage_position - CGAL::ORIGIN;
            std::ranges::for_each(
                aircraft_geometry->empennage,
                [&offset](auto& empennage)
                { empennage.origin += offset; });
        }
        catch (...)
        {
            aircraft_geometry->empennage = {};
            std::cout << "Empennage geometry not defined in the acXML\n";
        }
            
        return aircraft_geometry;
    }

    inline auto relative_position(const geom2::MultisectionSurface<geom2::AirfoilSection>& sections, const geom2::Kernel::Point_2& point) -> double
    {
        auto local_chord = geom2::measure::chord(sections, -point.y());
        auto x_LE = geom2::measure::offset_LE(sections, -point.y());
        auto x_distance = point.x() - x_LE.x();
        
        return x_distance / local_chord;
    };

    inline auto get_local_thickness(const geom2::MultisectionSurface<geom2::AirfoilSection>& surface, const geom2::Kernel::Point_2& point) -> double
    {
        auto inboard_section_index = geom2::detail::get_section_index(surface.sections, -point.y());

        double relative_x_position = relative_position(surface, point);

        auto inboard_thickness = geom2::measure::thickness(
            surface.sections.at(inboard_section_index.value()), relative_x_position);

        auto outboard_thickness = geom2::measure::thickness(
            surface.sections.at(inboard_section_index.value() +1), relative_x_position);

        auto dthickness_dy = -(outboard_thickness - inboard_thickness) / 
            (surface.sections.at(inboard_section_index.value() + 1).origin.z() - surface.sections.at(inboard_section_index.value()).origin.z());

        return inboard_thickness + dthickness_dy * (point.y() + surface.sections.at(inboard_section_index.value()).origin.z());
    }

    inline auto get_local_skeleton(const geom2::MultisectionSurface<geom2::AirfoilSection>& surface, const geom2::Kernel::Point_2& point) -> double
    {
        auto inboard_section_index = geom2::detail::get_section_index(surface.sections, -point.y());

        double relative_x_position = relative_position(surface, point);

        auto inboard_section = surface.sections.at(inboard_section_index.value());

        const auto [top, bottom] = geom2::measure::top_and_bottom(
            inboard_section.get_contour(false), relative_x_position);

        const auto local_thickness = get_local_thickness(surface, point);

        if (top.y() == 0. && bottom.y() == 0) { return 0.; };

        return ((top.y() - bottom.y()) * 0.5 + bottom.y()) * local_thickness / (top.y() - bottom.y());
    }

    inline auto get_local_twist(const geom2::MultisectionSurface<geom2::AirfoilSection>& sections, const geom2::Kernel::Point_2& point1, const geom2::Kernel::Point_2& point2) -> double
    {
        double thickness_1 = get_local_skeleton(sections, point1);
        double thickness_2 = get_local_skeleton(sections, point2);

        double h = thickness_2 - thickness_1;
        double x = point2.x() - point1.x();

        return std::atan2(h, x);
    };

    struct Panel
    {
        geom2::Point_3 inboard_point = { 0., 0., 0.};
        geom2::Point_3 outboard_point = { 0., 0., 0.};
        std::string inboard_coupling = "F F T T F F";
        std::string outboard_coupling = "F F T T F F";
        double inboard_chord;
        double outboard_chord;
        double span;
        double inboard_twist;
        double outboard_twist;

        Panel(const Quartet& quartet, const geom2::MultisectionSurface<geom2::AirfoilSection>& sections)
        {
            double inboard_proj_chord = quartet.BL.x() - quartet.TL.x();
            double outboard_proj_chord = quartet.BR.x() - quartet.TR.x();

            span = quartet.TR.y() - quartet.TL.y();

            inboard_point += {0.25 * inboard_proj_chord + quartet.TL.x(), quartet.TL.y(), 0.};
            outboard_point += {0.25 * outboard_proj_chord + quartet.TR.x(), quartet.TR.y(), 0.};;

            auto TL_LE = geom2::measure::offset_LE(sections, quartet.TL.y());
            auto TR_LE = geom2::measure::offset_LE(sections, quartet.TR.y());

            inboard_chord = inboard_proj_chord / std::cos(get_local_twist(sections, quartet.TL, quartet.BL));
            outboard_chord = outboard_proj_chord / std::cos(get_local_twist(sections, quartet.TR, quartet.BR));

            inboard_twist = geom2::measure::twist(sections, quartet.TL.y()) + get_local_twist(sections, quartet.TL, quartet.BL);
            outboard_twist = geom2::measure::twist(sections, quartet.TR.y()) + get_local_twist(sections, quartet.TR, quartet.BR);

            auto inboard_thickness = 0.25 * inboard_proj_chord * std::tan(get_local_twist(sections, quartet.TL, quartet.BL)) + get_local_skeleton(sections, quartet.TL);
            auto outboard_thickness = 0.25 * outboard_proj_chord * std::tan(get_local_twist(sections, quartet.TR, quartet.BR)) + get_local_skeleton(sections, quartet.TR);

            inboard_point += {0., 0., TL_LE.y() + inboard_thickness};
            outboard_point += {0., 0., TR_LE.y() + outboard_thickness};

            double wing_span = geom2::measure::span(sections)*0.5;

            if (quartet.TL.y() == 0.)
            {
                inboard_coupling = "F T F F F F";
            };

            if (quartet.TR.y() >= wing_span - 0.1)
            {
                outboard_coupling = "T F F F F F";
            }
        }
    };
    
    inline auto create_mesh(const node& case_node, const geom2::MultisectionSurface<geom2::AirfoilSection>& sections) -> std::vector<std::vector<Panel>>
    {
        double n_chordwise = case_node.at("n_chordwise_panel/value");
        double span = geom2::measure::span(sections);
        double n_spanwise = case_node.at("n_spanwise_section/value");
        double increment = 0.5 * span / n_spanwise - 0.0000001;
        double position = 0.;
        std::vector<Quartet> panel_points = {};
        std::vector<std::vector<Panel>> panel_elements = {};

        while (position < 0.5 * span - n_spanwise*0.001)
        {
            double chord_1 = geom2::measure::chord(sections, position);
            double chord_2 = geom2::measure::chord(sections, position + increment);

            auto offset_LE_1 = geom2::measure::offset_LE(sections, position);
            auto offset_LE_2 = geom2::measure::offset_LE(sections, position + increment);

            std::vector<Panel> segment_panels = {};

            for (int i = 0; i < n_chordwise; i++)
            {
                geom2::Kernel::Point_2 TL = { chord_1 / n_chordwise * i + offset_LE_1.x(), position };
                geom2::Kernel::Point_2 BL = { chord_1 / n_chordwise * (i + 1) + offset_LE_1.x(), position };

                geom2::Kernel::Point_2 TR = { chord_2 / n_chordwise * i + offset_LE_2.x(), position + increment };
                geom2::Kernel::Point_2 BR = { chord_2 / n_chordwise * (i + 1) + offset_LE_2.x(), position + increment };

                Quartet element = Quartet(TL, BL, TR, BR);
                panel_points.push_back(element);

                segment_panels.push_back(Panel(element, sections));
            };

            panel_elements.push_back(segment_panels);
            position += increment;
        }
        return panel_elements;
    }

    inline auto rotate_surface(const std::vector<int> segment_idx_vec, int chordwise_idx, const double deflection_angle , std::vector<std::vector<Panel>>& mesh) -> std::vector<std::vector<Panel>>
    {
        auto new_mesh = mesh;

        for (auto segment_idx : segment_idx_vec)
        {
            auto segment = mesh[segment_idx];

            double inboard_offset_x = segment[chordwise_idx].inboard_chord * 0.25 * std::cos(segment[chordwise_idx].inboard_twist);
            double inboard_offset_z = segment[chordwise_idx].inboard_chord * 0.25 * std::sin(segment[chordwise_idx].inboard_twist);

            double outboard_offset_x = segment[chordwise_idx].outboard_chord * 0.25 * std::cos(segment[chordwise_idx].outboard_twist);
            double outboard_offset_z = segment[chordwise_idx].outboard_chord * 0.25 * std::sin(segment[chordwise_idx].outboard_twist);

            auto inboard_translation = geom2::Kernel::Aff_transformation_3(1., 0., 0., -inboard_offset_x, 0., 1., 0., 0., 0., 0., 1., -inboard_offset_z);
            auto outboard_translation = geom2::Kernel::Aff_transformation_3(1., 0., 0., -outboard_offset_x, 0., 1., 0., 0., 0., 0., 1., -outboard_offset_z);

            auto inboard_origin = inboard_translation(segment[chordwise_idx].inboard_point);
            auto outboard_origin = outboard_translation(segment[chordwise_idx].outboard_point);

            auto chordwise_position = chordwise_idx;

            while (chordwise_position < segment.size())
            {
                auto inboard_vector = inboard_origin - segment[chordwise_position].inboard_point;
                auto outboard_vector = outboard_origin - segment[chordwise_position].outboard_point;

                auto inboard_angle = std::atan2(inboard_vector.z(), inboard_vector.x());
                auto outboard_angle = std::atan2(outboard_vector.z(), outboard_vector.x());

                double inboard_radius = std::sqrt(std::pow(inboard_vector.x(), 2.) + std::pow(inboard_vector.y(), 2.) + std::pow(inboard_vector.z(), 2.));
                double outboard_radius = std::sqrt(std::pow(outboard_vector.x(), 2.) + std::pow(outboard_vector.y(), 2.) + std::pow(outboard_vector.z(), 2.));

                inboard_translation = geom2::Kernel::Aff_transformation_3(
                    1., 0., 0., -inboard_radius * std::cos(deflection_angle + inboard_angle),
                    0., 1., 0., 0.,
                    0., 0., 1., -inboard_radius * std::sin(deflection_angle + inboard_angle));

                outboard_translation = geom2::Kernel::Aff_transformation_3(
                    1., 0., 0., -outboard_radius * std::cos(deflection_angle + outboard_angle),
                    0., 1., 0., 0.,
                    0., 0., 1., -outboard_radius * std::sin(deflection_angle + outboard_angle));

                segment[chordwise_position].inboard_point = inboard_translation(inboard_origin);
                segment[chordwise_position].outboard_point = outboard_translation(outboard_origin);

                segment[chordwise_position].inboard_twist += deflection_angle;
                segment[chordwise_position].outboard_twist += deflection_angle;

                chordwise_position++;
            }

            new_mesh[segment_idx] = segment;
        }

        return new_mesh;
    }

    inline auto set_boundary_conditions(std::vector<std::vector<Panel>>& segments) -> std::vector<std::vector<Panel>>
    {
        for (int i = 1; i < segments.size(); i++)
        {
            for (int j = 0; j < segments[i].size(); j++)
            {
                double x_diff = segments[i][j].inboard_point.x() - segments[i - 1][j].outboard_point.x();
                double y_diff = segments[i][j].inboard_point.y() - segments[i - 1][j].outboard_point.y();
                double z_diff = segments[i][j].inboard_point.z() - segments[i - 1][j].outboard_point.z();

                if (x_diff < 0.0001 && y_diff < 0.0001 && z_diff < 0.0001)
                {
                    segments[i][j].inboard_coupling = "F F T T F F";
                    segments[i - 1][j].outboard_coupling = "F F T T F F";
                }
                else
                {
                    segments[i][j].inboard_coupling = "T F F F F F";
                    segments[i - 1][j].outboard_coupling = "T F F F F F";
                };
            }
        }

        return segments;
    }
};

#endif