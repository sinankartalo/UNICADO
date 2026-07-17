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

#include "aircraftGeometry2/io/convert.h"
#include <unordered_map>
#include <numbers>
#include <aixml/endnode.h>
#include "aircraftGeometry2/processing/measure.h"

/* === Types ===*/
using std::literals::string_literals::operator""s;

/* === Functions === */
namespace geom2
{
    namespace detail
    {
        /**
         * @brief Create a endnode object and set its properties.
         *
         * @tparam T The value type of the endnode.
         * @param value The value of the endnode.
         * @param unit The unit of the endnode.
         * @param node_path The location in the parent node of the endnode.
         * @param description The description of the endnode.
         * @return Endnode<T> Return the created endnode.
         */
        template<typename T>
        auto create_endnode(const T value, const std::string& unit, const std::string& node_path, const std::string& description) -> Endnode<T>
        {
            /* Create the node an set its properties */
            Endnode<T> new_node{node_path, description};
            new_node.set_value(value);
            new_node.set_unit(unit);

            /* Return the new node */
            return new_node;
        }

    }; // namespace detail

    namespace io
    {
        AixmlConverter::AixmlConverter(node* to_be_updated, const NodeInfo& info)
        : parent(to_be_updated)
        , surface_info(info)
        {
            /* Check for nullptr of parent */
            if (this->parent == nullptr)
            {
                throw std::runtime_error("AixmlConverter::AixmlConverter: parent node is nullptr");
            }

            /* Create the new node and update its description */
            this->new_node = &this->parent->operator[](this->surface_info.name + "@" + this->surface_info.id);
            this->new_node->setAttrib("description", this->surface_info.description);
        }

        auto AixmlConverter::operator()(const Hull &hull) -> node&
        {
            /* NodeInfo which is used throughout the function */
            NodeInfo info{};

            /* Add the origin position */
            /* -> The constructor makes sure that `this->new_node` has a valid value! */

            ///* Change the coordinate systems */
            //hull.surface.global_to_ac(Point_3 {0.0, 0.0, 0.0});

            info.name = "position";
            info.description = "Origin of the nacelle in the global aircraft coordinate system";
            this->insert_point(this->new_node, hull.surface.origin, info);

            /* Add the normal direction */
            info.name = "normal";
            info.description = "Normal direction of the nacelle in the global aircraft coordinate system";
            this->insert_direction(this->new_node, hull.surface.normal, info);

            /* Add the sections -> always override existing sections */
            node& sections = this->new_node->operator[](std::string{"sections"});
            sections.setAttrib("description", "Geometrical description nacelle sections");
            sections.deleteChildren();
            for (std::size_t i_section = 0; i_section < hull.surface.sections.size(); i_section++)
            {
                /* Reference the current section */
                const auto& section = hull.surface.sections[i_section];

                /* Create and reference the section node */
                node* section_node = &sections.appendChild("section", true);
                section_node->setAttrib("ID", std::to_string(i_section));

                /* Insert the origin point of the section */
                info.name = "origin";
                info.description = "Origin of the section (local)";
                this->insert_point(section_node, section.origin, info);

                /* Update the width of the section */
                auto width = detail::create_endnode(
                    section.get_contour(true).bbox().x_span(),
                    "m",
                    "width",
                    "width of the section");
                width.update(*section_node);

                /* Update the height of the section */
                auto height = detail::create_endnode(
                    section.get_contour(true).bbox().y_span(),
                    "m",
                    "height",
                    "Height of the section");
                height.update(*section_node);

                /* Update the profile name of the section */
                info.name = "profile";
                info.description = "The profile name of the section";
                this->insert_string(section_node, section.name, info);
            }

            /* Return the reference to the created surface node */
            return *this->new_node;
        }


        auto AixmlConverter::operator()(const Fuselage &fuselage) -> node&
        {
            /* NodeInfo which is used throughout the function */
            NodeInfo info{};

            /* Add the name data */
            /* -> The constructor makes sure that `this->new_node` has a valid value! */
            info.name = "name";
            info.description = "Name of the fuselage";
            this->insert_string(this->new_node, fuselage.surface.name, info);

            ///* Change the coordinate systems */
            //fuselage.surface.global_to_ac(Point_3{ 0.0, 0.0, 0.0 });

            /* Add the origin position */
            info.name = "position";
            info.description = "Position of one entire fuselage with regard to the global reference point.";
            this->insert_point(this->new_node, fuselage.surface.get_origin_ac_coordinate(Point_3{ 0.0, 0.0, 0.0 }), info);

            /* Add the normal direction */
            info.name = "direction";
            info.description = "unit vector according to global coordinate system for direction applied at position";
            this->insert_direction(this->new_node, fuselage.surface.get_direction_ac_coordinate(), info);

            /* Add the sections -> always override existing sections */
            node& sections = this->new_node->operator[](std::string{"sections"});
            sections.setAttrib("description", "Geometrical description of the fuselage sections of one entire fuselage");
            sections.deleteChildren();
            for (std::size_t i_section = 0; i_section < fuselage.surface.sections.size(); i_section++)
            {
                /* Reference the current section */
                const auto& section = fuselage.surface.sections[i_section];
                // const double span = section.origin.z();

                /* Create and reference the section node */
                node* section_node = &sections.appendChild("section", true);
                section_node->setAttrib("ID", std::to_string(i_section));

                /* Insert the name of the section */
                info.name = "name";
                info.description = "Name of the fuselage section";
                this->insert_string(section_node, std::string("section_") + std::to_string(i_section), info);

                /* Insert the name of the section shape */
                info.name = "section_shape";
                info.description = "Contains a string with name of *.dat file or the keyword ellipse";
                this->insert_string(section_node, section.name, info);

                /* Insert the origin point of the section */
                info.name = "origin";
                info.description = "Origin of fuselage section (local).";
                this->insert_point(section_node, section.origin, info);

                /* Update the upper height of the section */
                auto height_upper = detail::create_endnode(
                    section.get_contour(true).bbox().ymax(),
                    "m",
                    "upper_height",
                    "Height of the upper half of the fuselage section.");
                /** @todo Should `aircraftGeometry2` deal with the boundaries or is this the users responsibility? */
                // height_upper.set_lower_boundary(0.0);
                height_upper.update(*section_node);

                /* Update the lower height of the section */
                auto height_lower = detail::create_endnode(
                    std::abs(section.get_contour(true).bbox().ymin()),
                    "m",
                    "lower_height",
                    "Height of the lower half of the fuselage section.");
                // height_lower.set_upper_boundary(0.0);
                height_lower.update(*section_node);

                /* Update the width of the segment */
                auto width = detail::create_endnode(
                    section.get_contour(true).bbox().x_span(),
                    "m",
                    "width",
                    "Width of the fuselage section.");
                width.update(*section_node);
            }

            /* Return the reference to the created surface node */
            return *this->new_node;
        }

        auto AixmlConverter::operator()(const Spar& spar) -> node&
        {
            /* NodeInfo which is used throughout the function */
            NodeInfo info{};

            /* Insert the position nodes */
            /* -> The constructor makes sure that `this->new_node` has a valid value! */
            (*this->new_node)["position"s].setAttrib("description", "relative chord position");
            (*this->new_node)["position/inner_position"s].setAttrib("description", "relative inner position");
            (*this->new_node)["position/inner_position/chord"s].setAttrib("description", "chord position");
            (*this->new_node)["position/outer_position"s].setAttrib("description", "relative outer position");
            (*this->new_node)["position/outer_position/chord"s].setAttrib("description", "chord position");

            /* Set the position values */
            auto span_inner = detail::create_endnode(
                -1.0 * spar.surface.sections[0].origin.z(), // Airfoil surface are expected to be extruded in negative z direction
                "1",
                "spanwise",
                "relative spanwise position");
            span_inner.update(this->new_node->at("position/inner_position"));
            auto chord_inner_from = detail::create_endnode(
                spar.surface.sections[0].get_contour(true).bbox().xmin(),
                "1",
                "from",
                "relative chord position");
            chord_inner_from.update(this->new_node->at("position/inner_position/chord"));
            auto chord_inner_to = detail::create_endnode(
                spar.surface.sections[0].get_contour(true).bbox().xmax(),
                "1",
                "to",
                "relative chord position");
            chord_inner_to.update(this->new_node->at("position/inner_position/chord"));
            auto span_outer = detail::create_endnode(
                -1.0 * spar.surface.sections[1].origin.z(), // Airfoil surface are expected to be extruded in negative z direction
                "1",
                "spanwise",
                "relative spanwise position");
            span_outer.update(this->new_node->at("position/outer_position"));
            auto chord_outer_from = detail::create_endnode(
                spar.surface.sections[1].get_contour(true).bbox().xmin(),
                "1",
                "from",
                "relative chord position");
            chord_outer_from.update(this->new_node->at("position/outer_position/chord"));
            auto chord_outer_to = detail::create_endnode(
                spar.surface.sections[1].get_contour(true).bbox().xmax(),
                "1",
                "to",
                "relative chord position");
            chord_outer_to.update(this->new_node->at("position/outer_position/chord"));

            /* Return the reference to the created surface node */
            return *this->new_node;
        }

        auto AixmlConverter::operator()(const ControlDevice &device) -> node&
        {
            /* The control device can be treated  as the spar surface */
            Spar facade{device.surface};

            /* Return the converted result */
            return (*this)(facade);
        }

        auto AixmlConverter::operator()(const AirfoilSurface &airfoil_surface) -> node&
        {
            /* NodeInfo which is used throughout the function */
            NodeInfo info{};

            ///* Change the coordinate systems */
            //airfoil_surface.surface.global_to_ac(Point_3{ 0.0, 0.0, 0.0 });

            /* Add the origin position */
            /* -> The constructor makes sure that `this->new_node` has a valid value! */
            info.name = "position";
            info.description = "reference position in aircraft coordinates";
            this->insert_point(this->new_node, airfoil_surface.surface.origin, info);

            /* Start the parameters description */
            info.name = "normal";
            info.description = "unit vector according to aircraft coordinate system for direction applied at position";
            this->insert_direction(this->new_node, airfoil_surface.surface.normal, info);

            /* Add the sections -> always override existing sections */
            node& sections = (*this->new_node)[std::string{"sections"}];
            sections.setAttrib("description", "sections");
            sections.deleteChildren();
            for (std::size_t i_section = 0; i_section < airfoil_surface.surface.sections.size(); i_section++)
            {
                /* Reference the current section */
                const auto& section = airfoil_surface.surface.sections[i_section];

                /* Create and reference the section node */
                node* section_node = &sections.appendChild("section", true);
                section_node->setAttrib("ID", std::to_string(i_section));

                /* Insert the origin point of the section */
                info.name = "origin";
                info.description = "origin of chord (local)";
                this->insert_point(section_node, section.origin, info);

                /* Update the chord length of the section */
                auto chord_length = detail::create_endnode(
                    section.get_chord_length(),
                    "m",
                    "chord_length",
                    "chord length of the section");
                chord_length.update(*section_node);

                /* Update the geometric twist of the section */
                /* @fixed Add the actual twist readout! */
                auto geometric_twist = detail::create_endnode(
                    section.get_twist_angle(),
                    "rad",
                    "geometric_twist",
                    "geometric twist of the section around leading edge");
                geometric_twist.update(*section_node);

                /* Insert the profile name */
                info.name = "profile";
                info.description = "profile (data normalized to chord length)";
                this->insert_string(section_node, section.name, info);
            }

            /* Return the reference to the created surface node */
            return *this->new_node;
        }

        auto AixmlConverter::operator()(const Wing &wing) -> node&
        {
            /* NodeInfo which is used throughout the function */
            NodeInfo info{};

            /* Add the name data */
            /* -> The constructor makes sure that `this->new_node` has a valid value! */
            info.name = "name";
            info.description = "name of surface";
            this->insert_string(this->new_node, wing.surface.name, info);

            ///* Change the coordinate systems */
            //wing.surface.global_to_ac(Point_3{ 0.0, 0.0, 0.0 });

            /* Add the origin position */
            info.name = "position";
            info.description = "reference position in aircraft coordinates";
            this->insert_point(this->new_node, wing.surface.origin, info);

            /* Start the parameters description */
            (*this->new_node)["parameters"s].setAttrib("description", "aerodynamic surface parameters");
            info.name = "direction";
            info.description = "unit vector according to aircraft coordinate system for direction applied at position";
            this->insert_direction(&this->new_node->at("parameters"), wing.surface.normal, info);

            /* Add the symmetric information */
            const std::string symmetric = wing.surface.is_symmetric ? "true" : "false";
            info.name = "symmetric";
            info.description = "symmetric to local x-z plane";
            this->insert_string(&this->new_node->at("parameters"), symmetric, info);

            /* Add the sections -> always override existing sections */
            node& sections = this->new_node->at("parameters")[std::string{"sections"}];
            sections.setAttrib("description", "sections");
            sections.deleteChildren();
            for (std::size_t i_section = 0; i_section < wing.surface.sections.size(); i_section++)
            {
                /* Reference the current section */
                const auto& section = wing.surface.sections[i_section];

                /* Create and reference the section node */
                node* section_node = &sections.appendChild("section", true);
                section_node->setAttrib("ID", std::to_string(i_section));

                /* Insert the origin point of the section */
                info.name = "chord_origin";
                info.description = "origin of chord (local)";
                this->insert_point(section_node, section.origin, info);

                /* Update the chord length of the section */
                auto chord_length = detail::create_endnode(
                    section.get_chord_length(),
                    "m",
                    "chord_length",
                    "chord length of the section");
                chord_length.update(*section_node);

                /* Update the geometric twist of the section */
                /* @fixed Add the actual twist readout! */
                auto geometric_twist = detail::create_endnode(
                    section.get_twist_angle(),
                    "rad",
                    "geometric_twist",
                    "geometric twist of the section around leading edge");
                geometric_twist.update(*section_node);

                /* Update the scale_thickness of the section */
                auto scale_thickness = detail::create_endnode(
                    section.get_thickness_scale(),
                    "1",
                    "scale_thickness",
                    "scale the thickness defined by the profile with this factor");
                scale_thickness.update(*section_node);

                /* Insert the profile name */
                info.name = "profile";
                info.description = "profile (data normalized to chord length)";
                this->insert_string(section_node, section.name, info);
            }

            /* Return the reference to the created surface node */
            return *this->new_node;
        }

        void AixmlConverter::insert_string(node* target, const std::string content, const NodeInfo info)
        {
            /* Create the name node */
            Endnode<std::string> string_node{info.name, info.description};
            string_node = content;

            /* Insert this node into target */
            string_node.update(*target);
        }

        void AixmlConverter::insert_point(node* target, const geom2::Point_3 point, const NodeInfo info)
        {
            /* Create the position node and use its reference to further access it*/
            node& point_node = (*target)[info.name];
            point_node.setAttrib("description", info.description);

            /* Create and add the value end nodes */
            auto x_value = detail::create_endnode(point.x(), "m", "x", "x coordinate of point");
            auto y_value = detail::create_endnode(point.y(), "m", "y", "y coordinate of point");
            auto z_value = detail::create_endnode(point.z(), "m", "z", "z coordinate of point");
            x_value.update(point_node);
            y_value.update(point_node);
            z_value.update(point_node);
        }

        void AixmlConverter::insert_direction(node* target, const geom2::Direction_3 direction, const NodeInfo info)
        {
            /* Create the direction node and use its reference to further access it*/
            node& direction_node = (*target)[info.name];
            direction_node.setAttrib("description", info.description);

            /* Create and add the value end nodes */
            auto x_value = detail::create_endnode(direction.dx(), "1", "x", "x direction of unit vector");
            auto y_value = detail::create_endnode(direction.dy(), "1", "y", "y direction of unit vector");
            auto z_value = detail::create_endnode(direction.dz(), "1", "z", "z direction of unit vector");
            x_value.update(direction_node);
            y_value.update(direction_node);
            z_value.update(direction_node);
        }
    }; // namespace io
}; // namespace geom2
