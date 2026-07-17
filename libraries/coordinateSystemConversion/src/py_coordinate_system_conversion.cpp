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
#include <pybind11/numpy.h>
#include <vector>
#include "coordinateSystemConversion/coordinateBase.h"
#include "coordinateSystemConversion/coordinateSystemConversionConf.h"

namespace py = pybind11;

/* === Python bindings === */

//PYBIND11_MODULE(py11coordinateSystemConversion, m)
//{
//    namespace csc = CoordinateSystemConversion;
//
//    m.doc() = "Python bindings for coordinateSystemConversion";
//
//    m.def("convertCoordinateSystem",
//        &csc::convertCoordinateSystem,
//        py::arg("from") = coordinateBase(),
//        py::arg("to") = coordinateBase(),
//        py::arg("element3D") = std::vector<double>());
//
//    m.def("convertCoordinateSystem3D",
//        &csc::convertCoordinateSystem3D,
//        py::arg("from") = coordinateBase(),
//        py::arg("to"),
//        py::arg("element3D") = std::vector<double>());
//        
//    m.def("convertAircraftDesign",
//        &csc::convertAircraftDesign,
//        py::arg("to") = coordinateBase(),
//        py::arg("element3D") = std::vector<double>());
//        
//    m.def("convertCGAL",
//        &csc::convertCGAL,
//        py::arg("to") = coordinateBase(),
//        py::arg("element3D") = std::vector<double>());
//        
//    m.def("convertAircraftDynamics",
//        &csc::convertAircraftDynamics,
//        py::arg("to") = coordinateBase(),
//        py::arg("element3D") = std::vector<double>());
//        
//    m.def("ACDesign_to_CGAL",
//        &csc::ACDesign_to_CGAL,
//        py::arg("element3D") = std::vector<double>());
//        
//    m.def("ACDesign_to_ACDynamics",
//        &csc::ACDesign_to_ACDynamics,
//        py::arg("element3D") = std::vector<double>());
//        
//    m.def("CGAL_to_ACDynamics",
//        &csc::CGAL_to_ACDynamics,
//        py::arg("element3D") = std::vector<double>());
//        
//    m.def("CGAL_to_ACDesign",
//        &csc::CGAL_to_ACDesign,
//        py::arg("element3D") = std::vector<double>());
//        
//    m.def("ACDynamics_to_CGAL",
//        &csc::ACDynamics_to_CGAL,
//        py::arg("element3D") = std::vector<double>());
//
//    m.def("ACDynamics_to_ACDesign", 
//        &csc::ACDynamics_to_ACDesign,
//        py::arg("element3D") = std::vector<double>());
//}

PYBIND11_MODULE(py11coordinateSystemConversion, m)
{
    namespace sc = SimpleConversion;

    m.doc() = "Python bindings for simple coordinate system conversion";

    py::class_ <sc::Element3D>(m, "Element3D")
        .def(py::init<>())
        .def(py::init<double, double, double, double, double, double, double, double, double>(),
            py::arg("xx"), py::arg("xy"), py::arg("xz"),
            py::arg("yx"), py::arg("yy"), py::arg("yz"),
            py::arg("zx"), py::arg("zy"), py::arg("zz"))
        .def(py::init<double, double, double>(),
            py::arg("x"), py::arg("y"), py::arg("z"))
        .def_readwrite("tensord3D", &sc::Element3D::tensor3D, "3D tensor of the element")
        .def_readwrite("point3D", &sc::Element3D::point3D, "3D point of the element")
        .def("xx", &sc::Element3D::xx, "The xx-component of the matrix.")
        .def("xy", &sc::Element3D::xy, "The xy-component of the matrix.")
        .def("xz", &sc::Element3D::xz, "The xz-component of the matrix.")
        .def("yx", &sc::Element3D::yx, "The yx-component of the matrix.")
        .def("yy", &sc::Element3D::yy, "The yy-component of the matrix.")
        .def("yz", &sc::Element3D::yz, "The yz-component of the matrix.")
        .def("zx", &sc::Element3D::zx, "The zx-component of the matrix.")
        .def("zy", &sc::Element3D::zy, "The zy-component of the matrix.")
        .def("zz", &sc::Element3D::zz, "The zz-component of the matrix.")
        .def("x", &sc::Element3D::x, "The x-component of the matrix.")
        .def("y", &sc::Element3D::y, "The y-component of the matrix.")
        .def("z", &sc::Element3D::z, "The z-component of the matrix.")
        .def("CGAL2AC", &sc::Element3D::CGAL2AC, "Convert matrix from CGAL Coordinate system to AC Coordinate system.")
        .def("AC2CGAL", &sc::Element3D::AC2CGAL, "Convert matrix from AC Coordinate system to CGAL Coordinate system.")
        .def("CS12CS2", &sc::Element3D::CS12CS2,
            py::arg("theta_x"), py::arg("theta_y"), py::arg("theta_z"),
            "Convert matrix from Coordinate System 1 to Coordinate System 2 using a 123 transform.");
}