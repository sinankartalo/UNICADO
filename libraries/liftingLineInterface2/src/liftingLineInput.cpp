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

#include "liftingLineInterface2/liftingLineInput.h"
#include <runtimeInfo/runtimeInfo.h>
#include <moduleBasics/runtimeIO.h>
#include <standardFiles/functions.h>
#include <unitConversion/constants.h>
#include <aircraftGeometry2/airfoil_surface.h>
#include <aircraftGeometry2/processing/measure.h>

liftingLineInput::liftingLineInput(const liftingLineSettings &myLLsettings, std::string const& IoDir)
    :
    machNumber(NAN),
    myLLsettings(myLLsettings),
    ioDir(IoDir) {
    // ctor
}

void liftingLineInput::createLiftingLineInput(const std::vector<geom2::MultisectionSurface<geom2::AirfoilSection>>& liftingSurfaces,
                                                const std::vector<double>& fuselageWidths,
                                                const double& aMachNumber, const double& aReynoldsnumber, const double& refArea, const Point& aCoG) {
    this->machNumber = aMachNumber;
    // LILIfilename and machNumber_str
    this->initializeLiftingLine(aMachNumber);
    // LILIgeometry
    for (unsigned int liftingSurfaceID(0); liftingSurfaceID < liftingSurfaces.size(); liftingSurfaceID++) {
        this->theLILIWingGeometries.push_back(liftingLineWing());
        // Initialize struct liftingLinePanels to be used in liftingLineWingGeometry::setPanelDistribution
        this->theLILIWingGeometries.back().theInputPanelDistribution = this->setInputPanelDistribution();
        this->theLILIWingGeometries.back().generateLILIgeometry(liftingSurfaces.at(liftingSurfaceID), fuselageWidths.at(liftingSurfaceID), myLLsettings.reductionFactorHTP, myLLsettings.untwistFslgSeg);
    }
    this->checkLILIinput();
    // Write LILI input file .inp
    double halfSpan = fabs(liftingSurfaces.at(0).sections.front().origin.z() - liftingSurfaces.at(0).sections.back().origin.z());
    this->writeLILIinputFile(refArea, geom2::measure::span(liftingSurfaces.at(0)), geom2::measure::mean_aerodynamic_chord(liftingSurfaces.at(0)), aCoG,
                            liftingSurfaces.size(), myLLsettings.Polar, aReynoldsnumber);
}

void liftingLineInput::initializeLiftingLine(const double& aMachNumber) {
    if (fabs(aMachNumber - 0.) < ACCURACY_LOW) {  // Incompressible
        this->machNumber_str = "0.00";
        this->LILIfilename = this->setLILIfilename(true);
    } else {  // Compressible
        this->machNumber_str = num2Str(aMachNumber);
        this->LILIfilename = this->setLILIfilename(false);
    }
    this->LILIfilepath = this->setLILIfilepath(this->LILIfilename);
    this->LILIinputFilepath = this->LILIfilepath + ".inp";
}

std::string liftingLineInput::setLILIfilename(bool isIncompressible) {
    std::string liliFilename(myLLsettings.programShortName);
    if (isIncompressible) {
        liliFilename += "_inc";
    } else {
        liliFilename += "_M" + replaceAll(this->machNumber_str, ".", "");
    }
    liliFilename += "_LL";
    return liliFilename;
}

std::string liftingLineInput::setLILIfilepath(const std::string& liliFilename) {
    return (this->ioDir + liliFilename);
}

liftingLineWing::liftingLinePanels liftingLineInput::setInputPanelDistribution() {
    liftingLineWing::liftingLinePanels theInputPanelDistribution;
    theInputPanelDistribution.numberOfChordwisePanels = myLLsettings.numbChordwisePanels;
    theInputPanelDistribution.numberOfMinimumSpanwisePanelsPerSegment = myLLsettings.minSpanPanelsPerSegment;
    theInputPanelDistribution.numberOfAdditionalSpanwisePanels = myLLsettings.numbAddSpanwisePanels;
    theInputPanelDistribution.numberOfMinimumSpanwisePanelsForTipSegment = myLLsettings.minValueTip;
    return theInputPanelDistribution;
}

void liftingLineInput::checkLILIinput() {
    unsigned int totalNumberPanel(0);
    for (unsigned int i = 0; i < this->theLILIWingGeometries.size(); i++) {
        totalNumberPanel = totalNumberPanel + this->theLILIWingGeometries.at(i).N_PW_Chord * this->theLILIWingGeometries.at(i).N_PW_Span;
    }
    myRuntimeInfo->out << "-> Total number of panels: " << totalNumberPanel << std::endl;
    if (totalNumberPanel > 3000) {
        myRuntimeInfo->err << "Maximum of 3000 panel allowed in total. Current total panel number = " << totalNumberPanel << ". Abort program!" << std::endl;
        exit(1);
    }
}

void liftingLineInput::writeLILIinputFile(const double& wingReferenceArea, const double& wingReferenceSpan, const double& wingMAC, const Point& momentReferencePoint,
                                          const uint16_t& numberLiftingSurfaces, const liftingLineSettings::polarConfig& anInputPolar, const double& aReynoldsNumber,
                                          const uint16_t& numberOfPropellers) {
    myRuntimeInfo->out << "Write LILI Input file " << this->LILIfilename << ".inp ...";
    std::ofstream LILIinput;
    // Open and write
    LILIinput.open(this->LILIinputFilepath.c_str());
    if (!LILIinput) {
        myRuntimeInfo->err << "LIFTING_LINE input file (" << this->LILIfilename << ".inp) could not be opened!" << std::endl;
        exit(1);
    } else {
        int it2(1), N_PROPELLERS(numberOfPropellers);
        myRuntimeInfo->out << "(" << this->LILIfilename << ".inp)" << std::endl;
        /** Header of the liftingLine input file **/
        LILIinput << "|======================================================================================================================================|" << std::endl;
        LILIinput << " LIFTING_LINE INPUTFILE " << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << " VERSION V" << myLLsettings.LL_version << " " << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|**************************************************************************************************************************************|" << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|                                                             NAME_OF_DATASET                                                          |" << std::endl;
        LILIinput << " LIFTING_LINE V" << myLLsettings.LL_version << ", " << ioDir << " " << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|   SYMMETRY   | GEO_SCALING  |  TWIST_DISTR | GEO_TWIST | QSI_STDY_ROT | N_PROPELLERS | N_ADD_LINES  |   XML_DATA   |                             |"
                  << std::endl;
        // SYMMETRY
        LILIinput << "              1";
        // GEO_SCALING
        LILIinput << "       " << std::fixed << myLLsettings.spanForScale;
        // TWIST_DISTR
        LILIinput << "              " << int(myLLsettings.twistDistrType);
        // GEO_TWIST (liftingLine Version 3.0)
        LILIinput << "              1";
        // QSI_STDY_ROT
        LILIinput << "              0";
        // N_PROPELLERS
        LILIinput << "              " << N_PROPELLERS;
        // N_ADD_LINES
        LILIinput << "              0";
        // XML_DATA
        LILIinput << "              1" << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|   REF_AREA   |   REF_SPAN   | REF_LEN_CMX  | REF_LEN_CMY  | REF_LEN_CMZ  |  MOM_REF_X   |  MOM_REF_Y   |  MOM_REF_Z   |              |" << std::endl;
        // REF_AREA
        if (myLLsettings.referenceArea != 0) {
            LILIinput << "       " << std::fixed << myLLsettings.referenceArea;
        } else {
            LILIinput << "       " << std::fixed << wingReferenceArea;
        }
        // REF_SPAN
        if (myLLsettings.referenceSpan != 0) {
            LILIinput << "       " << std::fixed << myLLsettings.referenceSpan;
        } else {
            LILIinput << "       " << std::fixed << wingReferenceSpan;
        }
        // REF_LEN_CMX
        LILIinput << "        0.00";
        // REF_LEN_CMY
        LILIinput << "     " << std::fixed << wingMAC;
        // REF_LEN_CMZ
        LILIinput << "              0";
        // MOM_REF_X
        LILIinput << "          " << std::fixed << momentReferencePoint.xCoordinate;
        // MOM_REF_Y
        LILIinput << "              " << std::fixed << momentReferencePoint.yCoordinate;
        // MOM_REF_Z
        LILIinput << "          " << std::fixed << momentReferencePoint.zCoordinate << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|   N_WINGS   |" << std::endl;
        LILIinput << "            " << numberLiftingSurfaces << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|   N_PW_SPAN  |  N_PW_CHORD  |                                                                                                        |" << std::endl;
        for (unsigned int currentLiftingSurface(0); currentLiftingSurface < numberLiftingSurfaces; currentLiftingSurface++) {
            LILIinput << "             " << this->theLILIWingGeometries.at(currentLiftingSurface).numberOfWingSegments << "              " << this->theLILIWingGeometries.at(
                          currentLiftingSurface).N_PW_Chord << std::endl;
        }
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|**************************************************************************************************************************************|" << std::endl;
        /** Block 3: Variation Data **/
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|   N_ALPHA    |    ALPHA     |    ALPHA     |    ALPHA     |    ALPHA     |    ALPHA     |    ALPHA     |    ALPHA     |      ...      " << std::endl;
        // N_ALPHA
        LILIinput << "              " << anInputPolar.numberAoA;
        // ALPHAs
        for (int n(0); n <= anInputPolar.numberAoA - 1; n++) {
            LILIinput << "      " << anInputPolar.AoA.at(n);
        }
        LILIinput << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "| N_TARGET_CFZ |  TARGET_CFZ  |  TARGET_CFZ  |  TARGET_CFZ  |  TARGET_CFZ  |  TARGET_CFZ  |  TARGET_CFZ  |  TARGET_CFZ  |      ...      " << std::endl;
        // N_TARGET_CFZ
        LILIinput << "              " << anInputPolar.numberCL;
        // TARGET_CFZs
        for (int n(0); n <= anInputPolar.numberCL - 1; n++) {
            LILIinput << "            " << anInputPolar.liftCoefficient.at(n);
        }
        LILIinput << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|  COMP_MA_NO  |     BETA     |                                                                                                        |" << std::endl;
        // COMP_MA_NO
        LILIinput << "           " << this->machNumber_str;
        // BETA
        LILIinput << "       " << std::fixed << anInputPolar.beta << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|**************************************************************************************************************************************|" << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        /* Block 4: Propeller data */
        if (N_PROPELLERS > 1) {
            LILIinput << "|    X_PROP    |    Y_PROP    |    Z_PROP    |    D_PROP    |     TILT     |      TOE     |   ROTATION   |    N_PHI     |    N_RAD     |" << std::endl;
            LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
            LILIinput << "|  PROP_NUMBER |     PHI      |      r/R     |   dVAX/V0    |   dVTN/V0    |                                                           |" << std::endl;
            for (int i(1); i <= N_PROPELLERS; i++) {
                for (int k(1); k <= 1; k++) { /* Adapt condition for k!!! */
                    // PROP_NUMBER
                    LILIinput << i;
                    // LILIinput << PHI;
                    // LILIinput << r/R;
                    // LILIinput << dVAX/V0;
                    // LILIinput << dVTN/V0 << std::endl;
                }
            }
            LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
            LILIinput << "| SLIP_DEF_VER | SLIP_DEF_HOR |    SDP_3     |    SDP_4     |    SDP_5     |    SDP_6     |    SDP_7     |    SDP_8     |              |" << std::endl;
            // LILIinput << SLIP_DEF_VER << std::endl;
            // LILIinput << SLIP_DEF_HOR << std::endl;
            // SDP_3 - SDP8 describe Slipstream development parameters (not activated by the LiLi developers yet)
        }
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        /**Block 6: XML-Data**/
        LILIinput << "|**************************************************************************************************************************************|" << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|  XML_MA_NO   |   XML_RE_NO  |                                                                                                        |" << std::endl;
        // XML_MA_NO
        LILIinput << "           " << this->machNumber_str;
        // XML_RE_NO
        LILIinput << "    " << aReynoldsNumber << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        // This for-loop could optionally be improved so that the correct airfoils are entered into the input file.
        for (unsigned int j(1); j <= numberLiftingSurfaces; j++) {
            LILIinput << "|  N_AIRFOILS  |                                                                                                                       |" << std::endl;
            LILIinput << "              1" << std::endl;
            LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
            LILIinput << "|                                                                 AIRFOIL                                                              |" << std::endl;
            LILIinput << " NACA0000" << std::endl;
            LILIinput << "| N_ETA_COORD  |   ETA_COORD  |   ETA_COORD  |   ETA_COORD  |   ETA_COORD  |   ETA_COORD  |   ETA_COORD  |   ETA_COORD  |      ...      " << std::endl;
            LILIinput << "              1             0.                                                                                                          " << std::endl;
            LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        }
        LILIinput << "|**************************************************************************************************************************************|" << std::endl;
        /**Block 7: Geometrical Data**/
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        LILIinput << "|   INDEX_PW   |   N_PANELS   |  PANEL_DISTR |FACT_CL_ALPHA | FUSELAGE_INF | ELLIPT_CHORD |                                            |" << std::endl;
        LILIinput << "|    X_PW_1    |    Y_PW_1    |    Z_PW_1    |  CHORD_PW_1  |  TWIST_PW_1  | COUPL_COND_1 | FORKING_NO_1 |                             |" << std::endl;
        LILIinput << "|    X_PW_2    |    Y_PW_2    |    Z_PW_2    |  CHORD_PW_2  |  TWIST_PW_2  | COUPL_COND_2 | FORKING_NO_2 |                             |" << std::endl;
        LILIinput << "|--------------------------------------------------------------------------------------------------------------------------------------|" << std::endl;
        for (unsigned int currentLiftingSurface(0); currentLiftingSurface < numberLiftingSurfaces; currentLiftingSurface++) {
            int it = 0;
            for (unsigned int i = 0; i <= this->theLILIWingGeometries.at(currentLiftingSurface).N_PW_Chord - 1; i++) {
                for (int j = this->theLILIWingGeometries.at(currentLiftingSurface).numberOfWingSegments - 1; j >= 0; j--) {
                    // INDEX_PW
                    LILIinput << "             " << it2;
                    // N_PANELS
                    LILIinput << "              " << this->theLILIWingGeometries.at(currentLiftingSurface).N_P.at(j);
                    // PANEL_DISTR
                    if (j == this->theLILIWingGeometries.at(currentLiftingSurface).numberOfWingSegments - 1) {
                        LILIinput << "              1";
                    } else {
                        LILIinput << "              0";
                    }
                    // FACT_CL_ALPHA LILIinput
                    LILIinput << "              1";
                    // FUSELAGE_INF
                    LILIinput << "              0";
                    // ELLIPT_CHORD
                    LILIinput << "              0" <<  std::endl;
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelRight.panelReferencePoint.at(it).xCoordinate;
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelRight.panelReferencePoint.at(it).yCoordinate;
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelRight.panelReferencePoint.at(it).zCoordinate;
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelRight.absoluteChord_PW.at(it);
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelRight.TWIST.at(it);
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelRight.COUPLING_CONDITIONS.at(it);
                    // FORKING_NO
                    LILIinput << "             " << "0" <<  std::endl;
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelLeft.panelReferencePoint.at(it).xCoordinate;
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelLeft.panelReferencePoint.at(it).yCoordinate;
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelLeft.panelReferencePoint.at(it).zCoordinate;
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelLeft.absoluteChord_PW.at(it);
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelLeft.TWIST.at(it);
                    LILIinput << "       " << this->theLILIWingGeometries.at(currentLiftingSurface).panelLeft.COUPLING_CONDITIONS.at(it);
                    // FORKING_NO
                    LILIinput << "             0" << std::endl;
                    it++;
                    it2++;
                }
//                    it++;
            }
        }
        LILIinput << "|=====================================================END OF LIFTING_LINE INPUTFILE====================================================|" << std::endl;
    }
    LILIinput.close();
}
