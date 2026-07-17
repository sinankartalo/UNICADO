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

#include "liftingLineInterface2/liftingLineInterface.h"
#include <runtimeInfo/runtimeInfo.h>
#include <moduleBasics/runtimeIO.h>
#include <standardFiles/functions.h>
#include <aircraftGeometry2/airfoil_surface.h>
#include "liftingLineInterface2/liftingLineInput.h"
#include "liftingLineInterface2/liftingLineOutput.h"


liftingLineInterface::liftingLineInterface(const std::string& path2Conf, const std::string& IoDir)
    :
    myLLsettings(liftingLineSettings(path2Conf)),
    LILI_bkp_dir_created(false),
    ioDir(IoDir) {
    this->initializePointer();
}

void liftingLineInterface::initializePointer() {
    this->theLiftingLineInputPt = new liftingLineInput(this->myLLsettings, this->ioDir);
    this->theLLOutputPt = new liftingLineOutput(*(this->theLiftingLineInputPt));
}

void liftingLineInterface::initializeLiftingLine(const std::vector<geom2::MultisectionSurface<geom2::AirfoilSection>>& liftingSurfaces,
                                                 const std::vector<double>& fuselageWidths,
                                                 const double& aMachNumber, const double& aReynoldsnumber, const double& refArea, const Point& aCoG) {
    this->theLiftingLineInputPt->createLiftingLineInput(liftingSurfaces, fuselageWidths, aMachNumber, aReynoldsnumber, refArea, aCoG);
    // theLLOutputPt needs to be re-initialized to save results for access from external tool
    this->theLLOutputPt = new liftingLineOutput(*theLiftingLineInputPt);
    // Backup of old LiLi Folder if numerical error occurs
    this->LILI_src_dir = this->ioDir + theLiftingLineInputPt->LILIfilename + ".lili.V" + myLLsettings.LL_version;
    this->LILI_bkp_dir = this->LILI_src_dir + "_bkp";
    LILI_bkp_dir_created = this->createBackupDir(this->LILI_src_dir, this->LILI_bkp_dir);
}

bool liftingLineInterface::createBackupDir(const std::string& aSourceDir, const std::string& aBackupDir) {
    bool bkpDirCreated(false);
    if (fileExists(aSourceDir) && this->myLLsettings.error_handling == 1) {
        if (fileExists(aBackupDir)) {
            deleteDirectory(getFullPathString(aBackupDir), false);
        }
        myRuntimeInfo->out << "Create backup directory in ioDir: " << theLiftingLineInputPt->LILIfilename << ".lili.V" << this->myLLsettings.LL_version << "_bkp" << std::endl;
        copyFiles(aSourceDir, aBackupDir);
        bkpDirCreated = true;
    } else {
        if (!fileExists(aSourceDir) && this->myLLsettings.error_handling == 1) {
            myRuntimeInfo->out << "No backup directory created since no aero-data available in " << aSourceDir << "!" << std::endl;
        }
        bkpDirCreated = false;
    }
    return bkpDirCreated;
}

void liftingLineInterface::executeLiftingLine() { // cppcheck-suppress unusedFunction
    myRuntimeInfo->out << "Start LiftingLine (" << this->theLiftingLineInputPt->LILIfilename << ") ... ";
    if (this->myLLsettings.consoleOut) {
        myRuntimeInfo->out << std::endl;
    }
    std::string executionCommand(this->setExecutionCommand());
    #ifndef _WIN32
        handleChildProcess("chmod +x " + this->myLLsettings.LL_Exe_path, "");
    #endif // _WIN32
    myRuntimeInfo->debug << "Execution Command: " << executionCommand << std::endl;
    handleChildProcess(executionCommand, "", this->myLLsettings.maxRuntime);
    if (!this->myLLsettings.consoleOut) {
        myRuntimeInfo->out << "DONE" << std::endl;
    } else {
        myRuntimeInfo->out << "... LiftingLine succeeded." << std::endl;
    }
}

std::string liftingLineInterface::setExecutionCommand() {
    #ifdef _WIN32
        std::string execCommand = this->myLLsettings.LL_Exe_path
                              + " -om:delete -pj:" + replaceAll(this->theLiftingLineInputPt->LILIinputFilepath, "/", "\\")
                              + " -op:" + replaceAll(this->ioDir, "/", "\\") + " -xo:y";
    #else
        std::string execCommand = this->myLLsettings.LL_Exe_path
                              + " -om:delete -pj:" + this->theLiftingLineInputPt->LILIinputFilepath
                              + " -op:" + this->ioDir + " -xo:y";
    #endif
    if (this->myLLsettings.tecplotOut == true) {
        execCommand += " -tp:y";
    } else {
        execCommand += " -tp:n";
    }
    if (!this->myLLsettings.consoleOut) {
        execCommand += " -q";
    }
    return execCommand;
}
