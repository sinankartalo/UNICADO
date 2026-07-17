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

#ifndef LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEINTERFACE_H_
#define LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEINTERFACE_H_

#include <moduleBasics/runtimeIO.h>
#include <aircraftGeometry2/airfoil_surface.h>
#include <string>
#include <vector>
#include <memory>
#include "liftingLineInterface2/liftingLineSettings.h"

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

class liftingLineInput;
class liftingLineOutput;
class liftingSurface;
class Point;

/** \class liftingLine Class for managing liftingLine data.
 */
class LIFTINGLINEINTERFACEDLLEXPORT liftingLineInterface {
 public:
    /** \brief Default constructor for liftingLine.
     *  \param path2Conf Path to liftingLine_conf.xml in liftingLine folder
     *  \param IoDir Input/output (project) directory
     */
    liftingLineInterface(const std::string& path2Conf, const std::string& IoDir);
    /** \brief Default destructor for liftingLine.
     */
    virtual ~liftingLineInterface() {}
    /** \brief Function to start liftingLine
    */
    void executeLiftingLine();
    /** \brief Function for initializing liftingLine
     *  \param liftingSurfaces Vector holding the liftingSurfaces
     *  \param theFuselage Constant reference to the geometry object of aircraft fuselage 
     *  \param aMachNumber Mach number for the liftingLine run
     *  \param aReynoldsnumber Reynolds number for the liftingLine run
     *  \param refArea Actual reference wing area, [m2]. Will be used in case no other is specified in liftingLine_conf.xml
     *  \param aCoG {x,y,z}-coordinates of the center of gravity of the aircraft, [m]
     */
    void initializeLiftingLine(const std::vector<geom2::MultisectionSurface<geom2::AirfoilSection>>& liftingSurfaces,
                                const std::vector<double>& fuselageWidths,
                                const double& aMachNumber, const double& aReynoldsnumber, const double& refArea, const Point& aCoG);

    liftingLineInput* theLiftingLineInputPt; /**< Pointer to liftingLineInput */
    liftingLineOutput* theLLOutputPt; /**< Pointer to liftingLineOutput */
    liftingLineSettings myLLsettings; /**< Object for liftingLineSettings */

    /** \brief Function to create backup directory
     *  \return Bool whether backup directory was created
     */
    bool createBackupDir(const std::string& aSourceDir, const std::string& aBackupDir);

    std::string LILI_src_dir; /**< Path to LiftingLine source directory */
    std::string LILI_bkp_dir; /**< Path to LiftingLine backup directory */
    bool LILI_bkp_dir_created; /**< Bool whether backup directory was created */

 private:
    // Member functions
    /** \brief Function initializes Pointer
    */
    void initializePointer();

    /** \brief Function to set the execution command to run liftingLine
     *  \return Execution command as std::string
     */
    std::string setExecutionCommand();

    // Member variables
    std::string ioDir; /**< Input/output (project) directory */
    std::string ioFilename; /**< Name of input/output file (aircraft.xml) */
};
#endif  // LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEINTERFACE_H_
