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

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>
#include <CGAL/Surface_mesh/IO/PLY.h>
#include "aircraftGeometry2/geometry/entity3d.h"
#include "aircraftGeometry2/geometry/section.h"
#include "aircraftGeometry2/geometry/surface.h"
#include "aircraftGeometry2/geometry/factory.h"
#include "aircraftGeometry2/io/dat.h"
#include "aircraftGeometry2/io/convert.h"
#include "aircraftGeometry2/processing/transform.h"
#include "aircraftGeometry2/processing/measure.h"
#include "aircraftGeometry2/fuselage.h"
#include "aircraftGeometry2/hull_surface.h"
#include "aircraftGeometry2/airfoil_surface.h"

namespace py = pybind11;

/* === Python bindings === */
PYBIND11_MODULE(py11aircraftGeometry2, m)
{
    /* Module docstring */
    m.doc() = "Python bindings for aircraftGeometry2.";

    /* Bind class geom2::Point_2 to Python.  */
    py::class_<geom2::Point_2>(m, "Point_2")
        .def(py::init<>())
        .def(py::init<geom2::Point_2 const &>(), py::arg("other"))
        .def(py::init<int, int>(), py::arg("x"), py::arg("y"))
        .def(py::init<double, double>(), py::arg("x"), py::arg("y"))
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("x", &geom2::Point_2::x, "The x-coordinate of the point.")
        .def("y", &geom2::Point_2::y, "The y-coordinate of the point.");

    /* Bind class geom2::Point_3 to Python. */
    py::class_<geom2::Point_3>(m, "Point_3")
        .def(py::init<>())
        .def(py::init<geom2::Point_3 const &>(), py::arg("other"))
        .def(py::init<int, int, int>(), py::arg("x"), py::arg("y"), py::arg("z"))
        .def(py::init<double, double, double>(), py::arg("x"), py::arg("y"), py::arg("z"))
        .def(py::self < py::self) //cppcheck-suppress duplicateExpression
        .def(py::self > py::self) //cppcheck-suppress duplicateExpression
        .def(py::self <= py::self) //cppcheck-suppress duplicateExpression
        .def(py::self >= py::self) //cppcheck-suppress duplicateExpression
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self - py::self)
        .def(py::self + geom2::Vector_3())
        .def(py::self - geom2::Vector_3())
        .def("x", &geom2::Point_3::x, "The x-coordinate of the point.")
        .def("y", &geom2::Point_3::y, "The y-coordinate of the point.")
        .def("z", &geom2::Point_3::z, "The z-coordinate of the point.");

    /* Bind class geom2::Direction_3 to Python. */
    py::class_<geom2::Direction_3>(m, "Direction_3")
        .def(py::init<>())
        .def(py::init<geom2::Direction_3 const &>(), py::arg("other"))
        .def(py::init<int, int, int>(), py::arg("x"), py::arg("y"), py::arg("z"))
        .def(py::init<double, double, double>(), py::arg("x"), py::arg("y"), py::arg("z"))
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("dx", &geom2::Direction_3::dx, "The x-component of the direction.")
        .def("dy", &geom2::Direction_3::dy, "The y-component of the direction.")
        .def("dz", &geom2::Direction_3::dz, "The z-component of the direction.");

    /* Bind class geom2::Vector_3 to Python. */
    py::class_<geom2::Vector_3>(m, "Vector_3")
        .def(py::init<>())
        .def(py::init<geom2::Vector_3 const &>(), py::arg("other"))
        .def(py::init<int, int, int>(), py::arg("x"), py::arg("y"), py::arg("z"))
        .def(py::init<double, double, double>(), py::arg("x"), py::arg("y"), py::arg("z"))
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("x", &geom2::Vector_3::x, "The x-component of the vector.")
        .def("y", &geom2::Vector_3::y, "The y-component of the vector.")
        .def("z", &geom2::Vector_3::z, "The z-component of the vector.");

    /* Bind class geom2::Polygon_2 to Python. */
    py::class_<geom2::Polygon_2>(m, "Polygon_2")
        .def(py::init<>())
        .def(
            "size",
            &geom2::Polygon_2::size,
            "Get the number of vertices of the polygon."
        )
        .def(
            "push_back",
            &geom2::Polygon_2::push_back,
            py::arg("point"),
            "Add a 2D point as vertex to the polygon")
        .def(
            "vertex",
            py::overload_cast<std::size_t>(&geom2::Polygon_2::vertex, py::const_),
            py::arg("index"),
            "Get the vertex at the given index.");

    /* Bind class geom2::Entity3D to Python. */
    py::class_<geom2::Entity3D>(m, "Entity3D")
        .def(py::init<>())
        .def_readwrite("name", &geom2::Entity3D::name, "The name of the entity.")
        .def_readwrite("origin", &geom2::Entity3D::origin, "The origin point of the entity.")
        .def_readwrite("normal", &geom2::Entity3D::normal, "The normal vector of the entity.")
        .def_readwrite("rotation_z", &geom2::Entity3D::rotation_z, "The rotation around the local z-axis [rad].");

    /* Bind class geom2::Section to Python. */
    py::class_<geom2::Section, geom2::Entity3D>(m, "Section")
        .def(py::init<>())
        .def(py::init<geom2::Polygon_2 const &>(), py::arg("contour"))
        .def(
            "get_contour",
            &geom2::Section::get_contour,
            py::arg("apply_transform"),
            "Get the contour polygon of the section. If apply_transform is false, the normalized shape is returned.")
        .def(
            "set_contour",
            &geom2::Section::set_contour,
            py::arg("contour"),
            "Set the contour of the polygon section.")
        .def("size", &geom2::Section::size, "Get the number of vertices of the section.");

    /* Bind class geom2::PolygonSection to Python. */
    py::class_<geom2::PolygonSection, geom2::Section>(m, "PolygonSection")
        .def(py::init<>())
        .def(py::init<geom2::Polygon_2 const &>(), py::arg("contour"))
        .def("set_width", &geom2::PolygonSection::set_width, py::arg("width"), "Set the width of the section.")
        .def("set_height", &geom2::PolygonSection::set_height, py::arg("height"), "Set the height of the section.")
        .def("set_beta_angle", &geom2::PolygonSection::set_beta_angle, py::arg("angle"), "Set the beta angle of the section.")
        .def("set_scale", &geom2::PolygonSection::set_scale, py::arg("scale"), "Set the scale of the section.");

    /* Bind class geom2::AirfoilSection to Python. */
    py::class_<geom2::AirfoilSection, geom2::Section>(m, "AirfoilSection")
        .def(py::init<>())
        .def(py::init<geom2::Polygon_2 const &>(), py::arg("contour"))
        .def("set_chord_length", &geom2::AirfoilSection::set_chord_length, py::arg("length"), "Set the chord length of the airfoil.")
        .def("set_dihedral_angle", &geom2::AirfoilSection::set_dihedral_angle, py::arg("angle"), "Set the dihedral angle of the airfoil.")
        .def("set_twist_angle", &geom2::AirfoilSection::set_twist_angle, py::arg("angle"), "Set the twist angle of the airfoil.")
        .def("scale_thickness", &geom2::AirfoilSection::scale_thickness, py::arg("factor"), "Scale the thickness of the airfoil.")
        .def("get_chord_length", &geom2::AirfoilSection::get_chord_length, "Get the absolute chord length of the airfoil.")
        .def("get_thickness_scale", &geom2::AirfoilSection::get_thickness_scale, "Get the current thickness scaling factor of the airfoil.")
        .def("get_twist_angle", &geom2::AirfoilSection::get_twist_angle, "Get the twist angle of the airfoil.");

    /* Bind template class geom2::MultisectionSurface<PolygonSection> to Python. */
    py::class_<geom2::MultisectionSurface<geom2::PolygonSection>, geom2::Entity3D>(m, "PolygonSurface")
        .def(py::init<>())
        .def_readwrite("is_symmetric", &geom2::MultisectionSurface<geom2::PolygonSection>::is_symmetric, "Whether the surface is symmetric to the local `XY` plane at the surface origin.")
        .def_readwrite("sections", &geom2::MultisectionSurface<geom2::PolygonSection>::sections, "The sections of the surface.");

    /* Bind template class geom2::MultisectionSurface<AirfoilSection> to Python. */
    py::class_<geom2::MultisectionSurface<geom2::AirfoilSection>, geom2::Entity3D>(m, "AirfoilSurface")
        .def(py::init<>())
        .def_readwrite("is_symmetric", &geom2::MultisectionSurface<geom2::AirfoilSection>::is_symmetric, "Whether the surface is symmetric to the local `XY` plane at the surface origin.")
        .def_readwrite("sections", &geom2::MultisectionSurface<geom2::AirfoilSection>::sections, "The sections of the surface.");

    /* Bind the template class geom2::SectionBuilder<PolygonSection> to python. */
    py::class_<geom2::SectionBuilder<geom2::PolygonSection>>(m, "PolygonBuilder")
        .def(py::init<>())
        .def("get_result", &geom2::SectionBuilder<geom2::PolygonSection>::get_result, py::return_value_policy::move, "Get the result of the section builder.")
        .def(
            "insert_back",
            py::overload_cast<const geom2::PolygonSection &, geom2::Vector_3>(&geom2::SectionBuilder<geom2::PolygonSection>::insert_back),
            py::arg("shape"), py::arg("offset"),
            "Insert a new section at the back of the surface.")
        .def(
            "insert_back",
            py::overload_cast<const geom2::Polygon_2 &, geom2::Vector_3>(&geom2::SectionBuilder<geom2::PolygonSection>::insert_back),
            py::arg("shape"), py::arg("offset"),
            "Insert a new section at the back of the surface.")
        .def(
            "arrange",
            py::overload_cast<const geom2::Polygon_2 &, geom2::Vector_3, std::size_t>(&geom2::SectionBuilder<geom2::PolygonSection>::arrange),
            py::arg("shape"), py::arg("offset"), py::arg("count"),
            "Arrange count sections of the given shape with the given offset and insert them to the surface.");

    /* Bind the template class geom2::SectionBuilder<AirfoilSection> to python. */
    py::class_<geom2::SectionBuilder<geom2::AirfoilSection>>(m, "AirfoilBuilder")
        .def(py::init<>())
        .def("get_result", &geom2::SectionBuilder<geom2::AirfoilSection>::get_result, py::return_value_policy::move, "Get the result of the section builder.")
        .def(
            "insert_back",
            py::overload_cast<const geom2::AirfoilSection &, geom2::Vector_3>(&geom2::SectionBuilder<geom2::AirfoilSection>::insert_back),
            py::arg("shape"), py::arg("offset"),
            "Insert a new section at the back of the surface.")
        .def(
            "insert_back",
            py::overload_cast<const geom2::Polygon_2 &, geom2::Vector_3>(&geom2::SectionBuilder<geom2::AirfoilSection>::insert_back),
            py::arg("shape"), py::arg("offset"),
            "Insert a new section at the back of the surface.")
        .def(
            "arrange",
            py::overload_cast<const geom2::Polygon_2 &, geom2::Vector_3, std::size_t>(&geom2::SectionBuilder<geom2::AirfoilSection>::arrange),
            py::arg("shape"), py::arg("offset"), py::arg("count"),
            "Arrange count sections of the given shape with the given offset and insert them to the surface.");

    /* Add build submodule */
    auto m_build = m.def_submodule("build", "Builder functions for the geometry module.");
    m_build.def(
        "ellipse",
        geom2::build::ellipse,
        py::arg("width"), py::arg("height"), py::arg("points_per_quarter"),
        "Build a polygon section with the shape of an ellipse.");

    /* Add the io submodule */
    auto m_io = m.def_submodule("io", "Input and output interface functions for geometry.");
    m_io.def("read_dat_file", geom2::io::read_dat_file, py::arg("file"), "Read a dat-file and return the content as a 2D polygon.");
    m_io.def("read_airfoil", geom2::io::read_airfoil, py::arg("file"), "Read a dat-file containing airfoil coordinates and sort the points in counter-clockwise order.");

    /* Bind Lambda function to export a Multisection surface to Python. */
    m_io.def(
        "export_ply",
        [](const geom2::MultisectionSurface<geom2::PolygonSection> surface, std::filesystem::path path)
        {
            auto mesh = geom2::transform::to_mesh(surface);
            CGAL::IO::write_PLY(path.string(), mesh);
        },
        py::arg("surface"), py::arg("path"),
        "Export a Multisection surface to a PLY file.")
    .def(
        "export_ply",
        [](const geom2::MultisectionSurface<geom2::AirfoilSection> surface, std::filesystem::path path)
        {
            auto mesh = geom2::transform::to_mesh(surface);
            CGAL::IO::write_PLY(path.string(), mesh);
        },
        py::arg("surface"), py::arg("path"),
        "Export a Multisection surface to a PLY file.")
    .def(
        "export_ply",
        [](const std::vector<geom2::Point_3> &points, std::filesystem::path path)
        {
            CGAL::Surface_mesh<geom2::Point_3> mesh;
            for (auto &point : points)
            {
                mesh.add_vertex(point);
            }
            CGAL::IO::write_PLY(path.string(), mesh);
        },
        py::arg("points"), py::arg("path"),
        "Export a list of points to a PLY file.");

    /* Add the measure submodule */
    auto m_measure = m.def_submodule("measure", "Measurement functions for the geometry module.");
    m_measure
        .def(
            "area",
            py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &>(&geom2::measure::area),
            py::arg("surface"),
            "Calculate the surface area of a multi-section surface.")
        .def(
            "area",
            py::overload_cast<const geom2::MultisectionSurface<geom2::AirfoilSection> &>(&geom2::measure::area),
            py::arg("surface"),
            "Calculate the surface area of a multi-section surface.")
        .def(
            "aspect_ratio",
            geom2::measure::aspect_ratio,
            py::arg("surface"),
            "Calculate the aspect ratio of a multi-section surface.")
        .def(
            "bottom",
            py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &, double>(&geom2::measure::bottom),
            py::arg("surface"),
            py::arg("position"),
            "Return the bottom point of a polygon at a local z position.")
        .def(
            "center",
            py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &, double>(&geom2::measure::center),
            py::arg("surface"),
            py::arg("position"),
            "Calculate the bounding box center of a multi-section surface at the given position.")
        .def(
            "centroid",
            py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &>(&geom2::measure::centroid),
            py::arg("surface"),
            "Calculate the centroid of a multi-section surface.")
        .def(
            "centroid",
            py::overload_cast<const geom2::MultisectionSurface<geom2::AirfoilSection> &>(&geom2::measure::centroid),
            py::arg("surface"),
            "Calculate the centroid of a multi-section surface.")
        .def(
            "centroids",
            py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &>(&geom2::measure::centroids),
            py::arg("surface"),
            "Calculate the centroids of a multi-section surface.")
        .def(
            "centroids",
            py::overload_cast<const geom2::MultisectionSurface<geom2::AirfoilSection> &>(&geom2::measure::centroids),
            py::arg("surface"),
            "Calculate the centroids of a multi-section surface.")
        .def(
            "chord",
            geom2::measure::chord,
            py::arg("surface"),
            py::arg("position"),
            "Measure the chord length of an airfoil surface at a given position in local Z direction.")
        .def(
            "dihedral",
            geom2::measure::dihedral,
            py::arg("surface"),
            py::arg("position"),
            "Measure the dihedral angle of an airfoil surface at a given position in local Z direction.")
        .def(
            "height",
            geom2::measure::height,
            py::arg("surface"),
            py::arg("position"),
            "Measure the height of a multi-section surface at a given position in local Z direction.")
        .def(
            "left",
            geom2::measure::left,
            py::arg("surface"),
            py::arg("position"),
            "Measure the left point of a multi-section surface at a given position in local Z direction.")
        .def(
            "length",
            geom2::measure::length,
            py::arg("surface"),
            "Measure the length of a multi-section surface in local Z direction.")
        .def(
            "mean_aerodynamic_chord",
            geom2::measure::mean_aerodynamic_chord,
            py::arg("surface"),
            "Measure the mean aerodynamic chord of a multi-section surface.")
        .def(
            "mean_aerodynamic_chord_position",
            geom2::measure::mean_aerodynamic_chord_position,
            py::arg("surface"),
            "Measure the position of the mean aerodynamic chord of a multi-section surface in spanwise direction.")
        .def(
            "inertia",
            py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &, double>(&geom2::measure::inertia),
            py::arg("surface"),
            py::arg("refined_edge_length"),
            "Calculate the moment of inertia of a multi-section surface.")
        .def(
            "inertia",
            py::overload_cast<const geom2::MultisectionSurface<geom2::AirfoilSection> &, double>(&geom2::measure::inertia),
            py::arg("surface"),
            py::arg("refined_edge_length"),
            "Calculate the moment of inertia of a multi-section surface.")
        .def(
            "offset_LE",
            geom2::measure::offset_LE,
            py::arg("surface"),
            py::arg("position"),
            "Measure the offset of the leading edge of a multi-section surface at a given position in local Z direction.")
        .def(
            "reference_area",
            geom2::measure::reference_area,
            py::arg("surface"),
            "Calculate the reference area of a wing surface.")
        .def(
            "right",
            geom2::measure::right,
            py::arg("surface"),
            py::arg("position"),
            "Measure the right point of a multi-section surface at a given position in local Z direction.")
        .def(
            "span",
            geom2::measure::span,
            py::arg("surface"),
            "Measure the span of an airfoil surface in local Z direction.")
        .def(
            "sweep",
            geom2::measure::sweep,
            py::arg("surface"),
            py::arg("position"),
            py::arg("chord_offset"),
            "Measure the sweep angle of a multi-section surface at a given position in local Z direction. The chord offset can be used to set where the sweep angle is measured in chord direction.")
        .def(
            "taper",
            geom2::measure::taper_ratio,
            py::arg("surface"),
            "Measure the taper ratio of an airfoil surface.")
        .def(
            "thickness",
            geom2::measure::thickness,
            py::arg("section"),
            py::arg("position"),
            "Measure the absolute thickness of an airfoil at a local x position.")
        .def(
            "thickness_max",
            geom2::measure::thickness_max,
            py::arg("section"),
            "Measure the maximum thickness of an airfoil.")
        .def(
            "top",
            py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &, double>(&geom2::measure::top),
            py::arg("surface"),
            py::arg("position"),
            "Return the top point of a polygon at a local z position.")
        .def(
            "top_and_bottom",
            geom2::measure::top_and_bottom,
            py::arg("shape"),
            py::arg("position"),
            "Return the top and bottom point of a polygon at a local x position.")
        .def(
            "twist",
            geom2::measure::twist,
            py::arg("surface"),
            py::arg("position"),
            "Measure the twist angle of an airfoil surface at a given position in local Z direction.")
        .def(
            "volume",
            py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &>(&geom2::measure::volume),
            py::arg("surface"),
            "Measure the enclosed volume of a multi-section surface.")
        .def(
            "volume",
            py::overload_cast<const geom2::MultisectionSurface<geom2::AirfoilSection> &>(&geom2::measure::volume),
            py::arg("surface"),
            "Measure the enclosed volume of a multi-section surface.")
        .def(
            "width",
            geom2::measure::width,
            py::arg("surface"),
            py::arg("position"),
            "Measure the width of a multi-section surface at a given position in local Z direction.");

    /* Add the transform submodule */
    auto m_transform = m.def_submodule("transform", "Transformation functions for the geometry module.");
    m_transform.def(
        "outline_3d",
        py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &, const geom2::Direction_3 &>(&geom2::transform::outline_3d<geom2::PolygonSection>),
        py::arg("surface"), py::arg("direction"),
        "Get the outline of a multi-section surface when viewed from the given direction."
    )
    .def(
        "outline_3d",
        py::overload_cast<const geom2::MultisectionSurface<geom2::AirfoilSection> &, const geom2::Direction_3 &>(&geom2::transform::outline_3d<geom2::AirfoilSection>),
        py::arg("surface"), py::arg("direction"),
        "Get the outline of a multi-section surface when viewed from the given direction."
    )
    .def(
        "outline_2d",
        py::overload_cast<const geom2::MultisectionSurface<geom2::PolygonSection> &, const geom2::Direction_3 &>(&geom2::transform::outline_2d<geom2::PolygonSection>),
        py::arg("surface"), py::arg("direction"),
        "Get the outline of a multi-section surface when viewed from the given direction and project it in 2d on the view plane."
    )
    .def(
        "outline_2d",
        py::overload_cast<const geom2::MultisectionSurface<geom2::AirfoilSection> &, const geom2::Direction_3 &>(&geom2::transform::outline_2d<geom2::AirfoilSection>),
        py::arg("surface"), py::arg("direction"),
        "Get the outline of a multi-section surface when viewed from the given direction and project it in 2d on the view plane.")
    .def(
        "to_absolute",
        geom2::transform::to_absolute,
        py::arg("surface"), py::arg("reference"),
        "Transform the surface which is defined with normalized coordinates to absolute coordinates using the reference surface to derive the absolute dimensions.")
    .def(
        "resample",
        geom2::transform::resample,
        py::arg("polygon"), py::arg("count"),
        "Resample a polygon to the given number of vertices.");

    /* Add the factory submodule */
    auto m_factory = m.def_submodule("factory", "Factory functions for the geometry module.");
    py::class_<geom2::FuselageFactory>(m_factory, "FuselageFactory")
        .def(py::init<std::shared_ptr<node>, std::filesystem::path>(), py::arg("AcXml"), py::arg("data_dir"))
        .def("create", &geom2::FuselageFactory::create, py::arg("id"), "Create a fuselage surface with the given id.");
    py::class_<geom2::HullFactory>(m_factory, "HullFactory")
        .def(py::init<std::shared_ptr<node>, std::filesystem::path>(), py::arg("AcXml"), py::arg("data_dir"))
        .def("create", &geom2::HullFactory::create, py::arg("id"), "Create a hull surface with the given id.");
    py::class_<geom2::SparFactory>(m_factory, "SparFactory")
        .def(py::init<std::shared_ptr<node>, std::filesystem::path>(), py::arg("AcXml"), py::arg("data_dir"))
        .def("create", &geom2::SparFactory::create, py::arg("id"), "Create a spar surface with the given id.");
    py::class_<geom2::ControlDeviceFactory>(m_factory, "ControlDeviceFactory")
        .def(py::init<std::shared_ptr<node>, std::filesystem::path>(), py::arg("AcXml"), py::arg("data_dir"))
        .def("create", &geom2::ControlDeviceFactory::create, py::arg("id"), "Create all control device surfaces with the given id.");
    py::class_<geom2::AirfoilSurfaceFactory>(m_factory, "AirfoilSurfaceFactory")
        .def(py::init<std::shared_ptr<node>, std::filesystem::path>(), py::arg("AcXml"), py::arg("data_dir"))
        .def("create", &geom2::AirfoilSurfaceFactory::create, py::arg("id"), "Create an airfoil surface with the given id.");
    py::class_<geom2::WingFactory>(m_factory, "WingFactory")
        .def(py::init<std::shared_ptr<node>, std::filesystem::path>(), py::arg("AcXml"), py::arg("data_dir"))
        .def("create", &geom2::WingFactory::create, py::arg("id"), "Create a wing surface with the given id.");

    /* Add the convert submodule */
    auto m_convert = m.def_submodule("convert", "Convert the geometry to different formats.");
    /* AIXML */
    auto m_convert_aixml = m_convert.def_submodule("aixml", "Convert the geometry to the aircraft exchange XL format.");
    m_convert_aixml
        .def(
            "to_hull_surface",
            [](node& to_be_updated, const py::tuple info, const geom2::MultisectionSurface<geom2::PolygonSection> &surface)
            {
                /* Treat the surface as a airfoil surface */
                geom2::io::SurfaceType hull = geom2::io::Hull{surface};

                /* Convert using the aixml format */
                std::string name = info[0].cast<std::string>();
                std::string id = info[1].cast<std::string>();
                std::string description = info[2].cast<std::string>();
                return std::visit(geom2::io::AixmlConverter{to_be_updated, {name, id, description}}, hull);
            },
            py::return_value_policy::reference,
            py::arg("to_be_updated"),
            py::arg("surface_info"),
            py::arg("surface"),
            "Convert a multi-section surface to a hull node in the aircraft xml."
        )
        .def(
            "to_fuselage_surface",
            [](node& to_be_updated, const py::tuple info, const geom2::MultisectionSurface<geom2::PolygonSection> &surface)
            {
                /* Treat the surface as a airfoil surface */
                geom2::io::SurfaceType fuselage = geom2::io::Fuselage{surface};

                /* Convert using the aixml format */
                std::string name = info[0].cast<std::string>();
                std::string id = info[1].cast<std::string>();
                std::string description = info[2].cast<std::string>();
                return std::visit(geom2::io::AixmlConverter{to_be_updated, {name, id, description}}, fuselage);
            },
            py::return_value_policy::reference,
            py::arg("to_be_updated"),
            py::arg("surface_info"),
            py::arg("surface"),
            "Convert a multi-section surface to a fuselage node in the aircraft xml."
        )
        .def(
            "to_spar_surface",
            [](node& to_be_updated, const py::tuple info, const geom2::MultisectionSurface<geom2::PolygonSection> &surface)
            {
                /* Treat the surface as a airfoil surface */
                geom2::io::SurfaceType spar = geom2::io::Spar{surface};

                /* Convert using the aixml format */
                std::string name = info[0].cast<std::string>();
                std::string id = info[1].cast<std::string>();
                std::string description = info[2].cast<std::string>();
                return std::visit(geom2::io::AixmlConverter{to_be_updated, {name, id, description}}, spar);
            },
            py::return_value_policy::reference,
            py::arg("to_be_updated"),
            py::arg("surface_info"),
            py::arg("surface"),
            "Convert a multi-section surface to a spar node in the aircraft xml.")
        .def(
            "to_control_device",
            [](node& to_be_updated, const py::tuple info, const geom2::MultisectionSurface<geom2::PolygonSection> &surface)
            {
                /* Treat the surface as a airfoil surface */
                geom2::io::SurfaceType flap = geom2::io::ControlDevice{surface};

                /* Convert using the aixml format */
                std::string name = info[0].cast<std::string>();
                std::string id = info[1].cast<std::string>();
                std::string description = info[2].cast<std::string>();
                return std::visit(geom2::io::AixmlConverter{to_be_updated, {name, id, description}}, flap);
            },
            py::return_value_policy::reference,
            py::arg("to_be_updated"),
            py::arg("surface_info"),
            py::arg("surface"),
            "Convert a multi-section surface to a control device node in the aircraft xml.")
        .def(
            "to_aerodynamic_surface",
            [](node& to_be_updated, const py::tuple info, const geom2::MultisectionSurface<geom2::AirfoilSection> &surface)
            {
                /* Treat the surface as a airfoil surface */
                geom2::io::SurfaceType wing = geom2::io::AirfoilSurface{surface};

                /* Convert using the aixml format */
                std::string name = info[0].cast<std::string>();
                std::string id = info[1].cast<std::string>();
                std::string description = info[2].cast<std::string>();
                return std::visit(geom2::io::AixmlConverter{to_be_updated, {name, id, description}}, wing);
            },
            py::return_value_policy::reference,
            py::arg("to_be_updated"),
            py::arg("surface_info"),
            py::arg("surface"),
            "Convert a multi-section surface to an aerodynamic surface node in the aircraft xml.");
}
