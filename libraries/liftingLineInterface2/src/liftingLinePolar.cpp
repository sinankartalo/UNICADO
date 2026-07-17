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

#include "liftingLineInterface2/liftingLinePolar.h"

liftingLinePolars::liftingLinePolars()
    :
    dCLtodAoA(0.),
    CLatAoA0(0.),
    dCMtodAoA(0.),
    CMatAoA0(0.),
    dCMtodCL(0.),
    CMatCL0(0.),
    CDmin(0.),
    CLatCDmin(0.),
    kFactor(0.),
    dCLtodAoA_wf(0.),
    liftingSurfacePart_dCLtodAoA(0.),
    liftingSurfacePart_CLatAoA0(0.),
    liftingSurfacePart_dCMtodAoA(0.),
    liftingSurfacePart_CMatAoA0(0.),
    liftingSurfacePart_dCMtodCL(0.),
    liftingSurfacePart_CMatCL0(0.),
    liftingSurfacePart_CDmin(0.),
    liftingSurfacePart_CLatCDmin(0.),
    liftingSurfacePart_kFactor(0.),
    liftingSurfacePart_dCLtodAoA_wf(0.) {
    //ctor
}

liftingLinePolars::liftingLinePolar::liftingLinePolar()
    :
    AoA(0.),
    CL(0.),
    CDind(0.),
    CM(0.) {
    //ctor
}

liftingLinePolars::liftingLineSegment::liftingLineSegment()
    :
    yPos(0.),
    llPolarSeg(nullptr) {
    //ctor
}

liftingLinePolars::liftingLineSegment::~liftingLineSegment() {
    //dtor
}

liftingLinePolars::liftingLinePolarWings::liftingLinePolarWings()
    :
    numberOfSegments(0) {
    //ctor
}
