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

This module provides a Python interface for generating and executing AVL simulations. It includes:
- Geometry initialization and manipulation
- Run case setup and execution
- Interaction with AVL for aerodynamic analysis

Features:
- Reads and processes aircraft geometry
- Manages aerodynamic surfaces and control devices
- Creates AVL input files for simulations
- Automates AVL run case execution and result extraction

Dependencies:
- numpy
- ambiance
- pyaircraftgeometry2
- pyavl
"""
import pyaircraftgeometry2 as geom2
import pyavl as avl
import os
import shutil
from .avlunicadogeometry import AerodynamicSurface
from .avlunicadoutility import load_aerodynamic_surfaces
from ambiance import Atmosphere
import numpy as np


class Avlinterface:
    """
    Manages the interface for AVL simulations, including geometry initialization and run case management.
    """
    aerodynamic_surfaces = None
    aerodynamic_components: list=[]
    aerodynamic_reference_data: list=[]
    aerodynamic_surface_control_keys = set()
    paths: str=None
    aircraft_name: str=None
    runcases: list=[]
    derivative_data = None
    none_high_lift_devices: list = ["aileron", "elevator", "rudder"]

    def __init__(self, paths: str = None, aircraft_name: str = "default_ac") -> None:
        """
        Initializes the Avlinterface class.

        Args:
            paths (str, optional): Path to aircraft geometry files.
            aircraft_name (str, optional): Name of the aircraft model.
        """
        self.paths = paths
        self.aircraft_name = aircraft_name

    def initialize_geometry(self, component_list: list[str] = [], use_control_devices=False, use_high_lift_devices=False):
        """
        Initializes aerodynamic geometry based on input components.

        Args:
            component_list (list, optional): List of components to include.
            use_control_devices (bool, optional): Whether to include control devices.
            use_high_lift_devices (bool, optional): Whether to include high lift devices.
        """
        # clean up aerodynamic components
        if self.aerodynamic_components:
            self.aerodynamic_components.clear()

        # Load aerodynamic surfaces from acxml
        self.aerodynamic_surfaces = load_aerodynamic_surfaces(self.paths, component_list)

        # Read reference data (S_ref, mac, Span)
        if "wing" in self.aerodynamic_surfaces:
            reference_area: float = 0
            reference_mac: float = 0
            reference_span: float = 0
            for aerodynamic_surface in self.aerodynamic_surfaces["wing"]["aerodynamic_surfaces"]:
                current_wing = aerodynamic_surface["geometry"]
                current_area = geom2.measure.reference_area(current_wing)

                # select reference values from biggest area
                if current_area > reference_area:
                    reference_area = current_area
                    reference_mac = geom2.measure.mean_aerodynamic_chord(current_wing)
                    reference_span = geom2.measure.span(current_wing)
                    reference_phi_25 = -geom2.measure.sweep(current_wing, 0.25*reference_span, 0.25)
                    self.aerodynamic_reference_data = [reference_area, reference_mac, reference_span, reference_phi_25]

        for component in self.aerodynamic_surfaces:
            # Read reference position
            component_reference_position = self.aerodynamic_surfaces[component]["reference_position"]
            for aerodynamic_surface in self.aerodynamic_surfaces[component]["aerodynamic_surfaces"]:
                
                # If use_high_lift_devices -> False ... only none_high_lift_devices are used for geometry, else all elements are used
                # WARNING -> if all devices are used, make sure that they are not too close to eachother, otherwise avl has issues with computation
                if not use_high_lift_devices:
                    aerodynamic_surface["control_devices"] = [control for control in aerodynamic_surface["control_devices"] if control.name in self.none_high_lift_devices]
                
                self.aerodynamic_components.append(AerodynamicSurface(self.paths, self.aircraft_name).build(
                    component_reference_position, aerodynamic_surface, use_control_devices))

                
                for control in aerodynamic_surface["control_devices"]:
                  self.aerodynamic_surface_control_keys.add(control.name)


    def create_avl_geometry_file(self, author: str = "", filename: str = "", runcasename: str = "default", mach_number: float = 0.78, reference_data: list[float] = None, reference_cog: list[float] = None):
        """
        Creates an AVL geometry input file.
        """
        if filename == "":
            filename = f"{self.aircraft_name}/{self.aircraft_name}"
        else:
            filename = f"{self.aircraft_name}/{filename}"
        if reference_data is None:
            reference_data = self.aerodynamic_reference_data
        if reference_cog is None:
            reference_cog = [0., 0., 0.]
            print(f"Reference center of gravity is None ... set to {reference_cog}")

        header_avl_file = [avl.Header(runcase=runcasename, mach=mach_number,
                                      sym=[0, 0, 0.0], base=reference_data[:3], ref=reference_cog)]

        avl.write_avl_file(author, header_avl_file + self.aerodynamic_components, filename)



    def add_runcase(self, runcase):
        """ Add runcase

        Args:
            runcase (object): Runcase
        """
        if not isinstance(runcase, AvlRuncase):
            print("Runcase will not be appended - no AvlRuncase...")

        else:
            self.runcases.append(runcase)

    def number_of_runcases(self):
        """Return numer of runcases"""
        return len(self.runcases)

    def runcase_ids(self):
        """Return list of runcase ids"""
        return [rc.runcase_id for rc in self.runcases]

    def define_runcase_batch(self, runcase_folder: str=".", alphas: list[float]=[0.0], betas: list[float]=[0.0], pb2vs: list[float]=[0.0], qc2vs: list[float]=[0.0], rb2vs: list[float]=[0.0], mach_numbers: list[float]=[0.0], CD0: float=0.02, heights_in_m: list[float]=[0.0], cog: list[float]=[0.0,0.0,0.0], controls: list[str]=[], clear_runcase_folder: bool=True):
        """
        Defines a batch of AVL run cases by varying aerodynamic parameters.

        Args:
            runcase_folder (str, optional): Path to store run case files. Defaults to ".".
            alphas (list[float], optional): List of angle of attack values. Defaults to [0.0].
            betas (list[float], optional): List of sideslip angle values. Defaults to [0.0].
            pb2vs (list[float], optional): List of roll rate values. Defaults to [0.0].
            qc2vs (list[float], optional): List of pitch rate values. Defaults to [0.0].
            rb2vs (list[float], optional): List of yaw rate values. Defaults to [0.0].
            mach_numbers (list[float], optional): List of Mach numbers. Defaults to [0.0].
            CD0 (float, optional): Zero-lift drag coefficient. Defaults to 0.02.
            heights_in_m (list[float], optional): List of altitude values in meters. Defaults to [0.0].
            cog (list[float], optional): Center of gravity coordinates [x, y, z]. Defaults to [0.0, 0.0, 0.0].
            controls (list[str], optional): List of control surface names. Defaults to an empty list.
            clear_runcase_folder (bool, optional): Whether to clear the run case folder before creation. Defaults to True.
        """
        runcase_id = self.number_of_runcases() + 1
        print("Clear existing runcases ...")
        self.runcases.clear()
        if os.path.isdir(runcase_folder) and clear_runcase_folder:
            if not self.is_directory_empty(runcase_folder):
                self.empty_directory(runcase_folder)

        # Create list from set
        if len(controls) == 0:
            controls=list(self.aerodynamic_surface_control_keys)

        # Create different runcase batch
        for mach_number, height_in_m in zip(mach_numbers,heights_in_m):
            for rb2v in rb2vs:
                for qc2v in qc2vs:
                    for pb2v in pb2vs:
                        for beta in betas:
                            for alpha in alphas:
                                self.runcases.append(AvlRuncase(runcase_folder, runcase_id, f"RunCase {runcase_id} - {height_in_m:.2f}", alpha, beta, pb2v, qc2v, rb2v, mach_number*np.cos(self.aerodynamic_reference_data[3]), CD0, height_in_m, cog, controls).create())
                                runcase_id += 1

        print(f"Current number of Runcases: ... {self.number_of_runcases()}")

    def run_avl(self, runcase_folder, stability_files_folder, results_folder, derivative_collection_file=None):
        """
        Executes an AVL simulation using the specified run cases and stability file storage.

        Args:
            runcase_folder (str): Path to the folder containing AVL run case files.
            stability_files_folder (str): Path to the folder where AVL stability results will be stored.
            results_folder (str): Path to the folder where final results will be saved.
            derivative_collection_file (str, optional): Name of the output CSV file to store aerodynamic derivatives.

        Returns:
            avl.PyAvlStabilityFilesConvert or None: Returns processed aerodynamic derivatives if `derivative_collection_file` is provided, otherwise None.
        """
        geometry_file_name = f"{self.aircraft_name}.avl"
        runner = avl.pyavlrunner.PyAvlRunner(geometry_file_name)
        runner.session_setup_single_rc(path_to_session_folder=self.aircraft_name, runcase_files_folder=runcase_folder, stability_files_folder=stability_files_folder, results_folder=results_folder, runcase_ids=self.runcase_ids())
        runner.run_avl_session()

        if isinstance(derivative_collection_file, str):

            os.makedirs(results_folder, exist_ok=True)
            stability_files = [f"{runner.stability_files_folder()}/{file}" for file in os.listdir(runner.stability_files_folder()) if file.endswith(".stab")]
            self.derivative_data = avl.PyAvlStabilityFilesConvert(filenames=stability_files,
                                           keyListControls=list(self.aerodynamic_surface_control_keys), fileout=f"{results_folder}/{derivative_collection_file}.csv")
        return self.derivative_data

    def is_directory_empty(self,directory):
        """Check whether a directory is empty or ot"""
        return not any(os.scandir(directory))

    def empty_directory(self,directory):
        """ Empty directory

        Args:
            directory (str): removes the content of this directory
        """
        for item in os.listdir(directory):
            item_path = os.path.join(directory, item)
            if os.path.isfile(item_path):
                os.remove(item_path)
            elif os.path.isdir(item_path):
                shutil.rmtree(item_path)

class AvlRuncase:
    """
    Defines an AVL run case with aerodynamic parameters.
    """
    def __init__(self, runcase_folder: str=".", runcase_id: int=1, runcase_name: str="default_runcase", alpha: float=0.0, beta: float=0.0, pb2v: float=0.0, qc2v: float=0.0, rb2v: float=0.0, mach_number: float=0.0, CD0: float=0.02, height_in_m: float=0.0, cog: list[float]=[0.0,0.0,0.0], controls=None):
        """
        Initializes an AVL run case.
        """
        self.runcase_directory = runcase_folder
        self.runcase_id = runcase_id
        self.runcase_name = runcase_name
        self.runcase_file: str= f"rc_{runcase_id:03}.run"
        self.alpha = alpha
        self.beta = beta
        self.pb2v = pb2v
        self.qc2v = qc2v
        self.rb2v = rb2v
        self.controls = controls
        self.CD0 = CD0
        self.mach_number = mach_number
        self.altitude = height_in_m
        self.density = Atmosphere(self.altitude).density
        self.speed_of_sound = Atmosphere(self.altitude).speed_of_sound
        self.velocity = mach_number * self.speed_of_sound
        self.cog = cog

    def create(self):
        """
        Creates an AVL run case file in the specified directory.
        """
        # Create runcase directory
        os.makedirs(self.runcase_directory, exist_ok=True)

        full_path_to_runcase = f"{self.runcase_directory}/{self.runcase_file}"
        if os.path.exists(full_path_to_runcase):
            os.remove(full_path_to_runcase)
        with open(full_path_to_runcase, "+w") as rcfile:
            avl.write_runcase(file=rcfile, runcasenum=1, runcasename=self.runcase_name, alpha=self.alpha, beta=self.beta, pb2v=self.pb2v, qc2v=self.qc2v, rb2v=self.rb2v, mach=self.mach_number, CD0=self.CD0, density=self.density, a=self.speed_of_sound, cgref=self.cog, controls=self.controls)
        return self
