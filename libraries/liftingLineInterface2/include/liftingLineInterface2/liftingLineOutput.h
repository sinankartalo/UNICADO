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

#ifndef LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEOUTPUT_H_
#define LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEOUTPUT_H_

#include <string>
#include "liftingLineInterface2/liftingLineInput.h"
#include "liftingLineInterface2/liftingLinePolar.h"

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

/** \class liftingLineOutput Class for managing liftingLineOutput data.
 */
class LIFTINGLINEINTERFACEDLLEXPORT liftingLineOutput : public liftingLinePolars {
 public:
    /** \brief Default constructor for liftingLineOutput.
     *  \param myLLinput Reference to liftingLineInput object
     */
    explicit liftingLineOutput(const liftingLineInput &myLLinput);
    /** \brief Default destructor for liftingLineOutput.
     */
    virtual ~liftingLineOutput() {}

    /** \brief Function to read liftingLine results from LL output.xml file.
     *  \param ioFilename Name of aircraft.xml
     *  \param numberWings Number of liftingSurfaces used for liftingLine calculation
     */
    void readLiftingLineResults(int numberLiftingSurfaces);
    /** \brief Function to get analytical lift/induced drag polar coefficients based on regression analysis.
     */
    void getAnalyticalPolar();
    /** \brief Function to get analytical lift/induced drag polar coefficients of liftingSurfaces based on regression analysis.
     *  \param aLiftingSurfacePolar Object holding polar points of respective liftingSurface
     *  \param llPolarSize Total number of calculated polar points
     */
    void getAnalyticalPolarLiftingSurface(const liftingLinePolars::liftingLinePolarWings& aLiftingSurfacePolar, const uint16_t& llPolarSize);
    /** \brief Function to get analytical lift/induced drag polar coefficients of liftingSurfaces parts based on regression analysis.
     *  \details Currently, the only use case is the application of ROMs, which optionally require parts of the induced drag from LILI to be added
     *  \param aLiftingSurfacePolar Object holding polar points of respective liftingSurface
     *  \param llPolarSize Total number of calculated polar points
     */
    void getAnalyticalPolarLiftingSurfacePart(const liftingLinePolars::liftingLinePolarWings& aLiftingSurfacePolar, const uint16_t& llPolarSize);

 private:
    liftingLineInput myLLinput; /**< Object for liftingLineInput */
};
#endif  // LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEOUTPUT_H_
