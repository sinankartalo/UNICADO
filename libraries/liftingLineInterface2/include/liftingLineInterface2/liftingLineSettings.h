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

#ifndef LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINESETTINGS_H_
#define LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINESETTINGS_H_

#include <string>
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

/** \class liftingLineSettings Class for managing liftingLineSettings data.
 */
class LIFTINGLINEINTERFACEDLLEXPORT liftingLineSettings {
 public:
    friend class liftingLineInput; /**< Definition of liftingLineInput as friend class */
    friend class liftingLineInterface; /**< Definition of liftingLine as friend class */
    friend class liftingLineOutput; /**< Definition of liftingLineOutput as friend class */

    /** \brief Default constructor for liftingLineSettings.
     *  \param path2Conf Path to liftingLine_conf.xml
     */
    explicit liftingLineSettings(const std::string& path2Conf);
    /** \brief Default destructor for liftingLineSettings.
     */
    virtual ~liftingLineSettings() {}

    std::string LL_version; /**< Number of LiftingLine version */
    unsigned int error_handling; /**< Value decides whether program should exit (0), use existing polars (1) or not abort (2) in case of numerical liftingLine error */
    double referenceArea; /**< Value for reference wing area, [m2]. If 0, then actual area calculated in aircraftGeometry lib is used. */

 private:
    /** \class polarConfig Class for managing polarConfig data from liftingLine_conf.xml.
     */
    class LIFTINGLINEINTERFACEDLLEXPORT polarConfig {
     public:
        /** \brief Constructor for polarConfig.
         */
        polarConfig();
        /** \brief Destructor for polarConfig.
         */
        virtual ~polarConfig() {}

        double beta; /**< Sideslip angle, [deg] */
        int numberAoA; /**< Number of angle of attack to be calculated with liftingLine */
        std::vector<double> AoA; /**< Vector holding angles of attack, [deg] */
        int numberCL; /**< Number of lift coefficients to be calculated with liftingLine */
        std::vector<double> liftCoefficient; /**< Vector holding lift coefficients */
    } Polar; /**< Object for polarConfig */

    /** \brief Function to read liftingLine settings from liftingLine_conf.xml
     *  \param path2Conf Path to liftingLine_conf.xml
     */
    void getLiftingLineSettings(const std::string& path2Conf);

    /** \brief Function check availability of LL_Exe_path and sets it to LiftingLine/LIFTING_LINE_<WINDOWS/LINUX>_64BIT.EXE as DEFAULT is given in the config
    */
    void checkLL_Exe_path();

    std::string LL_Exe_path; /**< Path to liftingLine executable (including *.exe) */
    std::string programShortName; /**< Short name of program LiftingLine is used in */
    bool consoleOut; /**< Switch for console output */
    bool tecplotOut; /**< Switch for tecplot output */
    unsigned int numbChordwisePanels; /**< Number of panels in chordwise direction */
    unsigned int numbAddSpanwisePanels; /**< Number of panels in spanwise direction (in addition to wing segments) */
    double reductionFactorHTP; /**< Reduction factor for panels in spanwise direction on HTP */
    unsigned int minSpanPanelsPerSegment; /**< Minimum number of panels in spanwise direction per segment */
    unsigned int minValueTip; /**< Minimum number of panels in spanwise direction for tip segment */
    bool twistDistrType; /**< Type of twist distribution (linear twist or linearly straked, fully three dimensional geometry twist) */
    bool untwistFslgSeg; /**< Switch to untwist fuselage segment to achieve planar calculation for this segment, [dimensionless] */
    double referenceSpan; /**< Value for reference wing span, [m]. If 0, the actual span calculated in aircraftGeometry lib is used */
    double spanForScale; /**< Factor for scaling the span */
    double maxRuntime; /**< Maximum runtime, [min] */
};
#endif  // LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINESETTINGS_H_
