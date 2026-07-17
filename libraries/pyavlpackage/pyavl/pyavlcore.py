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
PyAvl Core classes
"""
from .pyavlfilewrite import *
from .pyavlfileread import *
from ambiance import Atmosphere

"""
Geometry classes (.avl)
"""


class Header:
    def __init__(self, runcase: str="base", mach: float=0.0, sym: list=[0, 0, 0.0], base: list[float]=[1.0, 1.0, 1.0], ref: list[float]=[0.0, 0.0, 0.0]):
        """ Initialize avl header - please see avl primer

        Args:
            runcase (str, optional): Runcase name. Defaults to "base".
            mach (float, optional): mach number. Defaults to 0.0.
            sym (list, optional): symmetry elements according to AVL. Defaults to [0, 0, 0.0].
            base (list[float], optional): Basic aerodynamic parameters (Sref, cref, bref). Defaults to [1.0, 1.0, 1.0].
            ref (list[float], optional): Reference cg point. Defaults to [0.0, 0.0, 0.0].
        """
        self.runcase = runcase
        self.mach = mach
        self.sym = sym
        self.base = base
        self.ref = ref
        self.author = ""

    def write(self, fh) -> None:
        """ Write data to file via fh -> filehandler

        Args:
            fh (): filehandler/filedescriptor
        """
        write_separation_big(fh, "BASE FILE")
        write_headline(fh, "Geometric Name")
        write_elem_tab(fh, self.runcase, 2)
        write_headline(fh, "Mach (normal to wing)")
        write_elem_tab(fh, self.mach, 2)
        write_headline(fh, "Symmetric properties (iYsym, iZsym, Zsym)")
        write_elem_tab(fh, self.sym, 2)
        write_headline(fh, "Reference properties Sref, Cref, Bref")
        write_elem_tab(fh, self.base, 2)
        write_headline(fh, "Reference CG position (X,Y,Z)")
        write_elem_tab(fh, self.ref, 2)
        write_separation_big(fh, "Begin of Geometry")

class Control:
    def __init__(self, tag="ctrl", gain=1.0, xhinge=0.7, xyzhvec=[0.0, 1.0, 0.0], sgndup=1.0):
        self.key = "CONTROL"
        self.tag = tag
        self.gain = gain
        self.xhinge = xhinge
        self.xyzhvec = xyzhvec
        self.sgndup = sgndup
        self.description = ["!name", "gain", "xhinge", "XYZhvec", "SgnDup"]

    def write(self, fh) -> None:
        """ Write data to file via fh -> filehandler

        Args:
            fh (): filehandler/filedescriptor
        """
        write_key(fh, self.key)
        write_elem_tab(fh, self.description, 1)
        write_elem_tab(fh, self.tag)
        write_elem_tab(fh, self.gain)
        write_elem_tab(fh, self.xhinge)
        write_elem_tab(fh, self.xyzhvec)
        write_elem_tab(fh, self.sgndup, 2)


class Section:
    def __init__(self, refLE: list[float]=[0.0, 0.0, 0.0], chord: float=1.0, dihedral: float=0.0, nspan: int=1, sspace: float=1.0, afileKey: str="NACA",
                 afile: str="0012"):
        """ Section class

        Args:
            refLE (list[float], optional): Position of section reference point (leading edge). Defaults to [0.0, 0.0, 0.0].
            chord (float, optional): chord length. Defaults to 1.0.
            dihedral (float, optional): dihedral. Defaults to 0.0.
            nspan (int, optional): number of spanwise horseshoe vortices. Defaults to 1.
            sspace (float, optional): spanwise vortex spacing. Defaults to 1.0.
            afileKey (str, optional): airfoil file key (NACA or AFILE). Defaults to "NACA".
            afile (str, optional): for key NACA -> enter 4 digit naca profile, otherwise if AFILE -> enter path to airfoil. Defaults to "0012".
        """
        self.key = "SECTION"
        self.ref = refLE
        self.chord = chord
        self.dihedral = dihedral
        self.nspan = nspan
        self.sspace = sspace
        self.afileKey = afileKey
        self.afile = afile
        self.controls = []
        self.description = ["!Xle", "Yle", "Zle", "Chord", "dihedral", "nspan", "sspace"]

    def add_control_device(self, control: Control) -> None:
        """

        Args:
            control (Control): Add control object
        """
        self.controls.append(control)

    def write(self, fh) -> None:
        """ Write data to file via fh -> filehandler

        Args:
            fh (): filehandler/filedescriptor
        """
        write_separation_small(fh, "Section")
        write_key(fh, self.key)
        write_elem_tab(fh, self.description, 1)
        write_elem_tab(fh, self.ref)
        write_elem_tab(fh, self.chord)
        write_elem_tab(fh, self.dihedral)
        write_elem_tab(fh, self.nspan)
        write_elem_tab(fh, self.sspace, 2)
        write_afile(fh, self.afileKey, self.afile)
        write_new_line(fh)
        for control in self.controls:
            control.write(fh)


class Yduplicate:
    def __init__(self, ydupl: float=0.0):
        """ Yduplicate

        Args:
            ydupl (float, optional): Y postion of X-Z plane -> only use if iYsym is 0. Defaults to 0.0.
        """
        self.key = "YDUPLICATE"
        self.ydupl = ydupl
        self.description = ["!YDuplicate - Geometric Symmetry, non aerodynamic symmetry"]

    def write(self, fh) -> None:
        """ Write yduplicate

        Args:
            fh (): filehandler/filedescriptor
        """
        write_key(fh, self.key)
        write_elem_tab(fh, self.description, 1)
        write_elem_tab(fh, self.ydupl, 2)


class Surface:
    def __init__(self, tag: str="surf", nchord: int=10, cspace: float=1.0, nspan: int=20, sspace: float=1.0):
        """ General surface

        Args:
            tag (str, optional): Surface tag. Defaults to "surf".
            nchord (int, optional): Number of chordwise horseshoe vortices. Defaults to 10.
            cspace (float, optional): Chordwise vortex spacing parameter. Defaults to 1.0.
            nspan (int, optional): Number of spanwise horseshoe vortices. Defaults to 20.
            sspace (float, optional): Spanwise vortex spacing. Defaults to 1.0.
        """
        self.key = "SURFACE"
        self.tag = tag
        self.nchord = nchord
        self.cspace = cspace
        self.nspan = nspan
        self.sspace = sspace
        self.yduplicates = []
        self.angles = []
        self.scales = []
        self.translates = []
        self.sections = []

    def add_yduplicate(self, ydupl) -> None:
        """ Adds yduplicate object

        Args:
            ydupl (object): yduplicate object
        """
        self.yduplicates.append(ydupl)

    def add_angle(self, angle) -> None:
        """ Adds angle object

        Args:
            angle (object): angle object
        """
        self.angles.append(angle)

    def add_scale(self, scale) -> None:
        """ Adds scale object

        Args:
            scale (object): scale object
        """
        self.scales.append(scale)

    def add_translate(self, translate) -> None:
        """ Add translate object

        Args:
            translate (object): translate object
        """
        self.translates.append(translate)

    def add_section(self, section) -> None:
        """ Add section object

        Args:
            section (object): section object
        """
        self.sections.append(section)

    def write(self, fh) -> None:
        """ Write data to file via fh -> filehandler

        Args:
            fh (): filehandler/filedescriptor
        """
        write_separation_big(fh, f'Begin of Surface [{self.tag}]')
        write_key(fh, self.key)
        write_tag(fh, self.tag)
        write_elem_tab(fh, self.nchord)
        write_elem_tab(fh, self.cspace)
        write_elem_tab(fh, self.nspan)
        write_elem_tab(fh, self.sspace, 2)

        for ydl in self.yduplicates:
            ydl.write(fh)
        for angle in self.angles:
            angle.write(fh)
        for scale in self.scales:
            scale.write(fh)
        for translate in self.translates:
            translate.write(fh)
        for section in self.sections:
            section.write(fh)

        write_separation_big(fh, f'End of Surface [{self.tag}]')


class Scale:
    def __init__(self, xscale: float=1.0, yscale: float=1.0, zscale: float=1.0):
        """ Scale object

        Args:
            xscale (float, optional): x-scale factor. Defaults to 1.0.
            yscale (float, optional): y-scale factor. Defaults to 1.0.
            zscale (float, optional): z-scale factor. Defaults to 1.0.
        """
        self.key = "SCALE"
        self.xscale = xscale
        self.yscale = yscale
        self.zscale = zscale
        self.description = ["!Xscale", "Yscale", "Zscale"]

    def write(self, fh) -> None:
        """ Write to file

        Args:
            fh (): filehandler/filedescriptor
        """
        write_key(fh, self.key)
        write_elem_tab(fh, self.description, 1)
        write_elem_tab(fh, self.xscale)
        write_elem_tab(fh, self.yscale)
        write_elem_tab(fh, self.zscale, 2)


class Translate:
    def __init__(self, dX: float=0.0, dY: float=0.0, dZ: float=0.0):
        """ Translate object (right hand coordinate system starting in nose through back (x),
            left (y), up (z))

        Args:
            dX (float, optional): x translation. Defaults to 0.0.
            dY (float, optional): y translation. Defaults to 0.0.
            dZ (float, optional): z translation. Defaults to 0.0.
        """
        self.key = "TRANSLATE"
        self.dX = dX
        self.dY = dY
        self.dZ = dZ
        self.description = ["!dX", "dY", "dZ"]

    def write(self, fh) -> None:
        """ Write to file

        Args:
            fh (): filehandler/filedescriptor
        """
        write_key(fh, self.key)
        write_elem_tab(fh, self.description, 1)
        write_elem_tab(fh, self.dX)
        write_elem_tab(fh, self.dY)
        write_elem_tab(fh, self.dZ, 2)


class Angle:
    def __init__(self, dAinc: float=0.0) -> None:
        """ Angle object

        Args:
            dAinc (float, optional): Offset added on to the Ainc values (deg). Defaults to 0.0.
        """
        self.key = "ANGLE"
        self.dAinc = dAinc
        self.description = ["!Dihedral"]

    def write(self, fh) -> None:
        """ Write to file

        Args:
            fh (): filehandler/filedescriptor
        """
        write_key(fh, self.key)
        write_elem_tab(fh, self.description, 1)
        write_elem_tab(fh, self.dAinc, 1)


"""
Runcase class (.run)
"""


class Runcase:
    def __init__(self, alpha: list[float]=[0.0], beta: float=0.0, mach: float=0.0, cgID: int=1, cgpos: list[float]=[0.0, 0.0, 0.0], height: float=11000.0,
                 aileron: float=0.0,
                 elevator: float=0.0, rudder: float=0.0):
        """ Initialize a single runcase for avl -> inputs for .run file

        Args:
            alpha (list[float], optional): angle of attack. Defaults to [0.0].
            mach (float, optional): mach number. Defaults to 0.0.
            beta (float, optional): angle. Defaults to 0.0.
            cgID (int, optional): center of gravity id (depends on user). Defaults to 1.
            cgpos (list[float], optional): cgpos w.r.t. cgID. Defaults to [0.0, 0.0, 0.0].
            height (list[float], optional): height/altitude. Defaults to [11000.0].
            aileron (float, optional): aileron (roll) control surface deflection. Defaults to 0.0.
            elevator (float, optional): elevator (pitch) control surface deflection. Defaults to 0.0.
            rudder (float, optional): rudder (yaw) control surface deflection. Defaults to 0.0.
        """
        atmo = Atmosphere(height)
        
        if alpha is None:
            alpha = [0.0]
        self.alpha = alpha
        self.beta = beta
        self.mach = mach
        self.aileron = aileron
        self.elevator = elevator
        self.rudder = rudder
        self.cgID = cgID
        self.cgpos = cgpos
        self.density = float(atmo.density[0])
        self.a = float(atmo.speed_of_sound[0])
        self.numOfRuncases = len(alpha)
        self.cases = []
        self.caseName = None

    def create_runcase_fileName(self) -> str:
        """ Create runcase filename

        Returns:
            str -- runcase filename
        """
        strmach = f'mach{self.mach}_'
        strcg = f'cgID{self.cgID}'
        s = "Runcase_" + strmach + strcg
        s = s.split('.')
        s = '_'.join(s)
        return s

    def create_runcase_file(self,id: int) -> None:
        """ Creates a .run file based on a Runcase

        Args:
            id (int): Runcase file id
        """
        self.caseName = self.create_runcase_fileName()
        idx = 1
        with open(self.caseName + ".run", "w") as file:
            for a in self.alpha:
                self.cases.append(f'file_id{id+idx}')
                write_runcase(file, idx, self.cases[-1], alpha=a, beta=self.beta, mach=self.mach, density=self.density,
                                 a=self.a, aileron=self.aileron, elevator=self.elevator,
                                 rudder=self.rudder, cgref=self.cgpos)
                idx += 1

    def get_runcases(self) -> list:
        """ Returns list of runcases

        Returns:
            list: list of runcases
        """
        return self.cases

    def get_runcase_filename(self) -> str:
        """ Get runcase filename

        Returns:
            str: runcase filename
        """
        return self.caseName
