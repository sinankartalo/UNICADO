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

#ifndef LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEINPUT_H_
#define LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEINPUT_H_

#include <aircraftGeometry2/airfoil_surface.h>
#include <aircraftGeometry2/processing/measure.h>
#include <string>
#include <vector>
#include "liftingLineInterface2/liftingLineSettings.h"
#include "liftingLineInterface2/liftingLineWing.h"

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

class liftingSurface;
class Point;

/** \class liftingLineInput Class for managing liftingLineInput data.
 */
class LIFTINGLINEINTERFACEDLLEXPORT liftingLineInput {
 public:
    friend class liftingLineInterface; /**< Definition of liftingLine as friend class */
    friend class liftingLineOutput; /**< Definition of liftingLineOutput as friend class */

    /** \brief Default destructor for liftingLineInput.
     */
    virtual ~liftingLineInput() {}

    double machNumber; /**< Mach number of liftingLine run */
    std::string LILIfilename; /**< Name of liftingLine files (without file type) */
    std::string LILIfilepath; /**< Path to liftingLine input file */

 private:
    /** \brief Default constructor for liftingLineInput.
     *  \param myLLsettings Reference to liftingLineSettings
     *  \param IoDir Input/output (project) directory
     */
    liftingLineInput(const liftingLineSettings &myLLsettings, std::string const& IoDir);

    const liftingLineSettings& myLLsettings; /**< Object for liftingLineSettings */

    /** \brief Method to merge all functions to create liftingLine input.
     *  \param liftingSurfaces Vector holding the liftingSurfaces
     *  \param theFuselage Constant reference to the geometry object of aircraft fuselage 
     *  \param aMachNumber Mach number for the liftingLine run
     *  \param aReynoldsnumber Reynolds number for the liftingLine run
     *  \param refArea Actual reference wing area, [m2]. Will be used in case no other is specified in liftingLine_conf.xml
     *  \param aCoG {x,y,z}-coordinates of the center of gravity of the aircraft, [m]
     */
    void createLiftingLineInput(const std::vector<geom2::MultisectionSurface<geom2::AirfoilSection>>& liftingSurfaces,
                                const std::vector<double>& fuselageWidths,
                                const double& aMachNumber, const double& aReynoldsnumber, const double& refArea, const Point& aCoG);
    /** \brief Method to initialize liftingLine run.
     *  \details Method differentiates between compressible and incompressible.
     *  \param aMachNumber Mach number for which liftingLine shall be executed
     */
    void initializeLiftingLine(const double& aMachNumber);
    /** \brief Method to set the general liftingLine filename (without file type)
     *  \param isIncompressible Switch to differentiate between compressible and incompressible runs
     *  \return std::string liftingLine filename
     */
    std::string setLILIfilename(bool isIncompressible);
    /** \brief Method to set path for the liftingLine files
     *  \param liliFilename liftingLine filename (without file type)
     *  \return std::string liftingLine file path
     */
    std::string setLILIfilepath(const std::string& liliFilename);
    /** \brief Method to set the panel distribution used as input.
     *  \return Panel distribution with panel numbers used as input
     */
    liftingLineWing::liftingLinePanels setInputPanelDistribution();
    /** \brief Method to check total number of panels.
     */
    void checkLILIinput();
    /** \brief Method to write the liftingLine input file.
     *  \param wingReferenceArea Reference area of the main wing, [m2]
     *  \param wingReferenceSpan Reference span of the main wing, [m]
     *  \param wingMAC Mean aerodynamic chord of the main wing, [m]
     *  \param momentReferencePoint {x,y,z}-coordinates used as reference points for moments, [m]
     *  \param numberLiftingSurfaces Number if liftingSurfaces
     *  \param anInputPolar Reference to polar object from liftingLineSettings
     *  \param aReynoldsNumber Reynolds number used for the liftingLine run
     */
    void writeLILIinputFile(const double& wingReferenceArea, const double& wingReferenceSpan, const double& wingMAC, const Point& momentReferencePoint,
                            const uint16_t& numberLiftingSurfaces, const liftingLineSettings::polarConfig& anInputPolar, const double& aReynoldsNumber,
                            const uint16_t& numberOfPropellers = 0);

    std::vector<liftingLineWing> theLILIWingGeometries; /**< Vector holding liftingLineWing data */

    std::string ioDir; /**< Input/output (project) directory */
    std::string LILIinputFilepath; /**< Path for liftingLine input file */
    std::string machNumber_str; /**< Current Mach number as std::string */
};
#endif  // LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEINPUT_H_
