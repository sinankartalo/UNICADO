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

#ifndef LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEPOLAR_H_
#define LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEPOLAR_H_

#include <vector>

#ifdef BUILD_LIFTINGLINEINTERFACE_SHARED
    #ifdef _WIN32
        #define LIFTINGLINEINTERFACEDLLEXPORT __declspec(dllexport)
    #else
        #define LIFTINGLINEINTERFACEDLLEXPORT __attribute__ ((visibility ("default")))
    #endif
#elif defined(IMPORT_LIFTINGLINEINTERFACE_SHARED)
    #ifdef _WIN32
        #define LIFTINGLINEINTERFACEDLLEXPORT __declspec(dllimport)
    #else
        #define LIFTINGLINEINTERFACEDLLEXPORT
    #endif
#else
    #define LIFTINGLINEINTERFACEDLLEXPORT
#endif

/** \class liftingLinePolars Class for managing all polar related data.
 */
class LIFTINGLINEINTERFACEDLLEXPORT liftingLinePolars {
 public:
    /** \class liftingLinePolar Class for managing liftingLine polar data.
     */
    class LIFTINGLINEINTERFACEDLLEXPORT liftingLinePolar {
     public:
        /** \brief Constructor for liftingLinePolar.
         */
        liftingLinePolar();
        /** \brief Destructor for liftingLinePolar.
         */
        virtual ~liftingLinePolar() {}

        double AoA; /**< Angle of attack, [deg] */
        double CL; /**< Lift coefficient */
        double CDind; /**< Induced drag coefficient */
        double CM; /**< Moment coefficient */
    };

    /** \class liftingLineSegment Class for managing lifting line segments/panels.
     */
    class LIFTINGLINEINTERFACEDLLEXPORT liftingLineSegment {
     public:
        /** \brief Constructor for liftingLineSegment.
        */
        liftingLineSegment();
        /** \brief Destructor for liftingLineSegment.
         */
        virtual ~liftingLineSegment();
        double yPos; /**< y-position of the liftingLine panel middle used for vortex placement (calculated internally in liftingLine), [m] */
        liftingLinePolar *llPolarSeg; /**< Local polar point of the liftingLine panel */
    };

    /** \class liftingLinePolarWithSegments Class for polar points of panels including Cl-distribution.
     */
    class LIFTINGLINEINTERFACEDLLEXPORT liftingLinePolarWithSegments : public liftingLinePolar {
     public:
        /** \brief Constructor for liftingLinePolarWithSegments.
         */
        liftingLinePolarWithSegments() {}
        /** \brief Destructor for liftingLinePolarWithSegments.
         */
        virtual ~liftingLinePolarWithSegments() {}

        std::vector<liftingLineSegment> wingSegments; /**< Number of lifting line panels */
    };

    /** \class liftingLinePolarWings Class for managing wing polar data.
     */
    class LIFTINGLINEINTERFACEDLLEXPORT liftingLinePolarWings {
     public:
        /** \brief Constructor for liftingLinePolarWings.
         */
        liftingLinePolarWings();
        /** \brief Destructor for liftingLinePolarWings.
         */
        virtual ~liftingLinePolarWings() {}

        int numberOfSegments; /**< Number of liftingLine segments */
        std::vector <double> yPositions; /**< Vector holding different y-positions of the panels for liftingLineWings, [m] */
        std::vector<liftingLinePolarWithSegments> llPolarW; /**< Vector holding liftingLinePolarWithSegments objects */
    };

    /** \brief Default constructor for liftingLinePolars.
     */
    liftingLinePolars();
    /** \brief Default destructor for liftingLinePolars.
     */
    virtual ~liftingLinePolars() {}

    std::vector<liftingLinePolar> llPolar; /**< Vector holding liftingLinePolar objects */
    std::vector<liftingLinePolarWings> llPolarWings; /**< Vector holding liftingLinePolarWings objects. Usually 2 elements (wing and HLW) */

    /* Specific global parameters for aerodynamic coefficients of analytical polar. */
    double dCLtodAoA; /**< Slope of AoA-CL polar */
    double CLatAoA0; /**< CL at AoA = 0 */
    double dCMtodAoA; /**< Slope of AoA-CM polar */
    double CMatAoA0; /**< CM at AoA = 0 */
    double dCMtodCL; /**< Slope of CL-CM polar */
    double CMatCL0; /**< CM at CL = 0 */
    double CDmin; /**< Minimum drag coefficient (for dCDind/dCL = 0) */
    double CLatCDmin; /**< Lift coefficient at CDmin */
    double kFactor; /**< Third polynomial coefficient from regression fit */
    double dCLtodAoA_wf; /** Slope of AoA-CL polar with fuselage correction  */
    /* Specific liftingSurface parameters for aerodynamic coefficients of analytical polar. */
    std::vector<double> liftingSurface_dCLtodAoA; /**< Vector holding slope of AoA-CL polar of liftingSurfaces */
    std::vector<double> liftingSurface_CLatAoA0; /**< Vector holding CL at AoA = 0 of liftingSurfaces */
    std::vector<double> liftingSurface_dCMtodAoA; /**< Vector holding slope of AoA-CM polar of liftingSurfaces */
    std::vector<double> liftingSurface_CMatAoA0; /**< Vector holding CM at AoA = 0 of liftingSurfaces */
    std::vector<double> liftingSurface_dCMtodCL; /**< Vector holding slope of CL-CM polar of liftingSurfaces */
    std::vector<double> liftingSurface_CMatCL0; /**< Vector holding CM at CL = 0 of liftingSurfaces */
    std::vector<double> liftingSurface_CDmin; /**< Vector holding minimum drag coefficients of liftingSurfaces (for dCDind/dCL = 0) */
    std::vector<double> liftingSurface_CLatCDmin; /**< Vector holding lift coefficients at CDmin of liftingSurfaces */
    std::vector<double> liftingSurface_kFactor; /**< Vector holding third polynomial coefficients from regression fit of liftingSurfaces */
    /* Specific lifting surface part parameters for aerodynamic coefficients of analytical polar. */
    double liftingSurfacePart_dCLtodAoA; /**< Slope of AoA-CL polar */
    double liftingSurfacePart_CLatAoA0; /**< CL at AoA = 0 */
    double liftingSurfacePart_dCMtodAoA; /**< Slope of AoA-CM polar */
    double liftingSurfacePart_CMatAoA0; /**< CM at AoA = 0 */
    double liftingSurfacePart_dCMtodCL; /**< Slope of CL-CM polar */
    double liftingSurfacePart_CMatCL0; /**< CM at CL = 0 */
    double liftingSurfacePart_CDmin; /**< Minimum drag coefficient (for dCDind/dCL = 0) */
    double liftingSurfacePart_CLatCDmin; /**< Lift coefficient at CDmin */
    double liftingSurfacePart_kFactor; /**< Third polynomial coefficient from regression fit */
    double liftingSurfacePart_dCLtodAoA_wf; /** Slope of AoA-CL polar with fuselage correction  */
};
#endif  // LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEPOLAR_H_
