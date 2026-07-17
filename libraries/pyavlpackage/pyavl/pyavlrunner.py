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
PyAvl - A Python to AVL conversion script for generating lifting surfaces based on given geometry.

This module provides utilities to run Athena Vortex Lattice (AVL) simulations and install AVL/XFOIL tools.
"""
import os
import subprocess
import platform
import urllib.request
import shutil
import stat

class PyAvlRunner:
    """
    Handles execution of AVL simulations, manages run cases, and processes stability results.
    """
    def __init__(self,avlFile=None,runcasefile=None,runcases=["default"]):
        """
        Initializes the PyAvlRunner class.

        Args:
            avlFile (str, optional): Path to AVL geometry file.
            runcasefile (str, optional): Path to AVL run case file.
            runcases (list, optional): List of run case names.
        """
        self.avlproc = None
        self.cmdline = ""
        self.avlFile = avlFile
        self.runcaseFile = runcasefile
        self.runcases = runcases
        self.count = 1
        self.path_to_session_folder = None
        self.path_to_stability_files = None
        self.path_to_runcase_files = None

    def session_setup(self):
        """
        Sets up the AVL session with geometry and run cases.
        """
        # Load geometry and case
        self.cmdline += f"load {self.avlFile}\n"
        self.cmdline += f"case {self.runcaseFile}.run\n"


        # Switch to operation mode
        self.cmdline += f"oper\n"

        # Select runcase
        for id, runcase in enumerate(self.runcases):
            self.cmdline += f"{id+1}\n"
            self.cmdline += "x\n"
            self.cmdline += f"st\n{runcase}_{id}\n"
        self.cmdline += "\nquit\n"

    def session_setup_single_rc(self, path_to_session_folder, runcase_files_folder, stability_files_folder, results_folder, runcase_ids):

        # Setup paths to session and resulting files
        self.path_to_session_folder = path_to_session_folder
        self.path_to_runcase_files = runcase_files_folder
        self.path_to_stability_files = stability_files_folder
        self.path_to_session_result_file = results_folder

        # Load geometry and case
        self.cmdline += f"load {path_to_session_folder}/{self.avlFile}\n"
        for _, id in enumerate(runcase_ids):
            self.cmdline += f"case {runcase_files_folder}/rc_{id:03}.run\n"

            # Switch to operation mode
            self.cmdline += f"oper\n"
            self.cmdline += f"I\n"
            self.cmdline += "x\n"
            self.cmdline += f"st {stability_files_folder}/res_{id:03}.stab\n"
            self.cmdline += "\n"
        self.cmdline += "quit\n"

        return self.cmdline

    def run_avl_session(self, cmdline=None):
        """
        Executes the AVL session.

        Args:
            cmdline (str, optional): Custom command sequence for AVL.
        """
        if cmdline is None:
            cmdline = self.cmdline
        print("Computing aerodynamics with Athena Vortex Lattice (AVL) ... ",end="")
        process = self.avl_process()
        process.communicate(input=cmdline.encode())
        process.wait()
        print("finished")


    def avl_process(self, path_to_avl_executable: str="."):
        """
        Initializes the AVL process.

        Args:
            path_to_avl_executable (str, optional): Path to the AVL executable.

        Returns:
            subprocess.Popen: AVL process.
        """
        avl_executable = "avl"
        if platform.system() == "Windows":
            avl_executable = "avl.exe"
        return subprocess.Popen(args=[f'{path_to_avl_executable}/{avl_executable}'],
                                stdin=subprocess.PIPE,
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL,
                                bufsize=0)

    def session_End(self):
        """End avl session"""
        self.avlproc.terminate()
        self.avlproc.kill()

    def session_folder(self):
        """Return session folder path"""
        return self.path_to_session_folder

    def stability_files_folder(self):
        """Returns stability files folder path"""
        return self.path_to_stability_files

    def runcase_files_folder(self):
        """Returns runcases files folder path"""
        return self.path_to_runcase_files

    def result_files_folder(self):
        """Returns result file folder path"""
        return self.path_to_session_result_file


class AVLinstaller:
    """
    Downloads and installs AVL based on the user's operating system.
    """
    def __init__(self):
        """Initializes AVLinstaller with appropriate download URLs."""
        self.avl_url = {"root": "http://web.mit.edu/drela/Public/web/avl/",
                        "Windows": "avl3.40_execs/WIN64/avl.exe",
                        "Darwin-x86_64": "avl3.40_execs/DARWIN64/avl",
                        "Darwin-arm64": "avl3.40_execs/DARWINM1/avl",
                        "Linux": "avl3.40_execs/LINUX64/avl"}
        self.operating_system: str=self.check_os()
        self.machine_architecture: str=self.check_architecture()
        self.avl_download_link: str=None

    def check_os(self) -> str:
        """Returns the operating system name."""
        return platform.system()

    def check_architecture(self) -> str:
        """Returns the system architecture."""
        return platform.machine()

    def set_avl_download_link(self) -> None:
        """Determines the correct AVL download link based on the OS and architecture."""
        if self.operating_system == "Darwin":
            self.operating_system += "-" + self.machine_architecture

        self.avl_download_link = self.avl_url["root"] + self.avl_url[self.operating_system]

    def is_command_installed(self, command):
        """Determines if command is installed correctly"""
        return shutil.which(command) is not None

    def download_avl(self) -> bool:
        """Downloads and installs AVL if not already installed."""
        print("Checking AVL ...")
        executable = "avl"
        if self.operating_system == "Windows":
            executable += ".exe"
        path_to_executable = "./" + executable

        if self.is_command_installed(path_to_executable) or os.path.exists(path_to_executable):
            print("AVL is already installed.")
            return True
        self.set_avl_download_link()
        print("Downloading AVL ...")
        print(self.avl_download_link)
        print(os.getcwd())
        urllib.request.urlretrieve(self.avl_download_link, os.getcwd()+"/" +executable)
        if self.is_command_installed(path_to_executable) or os.path.exists(path_to_executable):
            print("AVL is installed.")
            print("Changing rights")
            os.chmod(path_to_executable, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
            return True
        else:
            return False
