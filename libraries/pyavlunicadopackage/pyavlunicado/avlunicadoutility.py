# UNICADO - UNIversity Conceptual Aircraft Design and Optimization
#
# Copyright (C) 2025 UNICADO consortium
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# Description:
# This file is part of UNICADO.

"""
PyAvlUnicado - A Python Interface for Athena Vortex Lattice (AVL) for use with UNICADO

This module provides utility functions for handling aircraft geometry data,
specifically for interacting with aircraft XML files and extracting aerodynamic
surface information.

Functions:
    - get_element_path(element, tree): Retrieves the hierarchical path of an XML element.
    - load_aerodynamic_surfaces(paths, component_names): Loads aerodynamic surfaces from aircraft XML data.
    - load_aerodynamic_surfaces_from_information(paths, aerodynamic_surfaces_information): Extracts aerodynamic surface details.
    - aerodynamic_surfaces_from_component_available(component): Identifies aerodynamic surfaces in a component.
    - load_reference_position(component): Extracts the reference position of a component.
"""

import pyaircraftgeometry2 as geom2
import pyaixml as aixml


def get_element_path(element, tree):
    """
    Retrieves the hierarchical path of an XML element relative to the root.

    Args:
        element (ET.Element): The XML element whose path is to be determined.
        tree (ET.Element): The root of the XML tree.

    Returns:
        str: The hierarchical path of the element.
    """
    element_path = []
    while element != tree:
        parent = tree.find(f".//{element.tag}/..")
        element_path.append(element.tag)
        element = parent
    element_path.reverse()
    return '/'.join(element_path)


def load_aerodynamic_surfaces(paths, component_names: list[str]=[]):
    """
    Loads aerodynamic surface data from aircraft XML files.

    Args:
        paths (dict): Dictionary containing paths to aircraft exchange files.
        component_names (list[str], optional): List of component names to extract data from.

    Returns:
        dict: A dictionary containing aerodynamic surface details for each component.

    Raises:
        RuntimeError: If component_names is not a valid list of strings.
    """
    component_design_items = None
    if component_names or isinstance(component_names, list) and all(isinstance(name, str) for name in component_names):

        if component_names:
            component_design_items = []
            for component_name in component_names:
                component_design_items.append(paths["root_of_aircraft_exchange_tree"].find(
                    f".//component_design/{component_name}"))

        else:
            component_design_items = paths["root_of_aircraft_exchange_tree"].find('.//component_design')

        aerodynamic_surfaces = {}
        if component_design_items is not None:
            for component in component_design_items:
                info = aerodynamic_surfaces_from_component_available(component)
                if info is not None:
                    aerodynamic_surfaces[component.tag] = {
                        "aerodynamic_surfaces": load_aerodynamic_surfaces_from_information(paths, info),
                        "reference_position": load_reference_position(component),
                        "info": info
                    }
        return aerodynamic_surfaces
    else:
        raise RuntimeError(
            f"component_names does not match required type list - current type: {type(component_names)}")


def load_aerodynamic_surfaces_from_information(paths, aerodynamic_surfaces_information):
    """
    Extracts aerodynamic surfaces from the given aircraft XML data.

    Args:
        paths (dict): Dictionary containing paths to aircraft exchange files.
        aerodynamic_surfaces_information (list): List of aerodynamic surface information.

    Returns:
        list: A list of aerodynamic surfaces including geometry and control devices.
    """
    acxml = aixml.openDocument(paths["path_to_aircraft_exchange_file"])
    aerodynamic_surfaces = []
    for aerodynamic_surface_info in aerodynamic_surfaces_information:
        aerodynamic_surface = {}
        aerodynamic_surface["geometry"] = geom2.factory.WingFactory(
            acxml, paths["airfoil_data_directory"]).create(aerodynamic_surface_info["geometry_path"])

        aerodynamic_surface["control_devices"] = []
        for control_device in aerodynamic_surface_info["control_devices"]:
            device = geom2.factory.ControlDeviceFactory(acxml, paths["airfoil_data_directory"]).create(control_device["path"])
            device.name = control_device["name"]
            aerodynamic_surface["control_devices"].append(device)
        aerodynamic_surfaces.append(aerodynamic_surface)
    return aerodynamic_surfaces


def aerodynamic_surfaces_from_component_available(component):
    """
    Identifies aerodynamic surfaces in an aircraft component.

    Args:
        component (ET.Element): XML element representing the aircraft component.

    Returns:
        list or None: A list of aerodynamic surfaces found in the component, or None if no surfaces are available.
    """
    components_aerodynamic_surfaces = component.findall(".//aerodynamic_surface[@ID]")
    if components_aerodynamic_surfaces:
        aerodynamic_surfaces_available = []

        for components_aerodynamic_surface in components_aerodynamic_surfaces:
            aerodynamic_surface_available = {}
            aerodynamic_surface_available["geometry_path"] = f"{component.tag}/{get_element_path(components_aerodynamic_surface, component)}@{components_aerodynamic_surface.attrib['ID']}"
            aerodynamic_surface_available["control_devices"] = []

            # look for control devices
            control_devices = components_aerodynamic_surface.findall(".//control_device[@ID]")
            if control_devices:
                for control_device in control_devices:
                    control_device_info = {
                        "path": f"{aerodynamic_surface_available['geometry_path']}/{get_element_path(control_device, components_aerodynamic_surface)}@{control_device.attrib['ID']}",
                        "name": control_device.attrib["description"]
                    }
                    aerodynamic_surface_available["control_devices"].append(control_device_info)
            aerodynamic_surfaces_available.append(aerodynamic_surface_available)
        return aerodynamic_surfaces_available
    else:
        return None

def load_reference_position(component):
    """
    Extracts the reference position of an aircraft component.

    Args:
        component (ET.Element): XML element representing the aircraft component.

    Returns:
        dict: Dictionary containing x, y, and z coordinates of the reference position.
    """
    position = {
        "x": float(component.find("./position/x/value").text),
        "y": float(component.find("./position/y/value").text),
        "z": float(component.find("./position/z/value").text)
    }

    return position

