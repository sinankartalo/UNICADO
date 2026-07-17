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
#include <pybind11/stl/filesystem.h>
#include "aixml/node.h"

namespace py = pybind11;

/* === Helper functions === */
/**
 * @brief Helper function to insert a Python ElementTree object into a node.
 * This function recursively inserts the children of the element and copies
 * the attributes.
 * @param element The Python `ElementTree.Element` object.
 * @param insert_into The pointer to the node to insert the element(s) into.
 */
void insert_element(const py::object &element, node *const insert_into)
{
    /* Insert the node */
    node *new_node = &insert_into->appendChild(element.attr("tag").cast<std::string>());

    /* Set the node value */
    *new_node = element.attr("text").cast<std::string>();

    /* Update the attributes */
    py::dict attributes = element.attr("attrib");
    for (auto item : attributes) {
        new_node->addAttrib(item.first.cast<std::string>(), item.second.cast<std::string>());
    }
    
    /* Insert the children recursively */
    for (auto& child : element) {
        insert_element(py::cast<py::object>(child), new_node);
    }
}

/**
 * @brief Helper function to convert a Python ElementTree structure into a node object.
 * @param root The Python `ElementTree.Element` root element of the tree.
 * @return The node object.
 */
auto from_ElementTree(const py::object &root_element) -> std::shared_ptr<node> 
{
    /* Create a new node */
    auto root_node = std::make_shared<node>();

    /* Insert the children */
    insert_element(root_element, root_node.get());

    /* Return the node */
    return root_node;
}

/**
 * @brief Helper function to insert a node into a Python ElementTree object.
 * This function recursively inserts the children of the node and copies
 * the attributes.
 * @param node The node to insert.
 * @param insert_into The Python `ElementTree.Element` object to insert the node into.
 */
void insert_node(const node &node, py::object &insert_into)
{
    /* Get the attributes of the node */
    py::dict attributes{};
    for (auto& [key, value] : node.getAttribMap())
    {
        attributes.attr("__setitem__")(key, value);
    }

    /* Insert the new element */
    py::object ET_Element = py::module::import("xml.etree.ElementTree").attr("Element");
    insert_into.attr("append")(ET_Element(node.getName(), attributes));

    /* Insert the children recursively */
    for (auto& child : node.getChildren())
    {
        py::object element = insert_into.attr("__getitem__")(-1);
        insert_node(*child, element);
    }
}

/**
 * @brief Helper function to convert a node object into a Python ElementTree object.
 * @param root_node The node object.
 * @return The Python `ElementTree.Element` root element of the tree.
 */
auto to_ElementTree(node &root_node) -> py::object
{
    /* Create a new element */
    py::object ET_Element = py::module::import("xml.etree.ElementTree").attr("Element");
    auto root_element = ET_Element(root_node.getName());

    /* Insert the children recursively */
    insert_node(root_node, root_element);

    /* Return the element */
    return root_element;
}

/* === Python bindings === */
PYBIND11_MODULE(py11aixml, m)
{
    /* Module docstring */
    m.doc() = "Python bindings for aixml.";

    /* Add the document interface */
    m.def(
        "openDocument",
        py::overload_cast<const std::filesystem::path &>(&aixml::openDocument),
        py::arg("fileName"),
        "Open an aixml document.")
    .def(
        "saveDocument",
        aixml::saveDocument,
        py::arg("root"),
        py::arg("toolLevel") = 0,
        "Function saves the XML-tree to the file, throws error if toolLevel is smaller than the ToolLevel-attribute in a modified element");

    /* Add the node class with its shared pointer */
    py::class_<node, std::shared_ptr<node>>(m, "Node")
        .def(py::init<>())
        .def(
            "at",
            &node::at,
            py::arg("path"),
            py::arg("depth") = static_cast<py::int_>(std::numeric_limits<int>::max()),
            py::return_value_policy::reference,
            "Returns a reference to the node matching the specified path.")
        .def(
            "__float__",
            [](node &n) { return static_cast<double>(n); },
            "Converts the node to a float.")
        .def(
            "__int__",
            [](node &n) { return static_cast<int>(n); },
            "Converts the node to an int.")
        .def(
            "__str__",
            [](node &n) { return static_cast<std::string>(n); },
            "Converts the node to a string.");

    /* Create a node object from a Python ElementTree object */
    m.def(
        "from_ElementTree",
        from_ElementTree,
        py::arg("root_element"),
        py::return_value_policy::move,
        "Creates a node object from a Python `ElementTree.Element` object.");
    
    /* Create a Python ElementTree object from a node object */
    m.def(
        "to_ElementTree",
        to_ElementTree,
        py::arg("root_node"),
        py::return_value_policy::move,
        "Creates a Python `ElementTree.Element` object from a node object.");
}
