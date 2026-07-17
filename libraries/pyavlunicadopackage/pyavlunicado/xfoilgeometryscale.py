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
------------------------------------------------------------------------------
This file is a direct translation and copy of portions of the XFOIL code,
originally written by Mark Drela. The implementation below is based on
the XFOIL routines and is provided as-is under the original XFOIL license.

XFOIL License:
--------------
Copyright (C) 1996, 1998, 2002, 2005 by Mark Drela.  All rights reserved.

XFOIL is provided "as is" without warranty of any kind, either expressed
or implied, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose.

For more information on the XFOIL license, please refer to the license file
included with the original XFOIL distribution or visit:
http://web.mit.edu/drela/Public/web/xfoil/

------------------------------------------------------------------------------
"""

"""
xfoil_geometry.py

A collection of routines to compute spline derivatives,
evaluate splines, find leading edge parameters, opposite points,
arc lengths, curvatures, geometric properties, and to update airfoil
camber and thickness. 
"""

import numpy as np


def tri_solve(A: np.ndarray, B: np.ndarray, C: np.ndarray, D: np.ndarray, kk: int) -> None:
    """
    Solve a tri-diagonal system in place.
    
    The system is assumed to be:
        A[i]*D[i] + B[i+1]*D[i+1] + C[i]*D[i-1] = original_D[i]
    where D is replaced with the solution.
    
    Parameters
    ----------
    A : ndarray
        Main diagonal (length kk).
    B : ndarray
        Upper diagonal (length kk); note B[0] is unused.
    C : ndarray
        Lower diagonal (length kk); note C[-1] is unused.
    D : ndarray
        Right-hand side (length kk); will be overwritten with solution.
    kk : int
        Number of equations.
    """
    for k in range(1, kk):
        km = k - 1
        C[km] /= A[km]
        D[km] /= A[km]
        A[k] -= B[k] * C[km]
        D[k] -= B[k] * D[km]
    D[kk - 1] /= A[kk - 1]
    for k in range(kk - 2, -1, -1):
        D[k] -= C[k] * D[k + 1]
    # The solution is now in D.


def splind(X: np.ndarray, S: np.ndarray, xs1: float, xs2: float) -> np.ndarray:
    """
    Compute spline derivative array for a function defined by X(S).
    
    End conditions:
      - If xs1 or xs2 equal 999.0, use the usual zero second derivative condition.
      - If xs1 or xs2 equal -999.0, use the zero third derivative condition.
      - Otherwise, use the provided derivative.
    
    Parameters
    ----------
    X : ndarray
        Dependent variable array.
    S : ndarray
        Independent variable (parameter) array.
    xs1 : float
        Left endpoint derivative condition.
    xs2 : float
        Right endpoint derivative condition.
    
    Returns
    -------
    ndarray
        Spline derivative array.
    """
    N = len(X)
    if N > 1000:
        raise ValueError("SPLIND: array overflow, increase NMAX")
    A = np.zeros(N)
    B = np.zeros(N)
    C = np.zeros(N)
    D = np.zeros(N)

    # Interior points (i = 1 to N-2)
    for i in range(1, N - 1):
        dsm = S[i] - S[i - 1]
        dsp = S[i + 1] - S[i]
        B[i] = dsp
        A[i] = 2.0 * (dsm + dsp)
        C[i] = dsm
        D[i] = 3.0 * (((X[i + 1] - X[i]) * dsm / dsp) +
                      ((X[i] - X[i - 1]) * dsp / dsm))
    # Left endpoint condition
    if xs1 == 999.0:
        A[0] = 2.0
        C[0] = 1.0
        D[0] = 3.0 * (X[1] - X[0]) / (S[1] - S[0])
    elif xs1 == -999.0:
        A[0] = 1.0
        C[0] = 1.0
        D[0] = 2.0 * (X[1] - X[0]) / (S[1] - S[0])
    else:
        A[0] = 1.0
        C[0] = 0.0
        D[0] = xs1

    # Right endpoint condition
    if xs2 == 999.0:
        B[-1] = 1.0
        A[-1] = 2.0
        D[-1] = 3.0 * (X[-1] - X[-2]) / (S[-1] - S[-2])
    elif xs2 == -999.0:
        B[-1] = 1.0
        A[-1] = 1.0
        D[-1] = 2.0 * (X[-1] - X[-2]) / (S[-1] - S[-2])
    else:
        A[-1] = 1.0
        B[-1] = 0.0
        D[-1] = xs2
    if N == 2 and xs1 == -999.0 and xs2 == -999.0:
        B[-1] = 1.0
        A[-1] = 2.0
        D[-1] = 3.0 * (X[1] - X[0]) / (S[1] - S[0])
    tri_solve(A, B, C, D, N)
    return D


def segspl(X: np.ndarray, S: np.ndarray) -> np.ndarray:
    """
    Compute segmented spline derivatives for X(S), allowing discontinuities at duplicate S values.
    
    Parameters
    ----------
    X : ndarray
        Data array.
    S : ndarray
        Parameter array.
    
    Returns
    -------
    ndarray
        Derivative array computed piecewise using splind.
    
    Raises
    ------
    ValueError
        If the first or last S values are duplicated.
    """
    N = len(S)
    if np.isclose(S[0], S[1]):
        raise ValueError("SEGSPL: First input point duplicated")
    if np.isclose(S[-1], S[-2]):
        raise ValueError("SEGSPL: Last input point duplicated")
    XS = np.empty_like(X)
    seg_start = 0
    for i in range(1, N - 1):
        if np.isclose(S[i], S[i + 1]):
            seg_slice = slice(seg_start, i + 1)
            XS[seg_slice] = splind(X[seg_slice].copy(), S[seg_slice].copy(), xs1=-999.0, xs2=-999.0)
            seg_start = i + 1
    seg_slice = slice(seg_start, N)
    XS[seg_slice] = splind(X[seg_slice].copy(), S[seg_slice].copy(), xs1=-999.0, xs2=-999.0)
    return XS


def seval(ss: float, X: np.ndarray, XS: np.ndarray, S: np.ndarray) -> float:
    """
    Evaluate the spline defined by X(S) with derivatives XS at parameter value ss.
    
    Parameters
    ----------
    ss : float
        Parameter value at which to evaluate.
    X : ndarray
        Data array.
    XS : ndarray
        Spline derivative array.
    S : ndarray
        Parameter array.
    
    Returns
    -------
    float
        Interpolated value.
    """
    i = np.searchsorted(S, ss, side='right')
    if i == 0:
        i = 1
    if i >= len(S):
        i = len(S) - 1
    ds = S[i] - S[i - 1]
    t = (ss - S[i - 1]) / ds
    cx1 = ds * XS[i - 1] - X[i] + X[i - 1]
    cx2 = ds * XS[i] - X[i] + X[i - 1]
    return t * X[i] + (1.0 - t) * X[i - 1] + (t - t * t) * ((1.0 - t) * cx1 - t * cx2)


def deval(ss: float, X: np.ndarray, XS: np.ndarray, S: np.ndarray) -> float:
    """
    Evaluate the first derivative of the spline at parameter value ss.
    
    Parameters
    ----------
    ss : float
        Parameter value.
    X : ndarray
        Data array.
    XS : ndarray
        Spline derivative array.
    S : ndarray
        Parameter array.
    
    Returns
    -------
    float
        First derivative dX/dS evaluated at ss.
    """
    i = np.searchsorted(S, ss, side='right')
    if i == 0:
        i = 1
    if i >= len(S):
        i = len(S) - 1
    ds = S[i] - S[i - 1]
    t = (ss - S[i - 1]) / ds
    cx1 = ds * XS[i - 1] - X[i] + X[i - 1]
    cx2 = ds * XS[i] - X[i] + X[i - 1]
    return (X[i] - X[i - 1] +
            (1.0 - 4.0 * t + 3.0 * t * t) * cx1 +
            t * (3.0 * t - 2.0) * cx2) / ds


def d2val(ss: float, X: np.ndarray, XS: np.ndarray, S: np.ndarray) -> float:
    """
    Estimate the second derivative at ss using central finite differences.
    
    Parameters
    ----------
    ss : float
        Parameter value.
    X : ndarray
        Data array.
    XS : ndarray
        Spline derivative array.
    S : ndarray
        Parameter array.
    
    Returns
    -------
    float
        Approximated second derivative.
    """
    eps = 1e-6 * (S[-1] - S[0])
    return (deval(ss + eps, X, XS, S) - deval(ss - eps, X, XS, S)) / (2 * eps)


def lefind(X: np.ndarray, XP: np.ndarray, Y: np.ndarray, YP: np.ndarray, S: np.ndarray) -> float:
    """
    Locate the leading edge (LE) parameter S_LE. The LE is defined so that the
    surface tangent is normal to the chord connecting the LE and the trailing edge.
    
    Parameters
    ----------
    X, Y : ndarray
        Coordinate arrays.
    XP, YP : ndarray
        Spline derivative arrays for X and Y.
    S : ndarray
        Parameter (arc length) array.
    
    Returns
    -------
    float
        The parameter value at the leading edge.
    """
    N = len(S)
    dseps = (S[-1] - S[0]) * 1.0e-5
    x_te = 0.5 * (X[0] + X[-1])
    y_te = 0.5 * (Y[0] + Y[-1])
    s_le = None
    for i in range(2, N - 2):
        dx_te = X[i] - x_te
        dy_te = Y[i] - y_te
        dx = X[i + 1] - X[i]
        dy = Y[i + 1] - Y[i]
        dotp = dx_te * dx + dy_te * dy
        if dotp < 0.0:
            s_le = S[i]
            break
    if s_le is None:
        s_le = S[N // 2]
    i_guess = np.searchsorted(S, s_le)
    if i_guess > 0 and np.isclose(S[i_guess], S[i_guess - 1]):
        return S[i_guess]
    for _ in range(50):
        x_le = seval(s_le, X, XP, S)
        y_le = seval(s_le, Y, YP, S)
        dxds = deval(s_le, X, XP, S)
        dyds = deval(s_le, Y, YP, S)
        dxdd = d2val(s_le, X, XP, S)
        dydd = d2val(s_le, Y, YP, S)
        x_chord = x_le - x_te
        y_chord = y_le - y_te
        res = x_chord * dxds + y_chord * dyds
        ress = dxds**2 + dyds**2 + x_chord * dxdd + y_chord * dydd
        ds_le = -res / ress
        ds_le = max(ds_le, -0.02 * abs(x_chord + y_chord))
        ds_le = min(ds_le, 0.02 * abs(x_chord + y_chord))
        s_le += ds_le
        if abs(ds_le) < dseps:
            return s_le
    print("LEFIND: LE point not found. Continuing...")
    return s_le


def sopps(
    si: float,
    X: np.ndarray,
    XP: np.ndarray,
    Y: np.ndarray,
    YP: np.ndarray,
    S: np.ndarray,
    s_le: float
) -> float:
    """
    Find the opposite point parameter for a given point at parameter si.
    The opposite point is determined by matching the chordwise coordinate.
    
    Parameters
    ----------
    si : float
        Parameter value of the given point.
    X, Y : ndarray
        Coordinate arrays.
    XP, YP : ndarray
        Spline derivative arrays.
    S : ndarray
        Parameter (arc-length) array.
    s_le : float
        Leading edge parameter.
    
    Returns
    -------
    float
        Parameter of the opposite point.
    """
    N = len(S)
    s_len = S[-1] - S[0]
    x_le = seval(s_le, X, XP, S)
    y_le = seval(s_le, Y, YP, S)
    x_te = 0.5 * (X[0] + X[-1])
    y_te = 0.5 * (Y[0] + Y[-1])
    chord = np.sqrt((x_te - x_le) ** 2 + (y_te - y_le) ** 2)
    dxc = (x_te - x_le) / chord
    dyc = (y_te - y_le) / chord
    if si < s_le:
        i_idx = 0
        i_opp = N - 1
    else:
        i_idx = N - 1
        i_opp = 0
    denom = S[i_idx] - s_le
    s_frac = (si - s_le) / denom if denom != 0 else 0.0
    s_opp = s_le + s_frac * (S[i_opp] - s_le)
    if abs(s_frac) <= 1.0e-5:
        return s_le
    xi = seval(si, X, XP, S)
    yi = seval(si, Y, YP, S)
    x_le = seval(s_le, X, XP, S)
    y_le = seval(s_le, Y, YP, S)
    xbar = (xi - x_le) * dxc + (yi - y_le) * dyc
    for _ in range(12):
        x_opp = seval(s_opp, X, XP, S)
        y_opp = seval(s_opp, Y, YP, S)
        xopp_d = deval(s_opp, X, XP, S)
        yopp_d = deval(s_opp, Y, YP, S)
        res = (x_opp - x_le) * dxc + (y_opp - y_le) * dyc - xbar
        resd = xopp_d * dxc + yopp_d * dyc
        if abs(res) / s_len < 1.0e-5:
            break
        if resd == 0.0:
            print("SOPPS: Opposite-point location failed. Continuing...")
            s_opp = s_le + s_frac * (S[i_opp] - s_le)
            break
        ds_opp = -res / resd
        s_opp += ds_opp
        if abs(ds_opp) / s_len < 1.0e-5:
            break
    return s_opp


def scalc(X: np.ndarray, Y: np.ndarray) -> np.ndarray:
    """
    Compute the cumulative arc-length array for the 2-D curve defined by (X, Y).
    
    Parameters
    ----------
    X, Y : ndarray
        Coordinate arrays.
    
    Returns
    -------
    ndarray
        Cumulative arc-length.
    """
    N = len(X)
    S = np.empty(N)
    S[0] = 0.0
    for i in range(1, N):
        S[i] = S[i - 1] + np.hypot(X[i] - X[i - 1], Y[i] - Y[i - 1])
    return S


def curv(ss: float, X: np.ndarray, XS: np.ndarray, Y: np.ndarray, YS: np.ndarray, S: np.ndarray) -> float:
    """
    Compute the curvature of a splined 2-D curve at parameter ss.
    
    Parameters
    ----------
    ss : float
        Parameter value.
    X, Y : ndarray
        Coordinate arrays.
    XS, YS : ndarray
        Spline derivative arrays.
    S : ndarray
        Parameter (arc-length) array.
    
    Returns
    -------
    float
        Curvature.
    """
    N = len(S)
    i = np.searchsorted(S, ss, side='right')
    if i == 0:
        i = 1
    if i >= N:
        i = N - 1
    ds = S[i] - S[i - 1]
    t = (ss - S[i - 1]) / ds
    f1 = ds * XS[i - 1]
    f2 = -ds * (2.0 * XS[i - 1] + XS[i]) + 3.0 * (X[i] - X[i - 1])
    f3 = ds * (XS[i - 1] + XS[i]) - 2.0 * (X[i] - X[i - 1])
    xd = f1 + t * (2.0 * f2 + t * 3.0 * f3)
    xdd = 2.0 * f2 + t * 6.0 * f3

    g1 = ds * YS[i - 1]
    g2 = -ds * (2.0 * YS[i - 1] + YS[i]) + 3.0 * (Y[i] - Y[i - 1])
    g3 = ds * (YS[i - 1] + YS[i]) - 2.0 * (Y[i] - Y[i - 1])
    yd = g1 + t * (2.0 * g2 + t * 3.0 * g3)
    ydd = 2.0 * g2 + t * 6.0 * g3

    return (xd * ydd - yd * xdd) / np.sqrt((xd**2 + yd**2) ** 3)


def ae_calc(N: int, X: np.ndarray, Y: np.ndarray, T: np.ndarray, itype: int):
    """
    Calculate geometric properties of the shape defined by X, Y.
    
    Parameters
    ----------
    N : int
        Number of points.
    X, Y : ndarray
        Coordinate arrays.
    T : ndarray
        Skin-thickness array (used only if itype == 2).
    itype : int
        1 to integrate over whole area (dx dy); 2 to integrate over skin area (T ds).
    
    Returns
    -------
    tuple
        (area, xcen, ycen, ei11, ei22, apx1, apx2)
    """
    PI = np.pi
    sint = 0.0
    aint = 0.0
    xint = 0.0
    yint = 0.0
    xxint = 0.0
    xyint = 0.0
    yyint = 0.0
    for i in range(N):
        ip = i + 1 if i < N - 1 else 0
        dx = X[i] - X[ip]
        dy = Y[i] - Y[ip]
        xa = 0.5 * (X[i] + X[ip])
        ya = 0.5 * (Y[i] + Y[ip])
        ta = 0.5 * (T[i] + T[ip])
        ds = np.hypot(dx, dy)
        sint += ds
        if itype == 1:
            da = ya * dx
            aint += da
            xint += xa * da
            yint += ya * da / 2.0
            xxint += xa**2 * da
            xyint += xa * ya * da / 2.0
            yyint += ya**2 * da / 3.0
        else:
            da = ta * ds
            aint += da
            xint += xa * da
            yint += ya * da
            xxint += xa**2 * da
            xyint += xa * ya * da
            yyint += ya**2 * da
    if aint == 0.0:
        return (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)
    xcen = xint / aint
    ycen = yint / aint
    eiixx = yyint - ycen**2 * aint
    eixy = xyint - xcen * ycen * aint
    eiiyy = xxint - xcen**2 * aint
    eisq = 0.25 * (eiixx - eiiyy)**2 + eixy**2
    sgn = 1.0 if (eiiyy - eiixx) >= 0.0 else -1.0
    ei11 = 0.5 * (eiixx + eiiyy) - sgn * np.sqrt(eisq)
    ei22 = 0.5 * (eiixx + eiiyy) + sgn * np.sqrt(eisq)
    if ei11 == 0.0 or ei22 == 0.0:
        apx1 = 0.0
        apx2 = np.arctan2(1.0, 0.0)
    elif eisq / (ei11 * ei22) < (0.001 * sint)**4:
        apx1 = 0.0
        apx2 = np.arctan2(1.0, 0.0)
    else:
        c1 = eixy
        s1 = eiixx - ei11
        c2 = eixy
        s2 = eiixx - ei22
        if abs(s1) > abs(s2):
            apx1 = np.arctan2(s1, c1)
            apx2 = apx1 + 0.5 * PI
        else:
            apx2 = np.arctan2(s2, c2)
            apx1 = apx2 - 0.5 * PI
        if apx1 < -0.5 * PI:
            apx1 += PI
        if apx1 > 0.5 * PI:
            apx1 -= PI
        if apx2 < -0.5 * PI:
            apx2 += PI
        if apx2 > 0.5 * PI:
            apx2 -= PI
    return aint, xcen, ycen, ei11, ei22, apx1, apx2


def tc_calc(X: np.ndarray, XP: np.ndarray, Y: np.ndarray, YP: np.ndarray, S: np.ndarray):
    """
    Compute the maximum thickness and camber from discrete airfoil points.
    
    Parameters
    ----------
    X, Y : ndarray
        Coordinate arrays.
    XP, YP : ndarray
        Spline derivative arrays.
    S : ndarray
        Parameter (arc-length) array.
    
    Returns
    -------
    tuple
        (thickness, x_at_thickness, camber, x_at_camber)
    """
    s_le = lefind(X, XP, Y, YP, S)
    x_le = seval(s_le, X, XP, S)
    y_le = seval(s_le, Y, YP, S)
    x_te = 0.5 * (X[0] + X[-1])
    y_te = 0.5 * (Y[0] + Y[-1])
    chord = np.hypot(x_te - x_le, y_te - y_le)
    dxc = (x_te - x_le) / chord
    dyc = (y_te - y_le) / chord
    thickness = 0.0
    x_thick = 0.0
    camber = 0.0
    x_camber = 0.0
    N = len(S)
    for i in range(N):
        x_bar = (X[i] - x_le) * dxc + (Y[i] - y_le) * dyc
        y_bar = (Y[i] - y_le) * dxc - (X[i] - x_le) * dyc
        s_opp = sopps(S[i], X, XP, Y, YP, S, s_le)
        x_opp = seval(s_opp, X, XP, S)
        y_opp = seval(s_opp, Y, YP, S)
        y_bar_opp = (y_opp - y_le) * dxc - (x_opp - x_le) * dyc
        yc = 0.5 * (y_bar + y_bar_opp)
        yt = abs(y_bar - y_bar_opp)
        if abs(yc) > abs(camber):
            camber = yc
            x_camber = x_opp
        if abs(yt) > abs(thickness):
            thickness = yt
            x_thick = x_opp
    return thickness, x_thick, camber, x_camber


def geopar(
    X: np.ndarray, XP: np.ndarray, Y: np.ndarray, YP: np.ndarray,
    S: np.ndarray, T: np.ndarray, verbose: bool=False
) -> dict:
    """
    Compute overall geometric parameters of the airfoil shape.
    
    Parameters
    ----------
    X, Y : ndarray
        Coordinate arrays.
    XP, YP : ndarray
        Spline derivative arrays.
    S : ndarray
        Parameter (arc-length) array.
    T : ndarray
        Skin-thickness array (usually ones).
    
    Returns
    -------
    dict
        Dictionary of geometric parameters.
    """
    N = len(S)
    s_le = lefind(X, XP, Y, YP, S)
    x_le = seval(s_le, X, XP, S)
    y_le = seval(s_le, Y, YP, S)
    x_te = 0.5 * (X[0] + X[-1])
    y_te = 0.5 * (Y[0] + Y[-1])
    chord = np.hypot(x_te - x_le, y_te - y_le)
    curv_le = curv(s_le, X, XP, Y, YP, S)
    rad_le = 1.0 / curv_le if abs(curv_le) > 0.001 * (S[-1] - S[0]) else 0.0
    ang1 = np.arctan2(-YP[0], -XP[0])
    ang2 = np.arctan2(YP[-1], XP[-1])
    ang_te = ang2 - ang1
    area, xcena, ycena, ei11a, ei22a, apx1a, apx2a = ae_calc(N, X, Y, T, itype=1)
    slen, xcent, ycent, ei11t, ei22t, apx1t, apx2t = ae_calc(N, X, Y, T, itype=2)
    thick, x_thick, cambr, x_camber = tc_calc(X, XP, Y, YP, S)
    if verbose:
        print(f"Max thickness = {thick: .6f}  at x = {x_thick: .3f}")
        print(f"Max camber    = {cambr: .6f}  at x = {x_camber: .3f}")
    return {
        "SLE": s_le, "CHORD": chord, "AREA": area, "RADLE": rad_le, "ANGTE": ang_te,
        "EI11A": ei11a, "EI22A": ei22a, "APX1A": apx1a, "APX2A": apx2a,
        "EI11T": ei11t, "EI22T": ei22t, "APX1T": apx1t, "APX2T": apx2t,
        "THICK": thick, "CAMBR": cambr
    }


def sortol(x_arr: np.ndarray, y_arr: np.ndarray, tol: float) -> (np.ndarray, np.ndarray):
    """
    Sort (x, y) pairs by x.
    
    Parameters
    ----------
    x_arr : ndarray
        x-coordinate array.
    y_arr : ndarray
        y-coordinate array.
    tol : float
        Tolerance (unused here, kept for compatibility).
    
    Returns
    -------
    tuple
        Sorted (x_arr, y_arr) arrays.
    """
    order = np.argsort(x_arr)
    return x_arr[order], y_arr[order]


def getcam(
    X: np.ndarray, XP: np.ndarray, Y: np.ndarray, YP: np.ndarray, S: np.ndarray
) -> (np.ndarray, np.ndarray, int, np.ndarray, np.ndarray, int):
    """
    Compute the camber line and thickness distribution for an airfoil.
    
    Parameters
    ----------
    X, Y : ndarray
        Coordinate arrays.
    XP, YP : ndarray
        Spline derivative arrays.
    S : ndarray
        Parameter (arc-length) array.
    
    Returns
    -------
    tuple
        (x_camber, y_camber, n_camber, x_thick, y_thick, n_thick)
    """
    N = len(S)
    s_le = lefind(X, XP, Y, YP, S)
    x_le = seval(s_le, X, XP, S)
    y_le = seval(s_le, Y, YP, S)
    x_camber = np.empty(N)
    y_camber = np.empty(N)
    x_thick = np.empty(N)
    y_thick = np.empty(N)
    for i in range(N):
        if i == 0:
            x_opp = X[-1]
            y_opp = Y[-1]
        elif i == N - 1:
            x_opp = X[0]
            y_opp = Y[0]
        else:
            s_opp = sopps(S[i], X, XP, Y, YP, S, s_le)
            x_opp = seval(s_opp, X, XP, S)
            y_opp = seval(s_opp, Y, YP, S)
        x_camber[i] = 0.5 * (X[i] + x_opp)
        y_camber[i] = 0.5 * (Y[i] + y_opp)
        x_thick[i] = 0.5 * (X[i] + x_opp)
        y_thick[i] = abs(0.5 * (Y[i] - y_opp))
    # Append the leading-edge point
    x_camber = np.append(x_camber, x_le)
    y_camber = np.append(y_camber, y_le)
    n_camber = len(x_camber)
    x_thick = np.append(x_thick, x_le)
    y_thick = np.append(y_thick, 0.0)
    n_thick = len(x_thick)
    tol_val = 1.0e-5 * (S[-1] - S[0])
    x_camber, y_camber = sortol(x_camber, y_camber, tol_val)
    y_camber -= y_camber[0]
    x_thick, y_thick = sortol(x_thick, y_thick, tol_val)
    return x_camber, y_camber, n_camber, x_thick, y_thick, n_thick


def thkcam(
    tfac: float,
    cfac: float,
    XB: np.ndarray,
    XBP: np.ndarray,
    YB: np.ndarray,
    YBP: np.ndarray,
    SB: np.ndarray
) -> (np.ndarray, np.ndarray, dict):
    """
    Adjust the airfoil (buffer) thickness and camber.
    
    Parameters
    ----------
    tfac : float
        Thickness scaling factor.
    cfac : float
        Camber scaling factor.
    XB, YB : ndarray
        Buffer airfoil coordinate arrays.
    XBP, YBP : ndarray
        Buffer airfoil spline derivative arrays.
    SB : ndarray
        Parameter (arc-length) array for the buffer.
    
    Returns
    -------
    tuple
        Updated XB, YB arrays and a dictionary of geometric parameters.
    """
    NB = len(SB)
    s_le = lefind(XB, XBP, YB, YBP, SB)
    x_le = seval(s_le, XB, XBP, SB)
    y_le = seval(s_le, YB, YBP, SB)
    x_te = 0.5 * (XB[0] + XB[-1])
    y_te = 0.5 * (YB[0] + YB[-1])
    chord = np.hypot(x_te - x_le, y_te - y_le)
    dxc = (x_te - x_le) / chord
    dyc = (y_te - y_le) / chord
    W1 = np.empty(NB)
    W2 = np.empty(NB)
    for i in range(NB):
        if i == 0:
            xb_opp = XB[-1]
            yb_opp = YB[-1]
        elif i == NB - 1:
            xb_opp = XB[0]
            yb_opp = YB[0]
        else:
            s_opp = sopps(SB[i], XB, XBP, YB, YBP, SB, s_le)
            xb_opp = seval(s_opp, XB, XBP, SB)
            yb_opp = seval(s_opp, YB, YBP, SB)
        xc_avg = 0.5 * (XB[i] + xb_opp) * dxc + 0.5 * (YB[i] + yb_opp) * dyc
        yc_avg = cfac * (0.5 * (YB[i] + yb_opp) * dxc - 0.5 * (XB[i] + xb_opp) * dyc)
        xc_del = 0.5 * (XB[i] - xb_opp) * dxc + 0.5 * (YB[i] - yb_opp) * dyc
        yc_del = tfac * (0.5 * (YB[i] - yb_opp) * dxc - 0.5 * (XB[i] - xb_opp) * dyc)
        W1[i] = (xc_avg + xc_del) * dxc - (yc_avg + yc_del) * dyc
        W2[i] = (yc_avg + yc_del) * dxc + (xc_avg + xc_del) * dyc

    # Update buffer coordinates with new values
    XB[:] = W1
    YB[:] = W2

    # Recalculate arc-length and update spline derivatives
    SB_new = scalc(XB, YB)
    XBP = segspl(XB, SB_new)
    YBP = segspl(YB, SB_new)
    # For GEOPAR, define a thickness array (using ones)
    T = np.ones_like(XB)
    geo_params = geopar(XB, XBP, YB, YBP, SB_new, T)
    return XB, YB, geo_params

def load_profile(file_path: str=""):
    """
    Reads an airfoil profile file and returns the x and y coordinates.
    Assumes the file contains two whitespace-separated columns.
    """
    try:
        data = np.loadtxt(file_path)
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return None, None
    if data.ndim == 1 or data.shape[1] < 2:
        print(f"File {file_path} does not have two columns.")
        return None, None

    return data # x, y coordinates


def rescale_profile(coordinates: np.ndarray=None, scale_t_to_c: float=1.0, scale_camber: float=1.0) -> np.ndarray:
    """ Rescale profile according to thickness to chord or to camber

    Keyword Arguments:
        coordinates {np.ndarray} -- coordinates x, y (from TE,UP -> LE -> TE,DN) (default: {None})
        scale_t_to_c {float} -- scaling factor to scale current thickness to chord (default: {1.0})
        scale_camber {float} -- scaling factor to scale current camber (default: {1.0})

    Returns:
        np.ndarray -- scaled coordinates
    """
    x = coordinates[:, 0]
    y = coordinates[:, 1]
    
    # Spline points
    s = np.linspace(0, 1, coordinates.shape[0])
    xp = segspl(x, s)
    yp = segspl(y, s)
    
    xcpy = x.copy()
    ycpy = y.copy()
    scpy = s.copy()
    xpcpy = xp.copy()
    ypcpy = yp.copy()
    x_new, y_new, _ = thkcam(scale_t_to_c, scale_camber, xcpy, xpcpy, ycpy, ypcpy, scpy)
    return np.column_stack((x_new, y_new))

def save_profile(coordinates: np.ndarray=None, file: str="./scaled_profile.dat") -> None:
    """ Save profile coordinates in file

    Keyword Arguments:
        coordinates {np.ndarray} -- profile coordinates (default: {None})
        file {str} -- file + (default: {"./scaled_profile.dat"})
    """
    np.savetxt(file, coordinates, fmt="%.8f", delimiter=" ")
    return 