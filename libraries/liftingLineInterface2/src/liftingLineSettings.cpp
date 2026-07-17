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

#include "liftingLineInterface2/liftingLineSettings.h"
#include <aixml/node.h>
#include <runtimeInfo/runtimeInfo.h>
#include <standardFiles/functions.h>

liftingLineSettings::liftingLineSettings(const std::string& path2Conf) {
    this->getLiftingLineSettings(path2Conf);
}

liftingLineSettings::polarConfig::polarConfig()
    :
    beta(0.),
    numberAoA(0),
    AoA(0.),
    numberCL(0) {
    //ctor
}

void liftingLineSettings::getLiftingLineSettings(const std::string& path2Conf) {
    const node& myLLconfig(aixml::openDocument(path2Conf + "liftingLine_conf.xml"));
    this->programShortName = std::string(myLLconfig.at("/ConfigFile/LiftingLine/ProgramShortName"));
    this->LL_version = std::string(myLLconfig.at("/ConfigFile/LiftingLine/LiftingLineVersion"));
    this->LL_Exe_path = std::string(myLLconfig.at("/ConfigFile/LiftingLine/LiftingLinePath"));
    this->checkLL_Exe_path();
    this->consoleOut = myLLconfig.at("/ConfigFile/LiftingLine/LiftingLineConsoleOutput");
    this->tecplotOut = myLLconfig.at("/ConfigFile/LiftingLine/TecplotOutput");
    this->error_handling = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/ErrorHandling"), 0, true, 2, true);
    // Panel settings
    this->numbChordwisePanels = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/Paneling/ChordwisePanels"), 5, true, 20, true);
    this->numbAddSpanwisePanels = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/Paneling/SpanwiseAddPanels"), 5, true, 30, true);
    this->reductionFactorHTP = myLLconfig.at("/ConfigFile/LiftingLine/Paneling/SpanwiseAddPanels").getDoubleAttrib("HTPFactor");
    if (this->reductionFactorHTP > 1.5 || this->reductionFactorHTP < 0.1) {
        myRuntimeInfo->err << "Value for reductionFactorHTP must be [0.1:1.5]: " << this->reductionFactorHTP << ". Abort program!" << std::endl;
        exit(1);
    }
    this->minSpanPanelsPerSegment = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/Paneling/MinSpanPanelsPerSegment"), 1, true, 7, true);
    this->minValueTip = myLLconfig.at("/ConfigFile/LiftingLine/Paneling/MinSpanPanelsPerSegment").getIntAttrib("MinValueTip");
    if (minValueTip > 7 || minValueTip < 1) {
        myRuntimeInfo->warn << "Value for minValueTip must be [1:7]: " << minValueTip << std::endl;
        myRuntimeInfo->warn << "Set minValueTip to value of minSpanPanelsPerSegment = " << minSpanPanelsPerSegment << std::endl;
        minValueTip = minSpanPanelsPerSegment;
    }
    this->twistDistrType = myLLconfig.at("/ConfigFile/LiftingLine/TwistDistrType");
    this->untwistFslgSeg = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/UntwistFslgSegment"));
    this->referenceArea = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/ReferenceArea"), 0, true, 1000., true);
    this->referenceSpan = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/ReferenceSpan"), 0, true, 1000., true);
    this->spanForScale = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/SpanForScale"), 0, true, 1000., true);
    this->maxRuntime = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/maxRuntime"), 0, true, 1000., true);
    /** Polar Specifications **/
    this->Polar.beta = myLLconfig.at("/ConfigFile/LiftingLine/PolarConfig/Beta");
    if (fabs(this->Polar.beta) > 15.) {
        myRuntimeInfo->err << "Sideslip angle is to big. Beta = " << this->Polar.beta << " deg. Maximum angle allowed: 15 deg." << std::endl;
        exit(1);
    }
    /* Angle of attacks */
    this->Polar.numberAoA = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/PolarConfig/NumbAoAs"), 0, true, 20, true);
    for (int i(0); i <= this->Polar.numberAoA - 1; i++) {
        this->Polar.AoA.push_back(myLLconfig.at("/ConfigFile/LiftingLine/PolarConfig/AoA@" + num2Str(i + 1)));
    }
    /* Lift coefficients */
    this->Polar.numberCL = checkBoundaries(myLLconfig.at("/ConfigFile/LiftingLine/PolarConfig/NumbLiftCoefficients"), 0, true, 10, true);
    for (int i(0); i <= this->Polar.numberCL - 1; i++) {
        this->Polar.liftCoefficient.push_back(myLLconfig.at("/ConfigFile/LiftingLine/PolarConfig/LiftCoefficient@" + num2Str(i + 1)));
    }
    //Check
    if (this->Polar.numberAoA + this->Polar.numberCL < 3) {
        myRuntimeInfo->err << "Too few angles of attack or CL values to be calculated!" << std::endl;
        myRuntimeInfo->err << "AOA: " << this->Polar.numberAoA << std::endl;
        myRuntimeInfo->err << "CL:  " << this->Polar.numberCL << std::endl;
        myRuntimeInfo->err << "At least 3 points are required for the interpolation of the quadratic polars. Change settings in liftingLine_conf.xml! Abort program!" << std::endl;
        exit(1);
    }
    aixml::closeDocument(myLLconfig);
}

void liftingLineSettings::checkLL_Exe_path() {
    std::transform(this->LL_Exe_path.begin(), this->LL_Exe_path.end(), this->LL_Exe_path.begin(), ::toupper);
    if (this->LL_Exe_path.compare(std::string("DEFAULT")) == 0) {
        #ifdef _WIN32
            this->LL_Exe_path = "LiftingLine" + std::string(FILESEPERATOR) + "LIFTING_LINE_WINDOWS_64BIT.exe";
        #else
            this->LL_Exe_path = "LiftingLine" + std::string(FILESEPERATOR) + "LIFTING_LINE_LINUX_64BIT";
        #endif
    }
    if (!fileExists(this->LL_Exe_path)) {
        myRuntimeInfo->err << "Path " << this->LL_Exe_path << " does not exist. Abort program!" << std::endl;
        exit(1);
    }
}
