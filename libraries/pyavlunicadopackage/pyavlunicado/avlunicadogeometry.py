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
PyAvlUnicado - Geometry Processing for AVL

This module defines classes for handling aerodynamic surfaces and profile scaling within the AVL simulation framework.

Features:
- Constructs AVL-compatible aerodynamic surface representations.
- Computes 3D transformations and rotations.
- Integrates with `pyaircraftgeometry2` for wing and control surface geometry.
- Scales airfoil profiles for AVL using direct geometric transformations based on XFOIL code.

Dependencies:
- numpy
- pyaircraftgeometry2
- pyavl
"""
import pyaircraftgeometry2 as geom2
import pyavl as avl
import os

import shutil
import numpy as np

from collections import defaultdict
from .xfoilgeometryscale import *

class AerodynamicSurface:
    """
    Represents an aerodynamic surface for AVL simulations.

    This class constructs an AVL-compatible aerodynamic surface from aircraft geometry data,
    allowing for transformations, profile scaling, and control device integration.
    """
    def __init__(self, paths, folder_to_create_data: str = ".") -> None:
        """
        Initializes an aerodynamic surface object.

        Args:
            paths (dict): Dictionary containing paths to required data.
            folder_to_create_data (str, optional): Path where processed airfoil data will be stored. Defaults to "./".
        """
        self.aircraft_name = None
        self.surface = None
        self.paths = paths
        self.rotation_geom2_to_avl = None
        self.folder_to_create_data = folder_to_create_data

    def rotation_matrix(self, input_vector, target_vector):
        """
        Computes the rotation matrix that aligns one 3D vector with another.

        Args:
            input_vector (array-like): The initial 3D vector.
            target_vector (array-like): The target 3D vector.

        Returns:
            numpy.ndarray: A 3x3 rotation matrix.
        """

        # w_hat = np.cross(input_vector, target_vector)

        theta = np.arccos(np.dot(input_vector, target_vector))

        c = np.cos(theta)
        s = np.sin(theta)
        R = np.array([[1, 0, 0], [0, c, -s], [0, s, c]])

        return R

    def build(self, reference_position, aerodynamic_surface, include_control_devices=True):
        """
        Constructs the AVL-compatible aerodynamic surface from the given input data.

        Args:
            reference_position (dict): Reference position of the aerodynamic surface.
            aerodynamic_surface (dict): Dictionary containing geometry and control device data.
            include_control_devices (bool, optional): Whether to include control devices. Defaults to True.

        Returns:
            avl.Surface: The constructed AVL aerodynamic surface.
        """
        geometry = aerodynamic_surface["geometry"]
        controls = aerodynamic_surface["control_devices"]

        # Determine rotation matrix for geometry origin -> necessary due to extrusion of aerodynamic objects according to aircraftgeometry2
        geometry_normal = np.array([geometry.normal.dx(), geometry.normal.dy(), geometry.normal.dz()])
        extrusion_normal = np.array([0, 0, 1])
        self.origin_unicado_to_avl = self.rotation_matrix(extrusion_normal, geometry_normal)

        # Initialize avl sections list
        avl_sections = []

        # Set number of geometry sections
        number_of_geometry_sections = len(geometry.sections)

        for id in range(number_of_geometry_sections):

            section = geometry.sections[id]
            chord = section.get_chord_length()

            # leading_edge_reference = np.dot(self.origin_unicado_to_avl, np.array(
            # [section.origin.x(), section.origin.y(), section.origin.z()]))
            leading_edge_reference = self.xyz_unicado_to_avl(
                [section.origin.x(), section.origin.y(), section.origin.z()])  # leading_edge_reference.tolist()

            # remove small elements from leading edge reference to avoid asymmetric behaviour
            leading_edge_reference = [0.0 if abs(elem) < 1E-6 else elem for elem in leading_edge_reference]

            scaling = section.get_thickness_scale()

            # Profile scaling

            # XFoil profile destination
            xfoil_destination_dir = f"{self.folder_to_create_data}/profiles"
            # Create xfoil_destintation_dir if not existing
            os.makedirs(xfoil_destination_dir, exist_ok=True)
            profile_path = f"{self.paths['airfoil_data_directory']}/{section.name}.dat"
            xfoil_profile_path = f"{xfoil_destination_dir}/{section.name}.dat"

            # Copy profile to xfoil_destination_dir -> needs to be copied due to limitation of profiles
            # todo -> remove xfoil usage by scaling thickness directly
            shutil.copy(profile_path, xfoil_profile_path)

            section_profile_file_name = Profile(xfoil_profile_path, geometry.name,
                                                "geometry", id).save_to_file(xfoil_destination_dir, scaling)

            avl_section = avl.Section(refLE=leading_edge_reference, chord=chord,
                                      dihedral=0.0, nspan=5, afileKey="AFILE", afile=section_profile_file_name)

            avl_sections.append(avl_section)

        if include_control_devices:
            avl_sections = self.build_control_sections(avl_sections, geometry, controls)

        self.surface = avl.Surface(tag=geometry.name)

        for section in avl_sections:
            self.surface.add_section(section)
            # Do translation, angle etc at end
        self.surface.add_translate(avl.Translate([reference_position["x"] + geometry.origin.x(),
                                                  reference_position["y"] + geometry.origin.y(),
                                                  reference_position["z"] + geometry.origin.z()]))
        self.surface.add_angle(avl.Angle(geometry.rotation_z))
        if geometry.is_symmetric:  # symmetric
            self.surface.add_yduplicate(avl.Yduplicate(0.0))

        return self.surface

    def xyz_unicado_to_avl(self, geom2_xyz=[]):
        """
        Converts coordinates from UNICADO format to AVL format.

        Args:
            geom2_xyz (list, optional): List of x, y, z coordinates in UNICADO format.

        Returns:
            list: Transformed coordinates in AVL format.
        """
        return np.dot(self.origin_unicado_to_avl, geom2_xyz).tolist()

    def build_control_sections(self, avl_sections, geometry, controls):
        """
        Builds control surface sections for AVL.

        Args:
            avl_sections (list): List of existing AVL sections.
            geometry (object): Aircraft geometry object.
            controls (list): List of control surfaces.

        Returns:
            list: Updated AVL sections with control devices.
        """
        half_span = geom2.measure.span(geometry)
        if geometry.is_symmetric:
            half_span *= 0.5
        sections_available = [section.origin.z() for section in geometry.sections]

        # Set control surface vector parallel to surface normal
        xyzhvec = [abs(geometry.normal.dx()), abs(geometry.normal.dy()), abs(geometry.normal.dz())]

        for control in controls:
            for idx, section in enumerate(control.sections):
                # Build section
                position_spanwise = abs(half_span * section.origin.z())*-1.0
                current_id = None
                for id, position in enumerate(sections_available):
                    if self.float_equality(position, position_spanwise):
                        current_id = id
                        break
                    else:
                        continue

                # if current position is not existing -> create section
                if current_id is None:
                    sections_available.append(position_spanwise)
                    origin = geom2.measure.offset_LE(geometry, position_spanwise)
                    leading_edge_reference = self.xyz_unicado_to_avl([origin.x(), origin.y(), origin.z()])
                    leading_edge_reference = [0.0 if abs(elem) < 1E-6 else elem for elem in leading_edge_reference]

                    id_left, id_right = self.find_adjacent_sections(geometry, position_spanwise)
                    position_scale, position_profile = self.find_thickness_scaling_and_profile(
                        geometry, id_left, id_right, position_spanwise)

                    chord = geom2.measure.chord(geometry, position_spanwise)

                    # Profile scaling
                    # XFoil profile destination
                    xfoil_destination_dir = f"{self.folder_to_create_data}/profiles"
                    # Create xfoil_destintation_dir if not existing
                    os.makedirs(xfoil_destination_dir, exist_ok=True)
                    profile_path = f"{self.paths['airfoil_data_directory']}/{position_profile}.dat"
                    xfoil_profile_path = f"{xfoil_destination_dir}/{section.name}.dat"

                    # Copy profile to xfoil_destination_dir -> needs to be copied due to limitation of profiles
                    # todo -> remove xfoil usage by scaling thickness directly
                    shutil.copy(profile_path, xfoil_profile_path)

                    section_profile_file_name = Profile(xfoil_profile_path, geometry.name,
                                                        control.name, idx).save_to_file(xfoil_destination_dir, position_scale)

                    avl_section = avl.Section(refLE=leading_edge_reference, chord=chord,
                                              dihedral=0.0, nspan=5, afileKey="AFILE", afile=section_profile_file_name)

                    contour = section.get_contour(False)


                    # Hinge line according to avl documentation
                    # LE Surface 0 ... -xhinge
                    # TE Surface xhinge ... 1

                    # Assume trailing edge device
                    xhinge = contour.vertex(0).x()
                    # Change if xhinge is at zero
                    if abs(xhinge) < 1E-4:
                        xhinge = -contour.vertex(1).x()
                    # Direction for control surface - according to avl documentation
                    sgndup = 1
                    gain = 1
                    if control.name.startswith("aileron"):
                        sgndup = -1
                    if control.name.startswith("rudder") or control.name.startswith("slat"):
                        gain = -1

                    # Create avl control device
                    control_device = avl.Control(tag=control.name, gain=gain, xyzhvec=xyzhvec,
                                                 xhinge=xhinge, sgndup=sgndup)

                    # Add control device section
                    avl_section.add_control_device(control_device)
                    avl_sections.append(avl_section)
                else:
                    # add_control to section
                    contour = section.get_contour(False)
                    # Hinge line according to avl documentation
                    # LE Surface 0 ... -xhinge
                    # TE Surface xhinge ... 1

                    # Assume trailing edge device
                    xhinge = contour.vertex(0).x()
                    # Change if xhinge is at zero
                    if abs(xhinge) < 1E-4:
                        xhinge = -contour.vertex(1).x()
                    # Direction for control surface - according to avl documentation
                    sgndup = 1
                    gain = 1
                    if control.name.startswith("aileron_") or control.name == "aileron":
                        sgndup = -1
                    if control.name.startswith("rudder") or control.name.startswith("slat"):
                        gain = -1

                    control_device = avl.Control(tag=control.name, gain=gain, xyzhvec=xyzhvec,
                                                 xhinge=xhinge, sgndup=sgndup)
                    avl_sections[current_id].add_control_device(control_device)

        # Sort avl_sections
        sorted_section_idx = sorted(range(len(sections_available)), key=lambda x: sections_available[x], reverse=True)
        reordered_sections = [avl_sections[i] for i in sorted_section_idx]

        # Get control section ids start + end
        control_devices = defaultdict(list)
        for idx, section in enumerate(reordered_sections):
            for control in section.controls:
                control_devices[control.tag].append(idx)

        # Add control to section if control sections not next to eachother
        for device, boundary_ids in control_devices.items():
            first_section_id = boundary_ids[0]
            last_section_id = boundary_ids[-1]
            missing_section_ids = list(range(first_section_id+1,last_section_id))

            # Append missing control to existing section
            boundary_controls = [control for controls in [reordered_sections[first_section_id].controls, reordered_sections[last_section_id].controls] for control in controls if control.tag == device]
            for id in missing_section_ids:
                # Get first and last control devices
                first_hinge = boundary_controls[0].xhinge
                first_gain = boundary_controls[0].gain
                last_hinge = boundary_controls[-1].xhinge
                last_gain = boundary_controls[-1].gain

                control_to_add = boundary_controls[0]
                if not self.float_equality(first_hinge, last_hinge):
                    # Get first and last position in main extrusion direction -> xyzhvec
                    first_pos = [reordered_sections[first_section_id].ref[i] for i, val in enumerate(xyzhvec) if abs(val) > 0.9][0]
                    last_pos = [reordered_sections[last_section_id].ref[i] for i, val in enumerate(xyzhvec) if abs(val) > 0.9][0]

                    current_pos = [reordered_sections[id].ref[i] for i, val in enumerate(xyzhvec) if abs(val) > 0.9][0]

                    t = (current_pos-first_pos)/(last_pos-first_pos)
                    current_xhinge = self.lerp(first_hinge, last_hinge, t)
                    current_gain = self.lerp(first_gain,last_gain, t)
                    control_to_add.xhinge = current_xhinge
                    control_to_add.gain = current_gain

                reordered_sections[id].controls.append(control_to_add)

            # Assume that a device is always unique (e.g. slats are not separated -> otherwise it would be slat_1, slat_2 etc. )



        return reordered_sections

    def lerp(self, a: float, b: float, t: float) -> float:
        """
        Performs linear interpolation between two values.

        Args:
            a (float): The starting value.
            b (float): The ending value.
            t (float): The interpolation factor (0 ≤ t ≤ 1).

        Returns:
            float: The interpolated value.
        """
        return (1-t)* a + t*b

    def float_equality(self, value, to_check, tol=1e-4):
        """
        Checks if two floating-point numbers are equal within a given tolerance.

        Args:
            value (float): The first number to compare.
            to_check (float): The second number to compare.
            tol (float, optional): The allowed tolerance for equality. Defaults to 1e-4.

        Returns:
            bool: True if values are considered equal, otherwise False.
        """
        return abs(value-to_check) < tol


    def find_adjacent_sections(self, geometry, position):
        """
        Finds the indices of the two adjacent sections in the given geometry closest to the specified position.

        Args:
            geometry (object): The geometry containing sections.
            position (float): The spanwise position to find the nearest sections.

        Returns:
            tuple[int, int]: Indices of the left and right adjacent sections.
        """
        section_positions = [section.origin.z() for section in geometry.sections]
        id_left = 0
        id_right = 1
        if not position in section_positions:
            for idx, section_position in enumerate(section_positions):
                if position <= section_position:
                    id_left = idx
                    id_right = idx+1

        return id_left, id_right

    def find_thickness_scaling_and_profile(self, geometry, left_section_id, right_section_id, position):
        """
        Determines the thickness scaling and profile name based on the given spanwise position.

        Args:
            geometry (object): The aerodynamic geometry.
            left_section_id (int): The index of the left section.
            right_section_id (int): The index of the right section.
            position (float): The spanwise position to evaluate.

        Returns:
            tuple[float, str]: The interpolated thickness scaling factor and the selected airfoil profile name.
        """
        left_section = geometry.sections[left_section_id]
        left_section_profile = left_section.name
        left_section_scale = left_section.get_thickness_scale()

        right_section = geometry.sections[right_section_id]
        right_section_profile = right_section.name
        right_section_scale = right_section.get_thickness_scale()

        normalized_position = (position-left_section.origin.z())/(right_section.origin.z()-left_section.origin.z())

        position_scale: float = left_section_scale * \
            (1 - normalized_position) + right_section_scale * normalized_position
        position_profile: str = left_section_profile
        if right_section_profile != left_section_profile:
            if normalized_position > 0.5:
                position_profile = right_section_profile
            else:
                position_profile = left_section_profile

        return position_scale, position_profile

        # left_section_profile



class Profile:
    """Profile storage and scaling class."""
    def __init__(self, profile_file: str, section_geometry_name: str, section_type: str = "geometry", section_id: int = 0) -> None:
        """
        Initializes the Profile class.

        Args:
            profile_file (str): The filename of the profile.
            section_geometry_name (str): The name of the geometry section.
            section_type (str, optional): The type of the section. Defaults to "geometry".
            section_id (int, optional): The ID of the section. Defaults to 0.
        """

        self.profile_file = profile_file
        self.section_id = section_id
        self.output_file = f"section_{section_geometry_name}_{section_type}_{section_id}.dat"

    def remove_existing_profile(self, path: str) -> None:
        """
        Removes an existing profile file if it exists.

        Args:
            path (str): The directory path where the profile file is located.
        """
        path_to_file = f"{path}/{self.output_file}"
        if os.path.exists(path_to_file):
            os.remove(path_to_file)
        else:
            pass

    def save_to_file(self, path: str, scale: float = 1.) -> str:
        """
        Applies thickness scaling to the profile and saves the modified profile.

        Args:
            path (str, optional): The directory path for saving the scaled profile. Defaults to an empty string.
            scale (float, optional): The scaling factor for thickness. Defaults to 1.0.
        """

        self.remove_existing_profile(path)

        # Scale thickness
        self.run_thickness_scaling(path, scale)
        return f"{path}/{self.output_file}"

    def run_thickness_scaling(self, path:str="", scale: float=1.0) -> None:
        """ Runs thickness scaling

        Args:
            path (str, optional): path to airfoil directory. Defaults to "".
            scale (float, optional): scales thickness according to value. Defaults to 1.0.
        """
        output_path = f"{path}/{self.output_file}"

        coordinates = load_profile(self.profile_file)
        scaled_coordinates = rescale_profile(coordinates=coordinates, scale_t_to_c=scale)

        save_profile(scaled_coordinates, output_path)
