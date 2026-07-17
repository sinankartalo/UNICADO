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

def write_elem_new_line(fh, val):
    """
    Writes a value to the file and adds a new line.

    Args:
        fh (file object): File handle.
        val (str): Value to write.
    """
    if len(val):
        fh.write(f'{val}\n')


def write_elem_tab(fh, elems, newline=0):
    """
    Writes elements separated by spaces and optionally adds a new line.

    Args:
        fh (file object): File handle.
        elems (list or str): Elements to write.
        newline (int, optional): Number of new lines to add.
    """
    if type(elems) is list:
        for elem in elems:
            fh.write(f'{elem} ')
    else:
        fh.write(f'{elems} ')

    write_new_line(fh, newline)


def write_key(fh, key):
    """Writes a key as a new line."""
    write_elem_new_line(fh, key)


def write_tag(fh, tag):
    """Writes a tag as a new line."""
    write_elem_new_line(fh, tag)


def write_afile(fh, key, afile):
    """
    Writes a key followed by a filename as new lines.

    Args:
        fh (file object): File handle.
        key (str): Key to write.
        afile (str): Associated filename.
    """
    write_elem_new_line(fh, key)
    write_elem_new_line(fh, f'{afile}')


def write_commentline(list, commentstring):
    """
    Adds a commented line to a list.

    Args:
        lst (list): List to append the comment.
        commentstring (str): Comment text.
    """
    commentline = "# " + commentstring + "\n"
    list.append(commentline)


def write_separation_big(file, token=""):
    """
    Writes a large separation line with an optional token.

    Args:
        file (file object): File handle.
        token (str, optional): Text to include in the separation.
    """
    sepString = "# =="
    if len(token):
        sepString += f' {token} '

    lsepString = len(sepString)
    for i in range(0, 80 - lsepString):
        sepString += "="
    sepString += "\n"
    file.write(sepString)


def write_separation_small(file, token=""):
    """
    Writes a small separation line with an optional token.

    Args:
        file (file object): File handle.
        token (str, optional): Text to include in the separation.
    """
    sepString = "# --"
    if len(token):
        sepString += f' {token} '

    lsepString = len(sepString)
    for i in range(0, 80 - lsepString):
        sepString += "-"
    sepString += "\n"
    file.write(sepString)


def write_headline(file, short=""):
    """Writes a headline in a formatted manner."""
    file.write(f'# -- {short} --\n')


def write_new_line(fh, num=1):
    """
    Writes the specified number of new lines.

    Args:
        fh (file object): File handle.
        num (int): Number of new lines to write.
    """
    crnl = ""
    for i in range(0, num):
        crnl += "\n"
    fh.write(crnl)


def write_avl_file(author="", datalist=[], filename="filename", postfix=".avl"):
    """
    Writes AVL data to a specified file.

    Args:
        author (str, optional): Author name.
        datalist (list): Data elements to write.
        filename (str): Base filename.
        postfix (str, optional): File extension.
    """
    with open(filename + postfix, "w") as avlFile:
        for item in datalist:
            item.write(avlFile)


def write_runcase(file, runcasenum=1, runcasename="default", alpha=0.0, beta=0.0, pb2v=0.0, qc2v=0.0, rb2v=0.0,
                 controls=None, mach=0.0, CD0=0.02, density=1.225,a=340.294, cgref=[0.0, 0.0, 0.0]):
    """
    Writes a run case block to the file.

    Args:
        file (file object): File handle.
        runcasenum (int, optional): Run case number.
        runcasename (str, optional): Run case name.
        alpha (float, optional): Angle of attack.
        beta (float, optional): Sideslip angle.
        pb2v, qc2v, rb2v (float, optional): Non-dimensional rotational rates.
        controls (list, optional): List of control surfaces.
        mach (float, optional): Mach number.
        CD0 (float, optional): Zero-lift drag coefficient.
        density (float, optional): Air density.
        a (float, optional): Speed of sound.
        cgref (list, optional): Center of gravity reference coordinates.
    """
    file.write(f' ---------------------------------------------\n')
    file.write(f' Run case  {runcasenum}:  {runcasename}\n')
    file.write(f' alpha        ->  alpha       =   {alpha}\n')
    file.write(f' beta         ->  beta        =   {beta}\n')
    file.write(f' pb/2V        ->  pb/2V       =   {pb2v}\n')
    file.write(f' qc/2V        ->  qc/2V       =   {qc2v}\n')
    file.write(f' rb/2V        ->  rb/2V       =   {rb2v}\n')
    if controls is not None and isinstance(controls,list):
        for control in controls:
            file.write(f' {control}\t\t->{control}\t\t=\t{0.0}\n')
    # if aileron is not None:
        # file.write(f' aileron      ->  aileron     =   {aileron}\n')
    # if elevator is not None:
        # file.write(f' elevator     ->  elevator    =   {elevator}\n')
    # if rudder is not None:
        # file.write(f' rudder       ->  rudder      =   {rudder}\n')

    file.write("\n")

    default = 0.0
    velocity = mach*a[0]
    file.write(f' alpha     =  {alpha}     deg\n')
    file.write(f' beta      =  {beta}     deg\n')
    file.write(f' pb/2V     =  {default}\n')
    file.write(f' qc/2V     =  {default}\n')
    file.write(f' rb/2V     =  {default}\n')
    file.write(f' CL        =  {default}\n')
    file.write(f' CDo       =  {CD0}\n')
    file.write(f' bank      =  {default}     deg\n')
    file.write(f' elevation =  {default}     deg\n')
    file.write(f' heading   =  {default}     deg\n')
    file.write(f' Mach      =  {mach}\n')
    file.write(f' velocity  =   {velocity:.4f}     Lunit/Tunit\n')
    file.write(f' density   =   {density[0]:.4f}     Munit/Lunit^3\n')
    file.write(f' grav.acc. =   {9.80665}     Lunit/Tunit^2\n')
    file.write(f' turn_rad. =   {default}     Lunit\n')
    file.write(f' load_fac. =   {1.00000}\n')
    file.write(f' X_cg      =   {cgref[0]}    Lunit\n')
    file.write(f' Y_cg      =   {cgref[1]}    Lunit\n')
    file.write(f' Z_cg      =   {cgref[2]}    Lunit\n')
    file.write(f' mass      =   {1.00000}    Munit\n')
    file.write(f' Ixx       =   {1.00000}    Munit-Lunit^2\n')
    file.write(f' Iyy       =   {1.00000}    Munit-Lunit^2\n')
    file.write(f' Izz       =   {1.00000}    Munit-Lunit^2\n')
    file.write(f' Ixy       =   {0.00000}    Munit-Lunit^2\n')
    file.write(f' Iyz       =   {0.00000}    Munit-Lunit^2\n')
    file.write(f' Izx       =   {0.00000}    Munit-Lunit^2\n')
    file.write(f' visc CL_a =   {0.00000}\n')
    file.write(f' visc CL_u =   {0.00000}\n')
    file.write(f' visc CM_a =   {0.00000}\n')
    file.write(f' visc CM_u =   {0.00000}\n\n')
