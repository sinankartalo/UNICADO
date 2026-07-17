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
Python AVL Class for Reading Data

This module provides classes to process AVL stability files and extract relevant aerodynamics data.
"""
import numpy as np
import re
import csv
import math

class PyAvlStabilityFilesConvert:
    """
    Converts AVL stability files to a structured format and writes them to a CSV file.

    Attributes:
        filenames (list): List of AVL stability filenames.
        datatable (numpy.ndarray): Table of extracted data.
        keys (list): List of keys corresponding to the data columns.
        stabfiles (list): List of PyAvlStabilityFile objects.
        keyListControls (list, optional): List of control keys.
    """
    def __init__(self, filenames=[], keyListControls=None, fileout="default.csv"):
        """
        Initializes the PyAvlStabilityFilesConvert class and processes the given stability files.

        Args:
            filenames (list): List of filenames to process.
            keyListControls (list, optional): List of control keys.
            fileout (str): Output CSV filename.
        """
        filenames.sort()
        self.filenames = filenames
        self.datatable = None
        self.keys = None
        self.stabfiles = []
        self.keyListControls = keyListControls
        for filename in self.filenames:
            self.stabfiles.append(PyAvlStabilityFile(filename, keylistControls=keyListControls))

        with open(fileout, "w", newline='') as file:
            if self.keys is None:
                self.keys = self.stabfiles[0].get_keys()

            writer = csv.DictWriter(file, fieldnames=self.keys)
            writer.writeheader()
            datatable = []
            for stabfile in self.stabfiles:
                writer.writerow(stabfile.get_data())
                tmp = stabfile.get_data()
                datatable.append(list(tmp.values()))
            self.datatable = np.array(datatable)

    def get_keys(self):
        """Returns the list of keys from the stability files."""
        return self.keys

    def get_data(self):
        """Returns the numerical data extracted from the stability files."""
        return self.datatable

    def get_data_by_key(self,key):
        """
        Retrieves data for a specific key.

        Args:
            key (str): Key for which data is requested.

        Returns:
            numpy.ndarray or None: Data for the given key, or None if not found.
        """
        try:
            key_index = self.keys.index(key)
            return self.datatable[:,key_index]
        except ValueError:
            return None


    def get_data_by_control_key(self,key: str ,controlkey: str, exact: bool=False):
        """
        Retrieves data based on control key modifications.

        Args:
            key (str): The aerodynamic coefficient key.
            controlkey (str): The control surface key.
            exact (bool, optional): Whether to look for an exact match of the control key.

        Returns:
            numpy.ndarray or None: The extracted data.
        """
        available_keys = ["CL", "CY", "Cl", "Cm", "Cn","CDff"]
        try:
            # Get key in available key list -> set to 1 since zero would be direct control key
            available_key_index = available_keys.index(key) + 1
            if not exact:
                control_key_indices = [idx + available_key_index for idx, item in enumerate(self.keys) if item.startswith(controlkey)]
                # if more than one key index is available
                if len(control_key_indices) > 1:
                    # Combine data
                    return np.sum([self.datatable[:, idx]*math.degrees(1) for idx in control_key_indices], axis=1)
                else:
                    # else
                    return self.datatable[:, control_key_indices[0]]*math.degrees(1)
            else:
                control_key_index = self.keys.index(controlkey) + available_key_index
                return self.datatable[:, control_key_index]*math.degrees(1)


        except ValueError:
            return None




class PyAvlStabilityFiles:
    """
    Manages multiple AVL stability files.

    Attributes:
        filenames (list): List of AVL stability filenames.
        stabfiles (list): List of processed stability file data.
        keys (list): List of data keys.
        ctrlKeys (list, optional): List of control keys.
    """
    def __init__(self,filenames=[], keyListControls=None):
        """
        Initializes the class and processes the given stability files.

        Args:
            filenames (list): List of filenames.
            keyListControls (list, optional): List of control keys.
        """
        self.filenames = filenames

        self.stabfiles = []
        self.keys = None
        self.ctrlKeys = keyListControls
        for filename in self.filenames:
            stabilityfile = PyAvlStabilityFile(filename, keylistControls=keyListControls)
            if self.keys is None:
                self.keys = stabilityfile.get_keys()
            self.stabfiles.append(stabilityfile.get_data())

    def get_data(self):
        """Returns the data extracted from the stability files."""
        return self.stabfiles

    def get_keys(self):
        """Returns the list of keys from the stability files."""
        return self.keys

class PyAvlStabilityFile:
    """
    Represents an AVL stability file.

    Attributes:
        filename (str): Name of the AVL stability file.
        keylist (list): List of extracted data keys.
        keylistControls (list): List of control keys.
        data (dict): Extracted data from the file.
    """

    def __init__(self, filename, keylistControls=None):
        """
        Initializes the stability file by parsing its contents.

        Args:
            filename (str): Filename of the AVL stability file.
            keylistControls (list, optional): List of control keys.
        """
        self.filename = filename

        self.keylist = self.init_dictionary()
        self.keylistControls = self.set_control_keys(keylistControls)
        self.keyControlNums = []
        self.data = self.fill_dictionary()

    def get_keys(self):
        """Returns the list of keys in the stability file."""
        return self.keylist

    def get_data(self):
        """Returns the extracted data from the stability file."""
        return self.data

    def init_dictionary(self):
        """Initializes the dictionary with predefined aerodynamic data keys."""
        keylist = []

        keylist += ["Altitude","Sref", "Cref", "Bref"]
        keylist += ["Xref", "Yref", "Zref", "Xnp"]
        keylist += ["Alpha", "Beta", "Mach", "pb/2V", "qc/2V", "rb/2V"]
        keylist += ["CLtot","CYtot", "CDtot", "CDvis", "CDind", "Cltot", "Cmtot", "Cntot"]

        namederiv = ["CL", "CY", "Cl", "Cm", "Cn"]
        addderiv = ["a", "b", "p", "q", "r"]

        for id in addderiv:
            keylist += [val + id for val in namederiv]
        return keylist

    def fill_dictionary(self):
        """
        Reads the AVL stability file and fills the data dictionary.

        Returns:
            dict: Extracted aerodynamic data mapped to their respective keys.
        """
        with open(self.filename, "r") as file:
            lines = file.readlines()

            addderiv = []
            namederiv = ["CL", "CY", "Cl", "Cm", "Cn","CDff"]
            for ctrlkey in self.keylistControls:
                id = self.get_control_device_num(lines, ctrlkey)
                self.keylist.append(ctrlkey)
                self.keylist += [val + id for val in namederiv]

            data = dict.fromkeys(self.keylist, None)

            for key in self.keylist:
                if key == "Altitude":
                    data["Altitude"] = self.get_ft_from_runcase(lines)
                else:
                    data[key] = self.get_x_value(lines, key)

        return data

    def set_control_keys(self, controlnames=None):
        """Sets the control keys."""
        controlnames = list(controlnames)
        keylistControls = []
        if controlnames is None or type(controlnames) is not type([]):
            return keylistControls
        else:
            keylistControls = controlnames
        return keylistControls

    def get_x_value(self, lines, key):
        """
        Extracts the numerical value associated with a given key from the file.

        Args:
            lines (list): Lines from the AVL stability file.
            key (str): The key whose value needs to be extracted.

        Returns:
            float or None: Extracted numerical value or None if not found.
        """
        searchObj = re.compile(rf'{key} * = *.\d*.\d*')
        for line in lines:
            res = searchObj.search(line)
            if res is not None:
                ret = res.group().split("=")
                return float(ret[1])

    def get_control_device_num(self, lines, ctrlkey):
        """
        Retrieves the control device number for a given control key.

        Args:
            lines (list): Lines from the AVL stability file.
            ctrlkey (str): The control key whose number is needed.

        Returns:
            str: Control device number as a string.
        """
        searchObj = re.compile(rf'{ctrlkey[:]} * d\d*.\d*')
        for line in lines:
            res = searchObj.search(line)
            if res is not None:
                ret = res.group().split()
                return ret[1]

    def get_ft_from_runcase(self,lines):
        """
        Extracts altitude information from the run case.

        Args:
            lines (list): Lines from the AVL stability file.

        Returns:
            float: Altitude value extracted from the run case.
        """
        searchObj = re.compile(rf'Run case: RunCase \d* - \d*.\d*')
        for line in lines:
            res = searchObj.search(line)
            if res is not None:
                ret = res.group().split("-")
                return float(ret[-1])

    def write(self, fileout):
        """
        Writes the extracted data to a CSV file.

        Args:
            fileout (str): The output filename without extension.
        """
        with open(fileout + ".csv", "w", newline='') as file:
            writer = csv.DictWriter(file, fieldnames=self.keylist)
            writer.writeheader()
            writer.writerow(self.data)
