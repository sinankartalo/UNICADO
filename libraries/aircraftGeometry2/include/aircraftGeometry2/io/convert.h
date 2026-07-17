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

#ifndef AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_IO_CONVERT_H_
#define AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_IO_CONVERT_H_

/* === Includes === */
#include <memory>
#include <utility>
#include <string>
#include <string_view>
#include <variant>
#include <aixml/node.h>
#include "aircraftGeometry2/geometry/surface.h"

namespace geom2
{
    namespace io
    {
        /* === Helper classes ===
         * The following classes define how the converters
         * treat the surface. The visitor pattern of the converters
         * uses these class types to deduce the operator to call which
         * converts the surface according to the type.
         */

        /**
         * @struct Hull
         * @brief A converter will treat this type of surface as a hull.
         */
        struct Hull
        {
            Hull() = delete;
            explicit Hull(const MultisectionSurface<PolygonSection> &surface) : surface(surface) {}
            const geom2::MultisectionSurface<geom2::PolygonSection> &surface;
        };

        /**
         * @struct Fuselage
         * @brief A converter will treat this type of surface as a fuselage.
         */
        struct Fuselage
        {
            Fuselage() = delete;
            explicit Fuselage(const MultisectionSurface<PolygonSection> &surface) : surface(surface) {}
            const geom2::MultisectionSurface<geom2::PolygonSection> &surface;
        };

        /**
         * @struct Spar
         * @brief A converter will treat this type of surface as spar geometry
         */
        struct Spar
        {
            Spar() = delete;
            explicit Spar(const MultisectionSurface<PolygonSection> &surface) : surface(surface) {}
            const geom2::MultisectionSurface<geom2::PolygonSection> &surface;
        };

        /**
         * @struct ControlDevice
         * @brief A converter will treat this type of surface as a control device geometry
         */
        struct ControlDevice
        {
            ControlDevice() = delete;
            explicit ControlDevice(const MultisectionSurface<PolygonSection> &surface) : surface(surface) {}
            const geom2::MultisectionSurface<geom2::PolygonSection> &surface;
        };

        /**
         * @struct AirfoilSurface
         * @brief A converter will treat this type of surface as a airfoil surface.
         */
        struct AirfoilSurface
        {
            AirfoilSurface() = delete;
            explicit AirfoilSurface(const MultisectionSurface<AirfoilSection> &surface) : surface(surface) {}
            const geom2::MultisectionSurface<geom2::AirfoilSection> &surface;
        };

        /**
         * @struct Wing
         * @brief A converter will treat this type of surface as a wing.
         */
        struct Wing
        {
            Wing() = delete;
            explicit Wing(const MultisectionSurface<AirfoilSection> &surface) : surface(surface) {}
            const geom2::MultisectionSurface<geom2::AirfoilSection> &surface;
        };

        /**
         * @brief Variant type for the visitor pattern which defines the
         * types of surfaces which can be converted.
         */
        using SurfaceType = std::variant<Hull, Fuselage, Spar, ControlDevice, AirfoilSurface, Wing>;

        /* === Converter ===*/
        /**
         * @class Converter
         * @brief Converter base class which defines the interface for the
         * different converters.
         * The converters are intended to be used as visitors using `std::variant` as follows:
         * @code
         * // Treat the surface as a hull surface
         * geom2::io::SurfaceType hull = geom2::io::Hull{surface};
         *
         * // Convert using the aixml format
         * std::shared_ptr<node> result = std::visit(geom2::io::AixmlConverter{}, hull);
         * @endcode 
         * 
         * @tparam ReturnType The return type which are specific to the selected target format of the converter.
         */
        template <typename ReturnType>
        class Converter
        {
        public:
            /**
             * @brief Virtual default destructor as this is a base class.
             */
            virtual ~Converter() = default;

            /**
             * @brief Convert a surface as a hull to the target format.
             * 
             * @param hull The hull surface to convert.
             * @return ReturnType The result of the conversion.
             */
            virtual auto operator()(const Hull &hull) -> ReturnType = 0;

            /**
             * @brief Convert a surface as a fuselage to the target format.
             * 
             * @param fuselage The fuselage surface to convert.
             * @return ReturnType The result of the conversion.
             */
            virtual auto operator()(const Fuselage &fuselage) -> ReturnType = 0;

            /**
             * @brief Convert geometry as a spar to the target format.
             * 
             * @param spar The spar surface to convert.
             * @return ReturnType The result of the conversion.
             */
            virtual auto operator()(const Spar &spar) -> ReturnType = 0;

            /**
             * @brief Convert geometry as a control device to the target format.
             * 
             * @param device The control device surface to convert.
             * @return ReturnType The result of the conversion.
             */
            virtual auto operator()(const ControlDevice &device) -> ReturnType = 0;

            /**
             * @brief Convert a surface as an airfoil surface to the target format.
             * 
             * @param airfoil_surface The airfoil surface to convert.
             * @return ReturnType The result of the conversion.
             */
            virtual auto operator()(const AirfoilSurface &airfoil_surface) -> ReturnType = 0;

            /**
             * @brief Convert a surface as a wing surface to the target format.
             * 
             * @param wing The wing surface to convert.
             * @return ReturnType The result of the conversion.
             */
            virtual auto operator()(const Wing &wing) -> ReturnType = 0;
        };

        /**
         * @brief Convert the surfaces to aixml nodes.
         * The surface node will be appended to the given parent.
         * You can give any node reference where the surface will be inserted to.
         * Each of the visitor functions will return a reference to the created node.
         * When the node is already existing, only the parameters relevant for the
         * geometry will be overwritten, all other nodes will be left untouched.
         * 
         * @details About the internal pointers to the created objects:
         * 
         * -> Smart pointers do not work here, since `node` manages the destruction
         * automatically. Smart pointers would call the deleter of the 
         * children twice...
         */
        class AixmlConverter: public Converter<node&>
        {
        public:
            /** 
             * @struct NodeInfo
             * @brief Struct which contains the name, id, and description of a new node
             * when inserting the node into the tree.
             */
            struct NodeInfo
            {
                std::string name{}; /**< The name of the node. */
                std::string id{}; /**< The id of the surface. */
                std::string description{}; /**< The description of the node. */
            };

            /**
             * @brief Explicit constructor to append the surface to a parent node.
             * @attention The parent will be modified!
             * 
             * @param to_be_updated The pointer to the parent node to which the surface will be appended.
             * @param info The name and id of the surface.
             */
            explicit AixmlConverter(node* to_be_updated, const NodeInfo& info);

            /**
             * @brief Explicit constructor to append the surface to a parent node.
             * @attention The parent will be modified!
             * 
             * @param to_be_updated The reference to the parent node to which the surface will be appended.
             * @param info The name and id of the surface.
             */
            explicit AixmlConverter(node& to_be_updated, const NodeInfo& info) //NOLINT (runtime/references)
            : AixmlConverter(&to_be_updated, info) {}

            /**
             * @brief Explicit constructor to append the surface to a parent node.
             * @attention The parent will be modified!
             * 
             * @param to_be_updated The smart pointer to the parent node to which the surface will be appended.
             * @param info The name and id of the surface.
             */
            explicit AixmlConverter(std::shared_ptr<node> to_be_updated, const NodeInfo& info)
            : AixmlConverter(to_be_updated.get(), info) {}

            /**
             * @brief Convert a hull surface to an aixml node.
             * 
             * @param hull The hull surface to convert.
             * @return std::shared_ptr<node> Returns a shared pointer to the created node.
             */
            auto operator()(const Hull &hull) -> node& final;

            /**
             * @brief Convert a surface as a fuselage to the target format.
             * 
             * @param fuselage The fuselage surface to convert.
             * @return ReturnType The result of the conversion.
             */
            auto operator()(const Fuselage &fuselage) -> node& final;

            /**
             * @brief Convert spar geometry to an aixml node.
             * 
             * @param spar The spar geometry to convert.
             * @return std::shared_ptr<node> Returns a shared pointer to the created node.
             */
            auto operator()(const Spar &spar) -> node& final;

            /**
             * @brief Convert geometry as a control device to the target format.
             * 
             * @param device The control device surface to convert.
             * @return ReturnType The result of the conversion.
             */
            auto operator()(const ControlDevice &device) -> node& final;

            /**
             * @brief Convert an airfoil surface to an aixml node.
             * 
             * @param airfoil_surface The airfoil surface to convert.
             * @return std::shared_ptr<node> Returns a shared pointer to the created node.
             */
            auto operator()(const AirfoilSurface &airfoil_surface) -> node& final;

            /**
             * @brief Convert a wing surface to an aixml node.
             * 
             * @param wing The wing surface to convert.
             * @return std::shared_ptr<node> Returns a shared pointer to the created node.
             */
            auto operator()(const Wing &wing) -> node& final;

        private:
            /**
             * @brief Insert a string into the specified node.
             * @attention The target node will be modified!
             * 
             * @param target The pointer to the target node where the origin will be inserted.
             * @param content The string to insert.
             * @param info The name and description of the node which is inserted.
             */
            static void insert_string(node* target, std::string content, NodeInfo info);

            /**
             * @brief Insert some reference point of the surface into the specified node.
             * You can specify the name and description of the node which is inserted.
             * The description of the x,y, and z nodes will be set to a default description.
             * @attention The target node will be modified!
             * 
             * @param target The pointer to the target node where the point will be inserted.
             * @param point The point to insert.
             * @param info The name and description of the node to insert.
             */
            static void insert_point(node* target, geom2::Point_3 point, NodeInfo info);

            /**
             * @brief Insert some reference direction of the surface into the specified node.
             * You can specify the name and description of the node which is inserted.
             * The description of the x,y, and z nodes will be set to a default description.
             * @attention The target node will be modified!
             * 
             * @param target The pointer to the target node where the point will be inserted.
             * @param direction The direction to insert.
             * @param info The name and description of the node to insert.
             */
            static void insert_direction(node* target, geom2::Direction_3 direction, NodeInfo info);

            /* === Properties ===*/
            const NodeInfo surface_info; /**< The name and id of the surface. */
            node* new_node{nullptr}; /**< The node which is created. */
            node* parent;  /**< The parent node to which the surface will be appended to. */
        };
    }; // namespace io
}; // namespace geom2

#endif // AIRCRAFTGEOMETRY2_INCLUDE_AIRCRAFTGEOMETRY2_IO_CONVERT_H_

