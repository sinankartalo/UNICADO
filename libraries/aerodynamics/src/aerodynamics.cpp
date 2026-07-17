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

#include "aerodynamics/aerodynamics.h"
#include <aixml/endnode.h>
#include <aixml/node.h>
#include <atmosphere/atmosphere.h>
#include <runtimeInfo/runtimeInfo.h>
#include <standardFiles/functions.h>
#include <algorithm>
#include <cmath>
#include <climits>

bool allowExtrapolation(true);

// cppcheck-suppress unusedFunction
double aerodynamics::getCruiseDrag(const double& machNumber, const double& altitude, const double& bankAngle, const double& m_acft, const std::string& config,
                                   const atmosphere& atm) const {
    double CL = this->getCL(altitude, machNumber, bankAngle, m_acft, atm);
    return this->getCLDrag(machNumber, altitude, CL, config, atm);
}

// cppcheck-suppress unusedFunction
double aerodynamics::getIStabPolar(const double& machNumber, const double& CL, const std::string& config) const {
    if (machNumber <= 0.) {
        return 0.;
    } else {
        std::vector<std::string>::const_iterator entry = find(this->configurationList.begin(), this->configurationList.end(), config);
        if (entry == this->configurationList.end()) { // Entry not available!
            throw("Polar for configuration " + config + " not found!");
        } else {
            uint16_t pos(0);
            pos = static_cast<uint16_t>(entry - this->configurationList.begin());
            double iStab = this->configurations.at(pos).getIStab(CL, machNumber, this->reqCheck);
            return iStab;
        }
    }
}

double aerodynamics::configuration::getIStab(const double& CL, const double& machNumber, const bool& reqCheck) const {
    bool standardCase(false);
    double iStab_returnValue(NAN);
    if (machNumber <= this->machnumbers.front()) {
        standardCase = true;
        myRuntimeInfo->debug << "Current Mach number (Ma = " << machNumber << ") is smaller or equal than first Mach number in polars (Ma = "
                             <<  this->machnumbers.front() << ")." << std::endl;
        myRuntimeInfo->debug << "First Mach number is used. Watch out for extrapolation." << std::endl;
        iStab_returnValue = this->polars.front().getIStab(CL, reqCheck);
    } else if (machNumber >= this->machnumbers.back() && machNumber <= this->MMO) {
        standardCase = true;
        if (machNumber > this->machnumbers.back()) {
             myRuntimeInfo->debug << "Current Mach number (Ma = " << machNumber << ") exceeds last Mach number (Ma = " << this->machnumbers.back()
                                  << ") but is below max. operating Mach number (MMO = " << this->MMO << ")." << std::endl;
        }
        if (configName != "clean") {
            if (machNumber > this->machnumbers.back()) {
                myRuntimeInfo->debug << "Use last polar for this configuration (" << this->configName << ")." << std::endl;
            }
            iStab_returnValue = this->polars.back().getIStab(CL, reqCheck);
        } else if (this->polars.size() > 1) {
            myRuntimeInfo->debug << "Interpolate incidence angle of stabilizer with last two clean polars." << std::endl;
            uint16_t ID_last     = this->polars.size() - 1;
            uint16_t ID_2ndlast  = this->polars.size() - 2;
            double x_rel    = (machNumber - this->machnumbers.at(ID_2ndlast)) / (this->machnumbers.at(ID_last) - this->machnumbers.at(ID_2ndlast));
            double iStab_interpol_tmp = (1. - x_rel) * this->polars.at(ID_2ndlast).getIStab(CL, reqCheck) +
                                     x_rel  * this->polars.at(ID_last).getIStab(CL, reqCheck);
            if (std::isnan(iStab_interpol_tmp)) {
                myRuntimeInfo->debug << "NAN value in incidence angle of stabilizer interpolation function getiStab(CL,M). (Config: " + configName
                                            + ", Ma = " << machNumber << ", CL = " << CL << ")." << std::endl;
                return NAN;
            }
            iStab_returnValue = iStab_interpol_tmp;
        } else {
            standardCase = true;
            myRuntimeInfo->err << "Incidence angle of stabilizer interpolation with last two clean polars failed since only one clean polar is available." << std::endl;
            if (reqCheck) {
                throw std::string("Error in aerodynamics library.");
            } else {
                throw("Abort program!");
            }
        }
    } else if (machNumber >= this->machnumbers.back() && machNumber > this->MMO) {
        standardCase = true;
        myRuntimeInfo->err << "Mach number Ma = " << machNumber << " exceeds Max. operating Mach number = " << this->MMO << "." << std::endl;
        if (reqCheck) {
            throw std::string("Error in aerodynamics library.");
        } else {
            throw("Abort program!");
        }
    } else {
        standardCase = true;
        // Interpolation
        uint16_t i(1);
        while (i < this->machnumbers.size()) {
            if (machNumber > this->machnumbers.at(i)) {
                i++;
            } else {
                int ID_low  = i - 1;
                // If  Mach number is present in polars, it is set to ID_high.
                int ID_high = i;
                /* The following parts check if the requested lift value is valid in the respective polars and if not, whether extrapolation or Mach number grid change is possible.
                   Extrapolation has priority over changing Mach number polar. */

                // Mach number is not present in polars and has to be interpolated.
                if (machNumber != this->machnumbers.at(i)) {
                    // Requested drag needs to be interpolated between two polars, but the lower returns NaN for requested lift value.
                    if (i > 1 && !polars.at(ID_low).isValid(CL)) {
                        // If requested lift value is not valid in lower polar (left interpolation key point) and extrapolation is not allowed,
                        // next lower Mach number is used (if it holds valid data or extrapolation is allowed).
                        if (polars.at(ID_low).allowGridChange) {
                            if (polars.at(ID_low - 1).isValid(CL)) {
                                ID_low--;
                            } else {
                                myRuntimeInfo->err  << "Changing grid point from Ma ["  << machnumbers.at(ID_low) << ", " << machnumbers.at(ID_high)
                                                    << "] -> [" << machnumbers.at(ID_low - 1) << ", " << machnumbers.at(ID_high) << "]"
                                                    << " not possible";
                                if (CL > polars.at(ID_low).getCL_lastValid() && CL > polars.at(ID_low - 1).getCL_lastValid()) {
                                    myRuntimeInfo->err << ", because for both lower grid points CL = " << CL << " > CL_max ("
                                                       << polars.at(ID_low - 1).getCL_lastValid() << ";" << polars.at(ID_low).getCL_lastValid() << "). Abort program!" << std::endl;
                                } else {
                                    myRuntimeInfo->err << ". (CL = " << CL << "). BUG! Abort program!" << std::endl;
                                }
                                throw std::string("Error in aerodynamics library.");
                            }
                        }
                    }
                }
                // Mach number is available (or in between two polars) but requested lift value is NaN.
                if ((i < (this->machnumbers.size() - 1)) && !polars.at(ID_high).isValid(CL)) {
                    // If requested lift value is not valid in high polar (actual Mach number or right interpolation key point) and extrapolation is not allowed,
                    // next higher Mach number is used (if it holds valid data or extrapolation is allowed).
                    if (polars.at(ID_high).allowGridChange) {
                        if (polars.at(ID_high + 1).isValid(CL)) {
                            ID_high++;
                        } else {
                            myRuntimeInfo->err  << "Changing grid point from Ma ["  << machnumbers.at(ID_low) << ", " << machnumbers.at(ID_high)
                                                << "] -> [" << machnumbers.at(ID_low) << ", " << machnumbers.at(ID_high + 1) << "]"
                                                << " not possible";
                            if (CL > polars.at(ID_high).getCL_lastValid() && CL > polars.at(ID_high + 1).getCL_lastValid()) {
                                myRuntimeInfo->err  << ", because for both upper grid points CL = " << CL << " > CL_max ("
                                                    << polars.at(ID_high).getCL_lastValid() << ";" << polars.at(ID_high + 1).getCL_lastValid()
                                                    << ") and extrapolation not allowed." << std::endl;
                            } else {
                                myRuntimeInfo->err << " (CL = " << CL << "). BUG! Abort program!" << std::endl;
                                throw std::string("Error in aerodynamics library.");
                            }
                        }
                    }
                }
                double x_rel = (machNumber - machnumbers.at(ID_low)) / (machnumbers.at(ID_high) - machnumbers.at(ID_low));
                double iStab_interpol(NAN);
                /* If Mach number is present in polars, inc. angle of stab of lower ID does not need to be calculated.
                x_rel cannot equal zero since ID_low is only used for interpolation. */
                if (fabs(x_rel - 1) < ACCURACY_HIGH) {
                    iStab_interpol = polars.at(ID_high).getIStab(CL, reqCheck);
                } else {
                    iStab_interpol = (1. - x_rel) * polars.at(ID_low).getIStab(CL, reqCheck) +
                                     x_rel  * polars.at(ID_high).getIStab(CL, reqCheck);
                }
                if (std::isnan(iStab_interpol)) {
                    myRuntimeInfo->debug << "NAN-value in inc. angle of stabi interpolation function getIStab(CL,M)." <<
                    " (Config: " + configName + ", Ma = " << machNumber << ", CL = " << CL << "). Return NAN" << std::endl;
                    return NAN;
                }
                iStab_returnValue = iStab_interpol;
                break;
            }
        }
    }
    if (!standardCase) {
        myRuntimeInfo->err << "No result in iStab interpolation (Ma = " << machNumber << ", CL = " << CL << "). BUG!" << std::endl;
        if (reqCheck) {
            throw std::string("Error in aerodynamics library.");
        } else {
            throw("Abort program!");
        }
    }
    return iStab_returnValue;
}

double aerodynamics::configuration::polar::getIStab(const double& CL, const bool& reqCheck) const {
    const std::vector <double> *viStab_tmp;
    viStab_tmp = &this->vIStab;
    /* Interpolation of drag polars (CD = f(CL)) */
    if (this->vCL.size() <= 1) {
        return viStab_tmp->at(0);
    } else {
        if (!allowExtrapolation) {
            /* lin. Interpolation */
            if (CL > vCL.at(ID_lastValid)) {
                if (CL > 1.5 * vCL.at(ID_lastValid)) {
                    myRuntimeInfo->warn << "Stall! CLmax exceeded!" << std::endl;
                    myRuntimeInfo->warn << "CL = "                   << CL << std::endl;
                    myRuntimeInfo->warn << "CL_max = "               << vCL.at(ID_CLmax) << std::endl;
                    myRuntimeInfo->warn << "CL (last valid ID) = "   << vCL.at(ID_lastValid) << std::endl;
                    if (reqCheck) {
                        throw std::string("Error in aerodynamics library.");
                    } else {
                        throw("Abort program!");
                    }
                }
                return NAN;
            } else if (CL < vCL.at(ID_firstValid)) {
                return viStab_tmp->at(1) - ((viStab_tmp->at(1) - viStab_tmp->at(0)) / (vCL.at(1) - vCL.at(0)) * (vCL.at(1) - CL));
            } else {
                //Constantly increase CL vector
                uint16_t i(ID_firstValid);
                while (i < ID_lastValid) {
                    if (vCL.at(i + 1) >= CL) {
                        uint16_t foundPos(i);
                        double x_rel = (CL - vCL.at(foundPos)) / (vCL.at(foundPos + 1) - vCL.at(foundPos));
                        return (1. - x_rel) * viStab_tmp->at(foundPos) + x_rel * viStab_tmp->at(foundPos + 1);
                    } else {
                        i++;
                    }
                }
            }
        } else {
            if (CL > vCL.at(ID_lastValid)) {
                myRuntimeInfo->debug << "Extrapolating last valid CL = " << vCL.at(ID_lastValid) << " to requested CL = " << CL << std::endl;
                double x_rel    = (CL - vCL.at(ID_lastValid - 1)) / (vCL.at(ID_lastValid) - vCL.at(ID_lastValid - 1));
                return (1. - x_rel) * viStab_tmp->at(ID_lastValid - 1) +  x_rel * viStab_tmp->at(ID_lastValid);
            } else if (CL < vCL.at(ID_firstValid)) {
                myRuntimeInfo->debug << "Extrapolating first valid CL = " << vCL.at(ID_firstValid) << " to requested CL = " << CL << std::endl;
                double x_rel    = (CL - vCL.at(ID_firstValid)) / (vCL.at(ID_firstValid + 1) - vCL.at(ID_firstValid));
                if (x_rel < -10.) {
                    return viStab_tmp->at(ID_firstValid);
                } else {
                    return (1. - x_rel) * viStab_tmp->at(ID_firstValid) +  x_rel * viStab_tmp->at(ID_firstValid + 1);
                }
            } else {
                //Constantly increase CL vector
                uint16_t i(ID_firstValid);
                while (i < ID_lastValid) {
                    if (vCL.at(i + 1) >= CL) {
                        uint16_t foundPos(i);
                        double x_rel = (CL - vCL.at(foundPos)) / (vCL.at(foundPos + 1) - vCL.at(foundPos));
                        return (1. - x_rel) * viStab_tmp->at(foundPos) + x_rel * viStab_tmp->at(foundPos + 1);
                    } else {
                        i++;
                    }
                }
            }
        }
    }
    myRuntimeInfo->err << "No result in incidence angle of stabilizer interpolation (CL=" << CL << "). BUG!" << std::endl;
    if (reqCheck) {
        throw std::string("Error in aerodynamics library.");
    } else {
        throw("Abort program!");
    }
    return NAN;
}

// cppcheck-suppress unusedFunction
std::vector< std::vector<double> > aerodynamics::getCruiseOriginPolar(const double& machNumber, const double& altitude, const double& bankAngle, const double& m_acft,
                                                                      const std::string& config, const atmosphere& atm) {
    if (machNumber <= 0.) {
        throw("Polar for configuration " + config + " not found!");
    } else {
        std::vector<std::string>::const_iterator entry = find(this->configurationList.begin(), this->configurationList.end(), config);
        if (entry == this->configurationList.end()) { //Entry not available
            throw("Polar for configuration " + config + " not found!");
        } else {
            uint16_t pos(0);
            pos = static_cast<uint16_t>(entry - this->configurationList.begin());
            /* Calulate CL */
            double TAS = atm.getSpeedOfSound(altitude) * machNumber;
            double rho = atm.getDensity(altitude);
            double CL = 2. * m_acft * G_FORCE / (rho * pow(TAS, 2.) * this->wingReferenceArea * cos(bankAngle));
            return this->configurations.at(pos).getOriginPolar(CL, machNumber, useCDalternatePolars, this->reqCheck);
        }
    }
}

// cppcheck-suppress unusedFunction
double aerodynamics::getCruiseLoverD(const double& machNumber, const double& altitude, const double& bankAngle, const double& m_acft, const std::string& config,
                                     const atmosphere& atm) const {
    if (machNumber <= 0.) {
        return 0.;
    } else {
        std::vector<std::string>::const_iterator entry = find(this->configurationList.begin(), this->configurationList.end(), config);
        if (entry == this->configurationList.end()) { // Entry not available
            throw("Polar for configuration " + config + " not found!");
        } else {
            uint16_t pos(0);
            pos = static_cast<uint16_t>(entry - this->configurationList.begin());
            /** Calculate CL **/
            double TAS = atm.getSpeedOfSound(altitude) * machNumber;
            double rho = atm.getDensity(altitude);
            double CL = 2. * m_acft * G_FORCE / (rho * pow(TAS, 2.) * this->wingReferenceArea * cos(bankAngle));
            double CD = configurations.at(pos).getCD(CL, machNumber, useCDalternatePolars, this->reqCheck);
            return CL / CD;
        }
    }
}

double aerodynamics::getCLDrag(const double& machNumber, const double& altitude, const double& CL, const std::string& config, const atmosphere& atm) const {
    if (machNumber <= 0.) {
        return 0.;
    } else {
        std::vector<std::string>::const_iterator entry = find(this->configurationList.begin(), this->configurationList.end(), config);
        if (entry == this->configurationList.end()) { // Entry not available
            throw("Polar for configuration " + config + " not found!");
        } else {
            uint16_t pos(0);
            pos = static_cast<uint16_t>(entry - this->configurationList.begin());
            double TAS = atm.getSpeedOfSound(altitude) * machNumber;
            double rho = atm.getDensity(altitude);
            double CD = this->configurations.at(pos).getCD(CL, machNumber, useCDalternatePolars, this->reqCheck);
            return CD * rho * pow(TAS, 2.) * this->wingReferenceArea / 2.;
        }
    }
}

// cppcheck-suppress unusedFunction
double aerodynamics::getAltitudeAtCL(const double& machNumber, const double& bankAngle, const double& CL, const double& m_acft, const atmosphere& atm) const {
    double altitude(10000.0);
    double TAS = atm.getSpeedOfSound(altitude) * machNumber;
    double rho = 2. * m_acft * G_FORCE / (CL * pow(TAS, 2.) * this->wingReferenceArea * cos(bankAngle));
    double err = altitude - atm.getAltitudeAtDensity(rho);
    int i(0);
    while (fabs(err) > 0.01) {
        i++;
        altitude = altitude - err / 2.;
        TAS = atm.getSpeedOfSound(altitude) * machNumber;
        rho = 2. * m_acft * G_FORCE / (CL * pow(TAS, 2.) * this->wingReferenceArea * cos(bankAngle));
        err = altitude - atm.getAltitudeAtDensity(rho);
        if (i >= 100) {
            myRuntimeInfo->err << "No convergence when determining Altitude from CL-value" << std::endl;
            break;
        }
    }
    return altitude;
}

double aerodynamics::getCL(const double& altitude, const double& machNumber, const double& bankAngle, const double& m_acft, const atmosphere& atm) const {
    double TAS = atm.getSpeedOfSound(altitude) * machNumber;
    double rho = atm.getDensity(altitude);
    return 2. * m_acft * G_FORCE / (rho * pow(TAS, 2.) * this->wingReferenceArea * cos(bankAngle));
}

double aerodynamics::getCLLoverD(const double& machNumber, const double& CL, const std::string& config) const { // cppcheck-suppress unusedFunction
    std::vector<std::string>::const_iterator entry = find(this->configurationList.begin(), this->configurationList.end(), config);
    if (entry == this->configurationList.end()) { // Entry not available
        throw("Polar for configuration " + config + " not found!");
    } else {
        uint16_t pos(0);
        pos = static_cast<uint16_t>(entry - this->configurationList.begin());
        double CD = configurations.at(pos).getCD(CL, machNumber, useCDalternatePolars, this->reqCheck);
        return CL / CD;
    }
}

double aerodynamics::getCLmax(const double& machNumber, const std::string& config) const { // cppcheck-suppress unusedFunction
    std::vector<std::string>::const_iterator entry = find(this->configurationList.begin(), this->configurationList.end(), config);
    if (entry == this->configurationList.end()) { // Entry not available
        throw("Polar for configuration " + config + " not found!");
    } else {
        uint16_t pos(0);
        pos = static_cast<uint16_t>(entry - this->configurationList.begin());
        /** Calculate CL_max **/
        double CL_max(0.);
        if (machNumber <= configurations.at(pos).machnumbers.front()) {
            CL_max = configurations.at(pos).polars.front().getCL_lastValid();
        } else if (machNumber >= configurations.at(pos).machnumbers.back()) {
            if (machNumber <= this->MMO) {
                CL_max = configurations.at(pos).polars.back().getCL_lastValid();
            } else {
                myRuntimeInfo->err << "Error in function aerodynamics::getCLmax(...): machNumber > M_MO." << std::endl;
                if (this->reqCheck) {
                    throw std::string("Errors in aerodynamics library");
                } else {
                    throw("Abort program!");
                }
            }
        } else {
            for (uint16_t i(0); i < configurations.at(pos).machnumbers.size() - 1; i++) {
                if (machNumber >= configurations.at(pos).machnumbers.at(i) && machNumber < configurations.at(pos).machnumbers.at(i + 1)) {
                    if (machNumber == configurations.at(pos).machnumbers.at(i)) {
                        CL_max = configurations.at(pos).polars.at(i).getCL_lastValid();
                    } else {
                        CL_max = std::min(configurations.at(pos).polars.at(i).getCL_lastValid(), configurations.at(pos).polars.at(i + 1).getCL_lastValid());
                    }
                    break;
                }
            }
        }
        /* Check if extrapolation is available to increase maximum CL */
        if (configurations.at(pos).polars.back().extrapolationMargin > 0) {
            CL_max *= configurations.at(pos).polars.back().extrapolationMargin;
        }
        return CL_max;
    }
}

double aerodynamics::maxRangeCL(const double& machNumber, const std::string& config) const { // cppcheck-suppress unusedFunction
    std::vector<std::string>::const_iterator entry = find(this->configurationList.begin(), this->configurationList.end(), config);
    std::vector<double> values;
    std::vector<double> ClVals;
    std::vector<double>::iterator max;
    if (entry == this->configurationList.end()) { // Entry not available
        throw("Polar for configuration " + config + " not found!");
    } else {
        uint16_t confIdx(0);
        confIdx = static_cast<uint16_t>(entry - this->configurationList.begin());
        double CLmax(this->configurations.at(confIdx).polars.front().vCL.back());
        double CLmin(this->configurations.at(confIdx).polars.front().vCL.front());
        double deltaCL((CLmax - CLmin) / 100.);
        /** Get CD **/
        for (uint16_t i(0); i <= 100; i++) {
            double CL(0.);
            double CD(0.);
            CL = CLmin + i * deltaCL;
            CD = this->configurations.at(confIdx).getCD(CL, machNumber, useCDalternatePolars, this->reqCheck);
            ClVals.push_back(CL);
            values.push_back(CL / CD / CD);
        }
        max = max_element(values.begin(), values.end());
        uint16_t pos(0);
        pos = static_cast<uint16_t>(max - values.begin());
        return ClVals.at(pos);
    }
}

void aerodynamics::switchToAlternateDragPolars() { // cppcheck-suppress unusedFunction
    this->useCDalternatePolars = true;
    altPolarsInUse = true;
}

void aerodynamics::switchToDefltDragPolars() { // cppcheck-suppress unusedFunction
    this->useCDalternatePolars = false;
    altPolarsInUse = false;
}

void aerodynamics::switchOnReqSwitch() { // cppcheck-suppress unusedFunction
    this->reqCheck = true;
}

void aerodynamics::switchOffReqSwitch() { // cppcheck-suppress unusedFunction
    this->reqCheck = false;
}

aerodynamics::configuration aerodynamics::getConfiguration(const uint8_t& numberOfConfiguration) const { // cppcheck-suppress unusedFunction
    return this->configurations.at(numberOfConfiguration);
}

double aerodynamics::configuration::getCD(const double& CL, const double& machNumber, const bool& useAlternateDragPolars, const bool& reqCheck) const {
    bool standardCase(false);
    double CD_returnValue(NAN);
    if (machNumber <= this->machnumbers.front()) {
        standardCase = true;
        myRuntimeInfo->debug << "Current Mach number (Ma = " << machNumber << ") is smaller or equal than first Mach number in polars (Ma = "
                             <<  this->machnumbers.front() << ")." << std::endl;
        myRuntimeInfo->debug << "First Mach number is used. Watch out for extrapolation." << std::endl;
        CD_returnValue = this->polars.front().getCD(CL, useAlternateDragPolars, reqCheck);
    } else if (machNumber >= this->machnumbers.back() && machNumber <= this->MMO) {
        standardCase = true;
        if (machNumber > this->machnumbers.back()) {
             myRuntimeInfo->debug << "Current Mach number (Ma = " << machNumber << ") exceeds last Mach number (Ma = " << this->machnumbers.back()
                                  << ") but is below max. operating Mach number (MMO = " << this->MMO << ")." << std::endl;
        }
        if (configName != "clean") {
            if (machNumber > this->machnumbers.back()) {
                myRuntimeInfo->debug << "Use last polar for this configuration (" << this->configName << ")." << std::endl;
            }
            CD_returnValue = this->polars.back().getCD(CL, useAlternateDragPolars, reqCheck);
        } else if (this->polars.size() > 1) {
            myRuntimeInfo->debug << "Interpolate drag with last two clean polars." << std::endl;
            uint16_t ID_last     = this->polars.size() - 1;
            uint16_t ID_2ndlast  = this->polars.size() - 2;
            double x_rel    = (machNumber - this->machnumbers.at(ID_2ndlast)) / (this->machnumbers.at(ID_last) - this->machnumbers.at(ID_2ndlast));
            double CD_interpol_tmp = (1. - x_rel) * this->polars.at(ID_2ndlast).getCD(CL, useAlternateDragPolars, reqCheck) +
                                     x_rel  * this->polars.at(ID_last).getCD(CL, useAlternateDragPolars, reqCheck);
            if (std::isnan(CD_interpol_tmp)) {
                myRuntimeInfo->debug << "NAN value in CD interpolation function getCD(CL,M). (Config: " + configName + ", Ma = " << machNumber
                                    << ", CL = " << CL << ")." << std::endl;
                return NAN;
            }
            CD_returnValue = CD_interpol_tmp;
        } else {
            standardCase = true;
            myRuntimeInfo->err << "Drag interpolation with last two clean polars failed since only one clean polar is available." << std::endl;
            if (reqCheck) {
                throw std::string("Error in aerodynamics library.");
            } else {
                throw("Abort program!");
            }
        }
    } else if (machNumber >= this->machnumbers.back() && machNumber > this->MMO) {
        standardCase = true;
        myRuntimeInfo->err << "Mach number Ma = " << machNumber << " exceeds Max. operating Mach number = " << this->MMO << "." << std::endl;
        if (reqCheck) {
            throw std::string("Error in aerodynamics library.");
        } else {
            throw("Abort program!");
        }
    } else {
        standardCase = true;
        // Interpolation
        uint16_t i(1);
        while (i < this->machnumbers.size()) {
            if (machNumber > this->machnumbers.at(i)) {
                i++;
            } else {
                int ID_low  = i - 1;
                // If  Mach number is present in polars, it is set to ID_high.
                int ID_high = i;
                /* The following parts check if the requested lift value is valid in the respective polars and if not, whether extrapolation or Mach number grid change is possible.
                   Extrapolation has priority over changing Mach number polar. */

                // Mach number is not present in polars and has to be interpolated.
                if (machNumber != this->machnumbers.at(i)) {
                    // Requested drag needs to be interpolated between two polars, but the lower returns NaN for requested lift value.
                    if (i > 1 && !polars.at(ID_low).isValid(CL)) {
                        // If requested lift value is not valid in lower polar (left interpolation key point) and extrapolation is not allowed,
                        // next lower Mach number is used (if it holds valid data or extrapolation is allowed).
                        if (polars.at(ID_low).allowGridChange) {
                            if (polars.at(ID_low - 1).isValid(CL)) {
                                ID_low--;
                            } else {
                                std::stringstream errorMsg;
                                errorMsg << "Changing grid point from Ma ["  << machnumbers.at(ID_low) << ", " << machnumbers.at(ID_high)
                                                    << "] -> [" << machnumbers.at(ID_low - 1) << ", " << machnumbers.at(ID_high) << "]"
                                                    << " not possible";
                                if (CL > polars.at(ID_low).getCL_lastValid() && CL > polars.at(ID_low - 1).getCL_lastValid()) {
                                    errorMsg << ", because for both lower grid points CL = " << CL << " > CL_max ("
                                                       << polars.at(ID_low - 1).getCL_lastValid() << ";" << polars.at(ID_low).getCL_lastValid() << ")" << std::endl;
                                }
                                errorMsg << ". CL boundary reached (CL = " << num2Str(CL) << ") in aerodynamics library.";
                                throwError(__FILE__, __func__, __LINE__, errorMsg.str());
                            }
                        }
                    }
                }
                // Mach number is available (or in between two polars) but requested lift value is NaN.
                if ((i < (this->machnumbers.size() - 1)) && !polars.at(ID_high).isValid(CL)) {
                    // If requested lift value is not valid in high polar (actual Mach number or right interpolation key point) and extrapolation is not allowed,
                    // next higher Mach number is used (if it holds valid data or extrapolation is allowed).
                    if (polars.at(ID_high).allowGridChange) {
                        if (polars.at(ID_high + 1).isValid(CL)) {
                            //myRuntimeInfo->out << "Changing grid point: Ma " << machnumbers.at(ID_high) << " -> " << machnumbers.at(ID_high+1) << "." << std::endl;
                            ID_high++;
                        } else {
                            std::stringstream errorMsg;
                            errorMsg  << "Changing grid point from Ma ["  << machnumbers.at(ID_low) << ", " << machnumbers.at(ID_high)
                                                << "] -> [" << machnumbers.at(ID_low) << ", " << machnumbers.at(ID_high + 1) << "]"
                                                << " not possible";
                            if (CL > polars.at(ID_high).getCL_lastValid() && CL > polars.at(ID_high + 1).getCL_lastValid()) {
                                errorMsg  << ", because for both upper grid points CL = " << CL << " > CL_max ("
                                                    << polars.at(ID_high).getCL_lastValid() << ";" << polars.at(ID_high + 1).getCL_lastValid()
                                                    << ") and extrapolation not allowed" << std::endl;
                            }
                            errorMsg << ". CL boundary reached (CL = " << num2Str(CL) << ") in aerodynamics library.";
                            throwError(__FILE__, __func__, __LINE__, errorMsg.str());
                        }
                    }
                }
                double x_rel = (machNumber - machnumbers.at(ID_low)) / (machnumbers.at(ID_high) - machnumbers.at(ID_low));
                double CD_interpol(NAN);
                // If Mach number is present in polars, drag of lower ID does not need to be calculated. x_rel cannot equal zero since ID_low is only used for interpolation.
                if (fabs(x_rel - 1) < ACCURACY_HIGH) {
                    CD_interpol = polars.at(ID_high).getCD(CL, useAlternateDragPolars, reqCheck);
                } else {
                    CD_interpol = (1. - x_rel) * polars.at(ID_low).getCD(CL, useAlternateDragPolars, reqCheck) +
                                     x_rel  * polars.at(ID_high).getCD(CL, useAlternateDragPolars, reqCheck);
                }
                if (std::isnan(CD_interpol)) {
                    myRuntimeInfo->debug << "NAN-value in CD interpolation function getCD(CL,M). (Config: " + configName + ", Ma = " << machNumber << ", CL = " << CL
                    << "). Return NAN" << std::endl;
                    return NAN;
                }
                CD_returnValue = CD_interpol;
                break;
            }
        }
    }
    if (!standardCase) {
        myRuntimeInfo->err << "No result in CD interpolation (Ma = " << machNumber << ", CL = " << CL << "). BUG!" << std::endl;
        if (reqCheck) {
            throw std::string("Error in aerodynamics library.");
        } else {
            throw("Abort program!");
        }
    } else {
        if (CD_returnValue < 0.) {
            myRuntimeInfo->err << "Invalid drag in interpolation: CD=" << CD_returnValue << " (Config: " << configName << ", Ma = " << machNumber << ", CL = "
                                << CL << ")." << std::endl;
            if (reqCheck) {
                throw std::string("Error in aerodynamics library.");
            } else {
                throw("Abort program!");
            }
        }
    }
    return CD_returnValue;
}

std::vector< std::vector<double> > aerodynamics::configuration::getOriginPolar(const double& CL, const double& machNumber, const bool& useAlternateDragPolars,
                                                                               const bool& reqCheck) {
    std::vector< std::vector<double> > originPolar_returnValue(2);
    std::string originPolar_string;
    bool is_ma(false);
    for (uint16_t i = 0; i < this->machnumbers.size(); ++i) {
        // If the requested Mach number does not match one from the polar, the VC segments and the respective deflections are set to the nearest neighbor.
        if ((machNumber >= this->machnumbers.at(i)) && i != machnumbers.size()-1 && machNumber < machnumbers.at(i+1)) {
            double delta = machNumber - this->machnumbers.at(i);
            if ((machnumbers.at(i+1) - machNumber) < delta) {
                originPolar_string = this->polars.at(i+1).getOriginPolar(CL, useAlternateDragPolars, reqCheck);
            } else {
                originPolar_string = this->polars.at(i).getOriginPolar(CL, useAlternateDragPolars, reqCheck);
            }
            is_ma = true;
            break;
        // Check if requested Mach number is slightly below first Mach number in polar.
        } else if (fabs(machNumber - this->machnumbers.front()) < ACCURACY_LOW && machNumber < this->machnumbers.front()) {
            originPolar_string = this->polars.front().getOriginPolar(CL, useAlternateDragPolars, reqCheck);
            is_ma = true;
            break;
        // Check if requested Mach number is slightly above last Mach number in polar.
        } else if (fabs(machNumber - this->machnumbers.back()) < ACCURACY_LOW && machNumber >= this->machnumbers.back()) {
            originPolar_string = this->polars.back().getOriginPolar(CL, useAlternateDragPolars, reqCheck);
            is_ma = true;
            break;
        }
    }
    if (is_ma) {
        /** Cut of std::string **/
        std::string string_tmp1 = originPolar_string;
        this->numberOfFlapSettings = count(string_tmp1.begin(), string_tmp1.end(), '_');
        ///Extract segment numbers
        for (int j = 0; j < numberOfFlapSettings; ++j) {
            string_tmp1 = string_tmp1.substr(0, (string_tmp1.find_last_of("_")));
            std::string string_tmp_out = string_tmp1.substr(string_tmp1.find_last_of("g") + 1);
            string_tmp1 = string_tmp1.substr(0, (string_tmp1.find_last_of("s")) - 1);
            originPolar_returnValue[0].push_back(atof(string_tmp_out.c_str()));
        }
        reverse(originPolar_returnValue.at(0).begin(), originPolar_returnValue.at(0).end());
        /// Extract deflection angles
        std::string string_tmp2 = originPolar_string;
        for (int j = 0; j < numberOfFlapSettings; ++j) {
            std::string string_tmp_out = string_tmp2.substr(string_tmp2.find_last_of("_") + 1);
            string_tmp2 = string_tmp2.substr(0, (string_tmp2.find_last_of("s")) - 1);
            originPolar_returnValue[1].push_back(atof(string_tmp_out.c_str()));
        }
        reverse(originPolar_returnValue.at(1).begin(), originPolar_returnValue.at(1).end());
    } else {
        throw("Requested Mach number (" + num2Str(machNumber) + ") can neither be found in VC Polar nor has any direct neighbors.");
    }
    return originPolar_returnValue;
}

double aerodynamics::configuration::polar::getCD(const double& CL, const bool& useAlternateDragPolars, const bool& reqCheck) const {
    const std::vector <double> *vCD_tmp;
    if (!useAlternateDragPolars) { // Choice of CD vector (Default or turb.) according to switch position
        vCD_tmp = &this->vCD;
    } else {
        vCD_tmp = &this->vCD_alternate;
    }
    /* Interpolation of drag polars (CD = f(CL)) */
    if (this->vCL.size() <= 1) {
        return vCD_tmp->at(0);
    } else {
        if (this->extrapolationMargin == 1 || ((CL > vCL.at(ID_lastValid)) && (CL / vCL.at(ID_lastValid) > this->extrapolationMargin))
            || ((CL < vCL.at(ID_firstValid)) && (CL / vCL.at(ID_firstValid) < 2 - this->extrapolationMargin))) {  // No extrapolation allowed
            /* lin. Interpolation */
            if (CL > vCL.at(ID_lastValid)) {
                if (CL > 1.5 * vCL.at(ID_lastValid)) {
                    myRuntimeInfo->warn << "Stall! CLmax exceeded!" << std::endl;
                    myRuntimeInfo->warn << "CL = "                   << CL << std::endl;
                    myRuntimeInfo->warn << "CL_max = "               << vCL.at(ID_CLmax) << std::endl;
                    myRuntimeInfo->warn << "CL (last valid ID) = "   << vCL.at(ID_lastValid) << std::endl;
                    if (reqCheck) {
                        throw std::string("Error in aerodynamics library.");
                    } else {
                        throw("Abort program!");
                    }
                }
                return NAN;
            } else if (CL < vCL.at(ID_firstValid)) {
                return vCD_tmp->at(1) - ((vCD_tmp->at(1) - vCD_tmp->at(0)) / (vCL.at(1) - vCL.at(0)) * (vCL.at(1) - CL));
            } else {
                // Constantly increase CL vector
                uint16_t i(ID_firstValid);
                while (i < ID_lastValid) {
                    if (vCL.at(i + 1) >= CL) {
                        uint16_t foundPos(i);
                        double x_rel = (CL - vCL.at(foundPos)) / (vCL.at(foundPos + 1) - vCL.at(foundPos));
                        return (1. - x_rel) * vCD_tmp->at(foundPos) + x_rel * vCD_tmp->at(foundPos + 1);
                    } else {
                        i++;
                    }
                }
            }
        } else {
            if (CL > vCL.at(ID_lastValid)) {
                myRuntimeInfo->debug << "Extrapolating last valid CL = " << vCL.at(ID_lastValid) << " to requested CL = " << CL << std::endl;
                double x_rel    = (CL - vCL.at(ID_lastValid - 1)) / (vCL.at(ID_lastValid) - vCL.at(ID_lastValid - 1));
                return (1. - x_rel) * vCD_tmp->at(ID_lastValid - 1) +  x_rel * vCD_tmp->at(ID_lastValid);
            } else if (CL < vCL.at(ID_firstValid)) {
                myRuntimeInfo->debug << "Extrapolating first valid CL = " << vCL.at(ID_firstValid) << " to requested CL = " << CL << std::endl;
                double x_rel    = (CL - vCL.at(ID_firstValid)) / (vCL.at(ID_firstValid + 1) - vCL.at(ID_firstValid));
                if (x_rel < -10.) {
                    return vCD_tmp->at(ID_firstValid);
                } else {
                    return (1. - x_rel) * vCD_tmp->at(ID_firstValid) +  x_rel * vCD_tmp->at(ID_firstValid + 1);
                }
            } else {
                //Constantly increase CL vector
                uint16_t i(ID_firstValid);
                while (i < ID_lastValid) {
                    if (vCL.at(i + 1) >= CL) {
                        uint16_t foundPos(i);
                        double x_rel = (CL - vCL.at(foundPos)) / (vCL.at(foundPos + 1) - vCL.at(foundPos));
                        return (1. - x_rel) * vCD_tmp->at(foundPos) + x_rel * vCD_tmp->at(foundPos + 1);
                    } else {
                        i++;
                    }
                }
            }
        }
    }
    myRuntimeInfo->err << "No result in CD interpolation (CL=" << CL << "). BUG!" << std::endl;
    if (reqCheck) {
        throw std::string("Error in aerodynamics library.");
    } else {
        throw("Abort program!");
    }
    return NAN;
}

double aerodynamics::configuration::polar::getCL_firstValid() const { // cppcheck-suppress unusedFunction
    if (this->ID_firstValid == UINT16_MAX || this->ID_firstValid >= this->vCL.size()) {
        throw("Return CL (first valid) not possible: invalid ID " + num2Str(ID_firstValid) + ". Abort program!");
    }
    return vCL.at(this->ID_firstValid);
}

double aerodynamics::configuration::polar::getCL_lastValid() const {
    if (this->ID_lastValid == UINT16_MAX || this->ID_lastValid >= this->vCL.size()) {
        throw("Return CL (first valid) not possible: invalid ID " + num2Str(ID_lastValid) + ". Abort program!");
    }
    return vCL.at(this->ID_lastValid);
}

std::string aerodynamics::configuration::polar::getOriginPolar(const double& CL, const bool& useAlternateDragPolars, const bool& reqCheck) const {
    /** Choice of CD vector (Default or turb.) according to switch position **/
    const std::vector <std::string> *vOrigin_polar_tmp;
    if (!useAlternateDragPolars) {
        vOrigin_polar_tmp = &this->vOrigin_polar;
    } else {
        vOrigin_polar_tmp = &this->vOrigin_polar;
    }
    /* Interpolation of drag polars (CD = f(CL)) */
    if (this->vCL.size() <= 1) {
        return vOrigin_polar_tmp->at(0);
    } else {
        if (this->extrapolationMargin == 1 || ((CL > vCL.at(ID_lastValid)) && (CL / vCL.at(ID_lastValid) > this->extrapolationMargin))
            || ((CL < vCL.at(ID_firstValid)) && (CL / vCL.at(ID_firstValid) < 2 - this->extrapolationMargin))) {  // No extrapolation allowed
            /* lin. Interpolation */
            if (CL > vCL.at(ID_lastValid)) {
                if (CL > 1.5 * vCL.at(ID_lastValid)) {
                    myRuntimeInfo->warn << "Stall! CLmax exceeded!" << std::endl;
                    myRuntimeInfo->warn << "CL = "                   << CL << std::endl;
                    myRuntimeInfo->warn << "CL_max = "               << vCL.at(ID_CLmax) << std::endl;
                    myRuntimeInfo->warn << "CL (last valid ID) = "   << vCL.at(ID_lastValid) << std::endl;
                    if (reqCheck) {
                        throw std::string("Error in aerodynamics library.");
                    } else {
                        throw("Abort program!");
                    }
                }
                return "invalid";
            } else if (CL < vCL.at(ID_firstValid)) {
                return vOrigin_polar_tmp->at(ID_firstValid);
            } else {
                //Constantly increase CL vector
                uint16_t i(ID_firstValid);
                while (i < ID_lastValid) {
                    if (vCL.at(i + 1) >= CL) {
                        uint16_t foundPos(i);
                        double assignFlapSetting = (vCL.at(foundPos + 1) - CL) / (CL - vCL.at(foundPos));
                        if (assignFlapSetting < 0) {
                            throw("Error in the assignment of the flap setting");
                        } else if (assignFlapSetting > 0 && assignFlapSetting < 1) {
                            return vOrigin_polar_tmp->at(foundPos + 1);
                        } else if (assignFlapSetting > 1) {
                            return vOrigin_polar_tmp->at(foundPos);
                        }
                    } else {
                        i++;
                    }
                }
            }
        } else {
            if (CL > vCL.at(ID_lastValid)) {
                return vOrigin_polar_tmp->at(ID_lastValid);
            } else if (CL < vCL.at(ID_firstValid)) {
                return vOrigin_polar_tmp->at(ID_firstValid);
            } else {
                //Constantly increase CL vector
                uint16_t i(ID_firstValid);
                while (i < ID_lastValid) {
                    if (vCL.at(i + 1) >= CL) {
                        uint16_t foundPos(i);
                        double assignFlapSetting = (vCL.at(foundPos + 1) - CL) / (CL - vCL.at(foundPos));
                        if (assignFlapSetting < 0) {
                            throw("Error in the assignment of the flap setting");
                        } else if (assignFlapSetting > 0 && assignFlapSetting < 1) {
                            return vOrigin_polar_tmp->at(foundPos + 1);
                        } else if (assignFlapSetting > 1) {
                            return vOrigin_polar_tmp->at(foundPos);
                        }
                    } else {
                        i++;
                    }
                }
            }
        }
    }
    myRuntimeInfo->err << "No result in the assignment of the flap setting (CL=" << CL << "). BUG!" << std::endl;
    if (reqCheck) {
        throw std::string("Error in aerodynamics library.");
    } else {
        throw("Abort program!");
    }
    return "invalid";
}

bool aerodynamics::configuration::polar::isValid(const double& CL) const {
    if ((CL >= vCL.at(this->ID_firstValid) && CL <= vCL.at(this->ID_lastValid)) // CL within bounds
        || (CL > vCL.at(this->ID_lastValid) && (CL / vCL.at(this->ID_lastValid) <= this->extrapolationMargin))
        || (CL < vCL.at(this->ID_firstValid) && (CL / vCL.at(ID_firstValid) >= 2 - this->extrapolationMargin))) {
        return true;
    } else {
        return false;
    }
}

// cppcheck-suppress unusedFunction
double aerodynamics::getAlphaAtFlightCondition(const double& machNumber, const std::string& config, const double& liftCoefficient) const {
    double alpha(NAN);
    if (machNumber <= 0.) {
        return alpha;
    } else {
        std::vector<std::string>::const_iterator entry = find(this->configurationList.begin(), this->configurationList.end(), config);
        if (entry == this->configurationList.end()) { //Entry not available
            throw("Polar for configuration " + config + " not found!");
        } else {
            uint16_t pos(0);
            pos = static_cast<int>(entry - this->configurationList.begin());
            try {
                alpha = this->configurations.at(pos).getAlphaAtCL(liftCoefficient, machNumber);
            } catch (const std::string& text) {
                myRuntimeInfo->err << text << std::endl;
                myRuntimeInfo->err << "Stall! Extrapolation not allowed. No valid alpha for CL = " << liftCoefficient << " and Ma = " << machNumber << std::endl;
            }
            return alpha;
        }
    }
}

double aerodynamics::configuration::getCmAtCLmax(const double& machNumber) const { // cppcheck-suppress unusedFunction
    double CmAtCLmax(0.0);
    if (machNumber <= this->machnumbers.front()) {
        CmAtCLmax = this->polars.front().vCm.back();
    } else if (machNumber >= this->machnumbers.back()) {
        CmAtCLmax = this->polars.back().vCm.back();
    } else {
        uint16_t i(1);
        while (i < this->machnumbers.size()) {
            if (machNumber > this->machnumbers.at(i)) {
                i++;
            } else {
                CmAtCLmax = ((polars.at(i).vCm.back() - polars.at(i - 1).vCm.back()) /
                             (machnumbers.at(i) - machnumbers.at(i - 1)) *
                             (machNumber - machnumbers.at(i - 1)))
                            + polars.at(i - 1).vCm.back();
                return CmAtCLmax;
            }
        }
    }
    return CmAtCLmax;
}

double aerodynamics::configuration::getCLAtAlpha(const double& alpha, const double& machNumber) const { // cppcheck-suppress unusedFunction
    double CLAtAlpha(0.0);
    if (machNumber <= this->machnumbers.front()) {
        CLAtAlpha = getPolarValue(alpha, this->polars.front().vAOA, this->polars.front().vCL, this->polars.front().extrapolationMargin);
    } else if (machNumber >= this->machnumbers.back()) {
        CLAtAlpha = getPolarValue(alpha, this->polars.back().vAOA, this->polars.back().vCL, this->polars.back().extrapolationMargin);
    } else {
        uint16_t i(1);
        while (i < this->machnumbers.size()) {
            if (machNumber > this->machnumbers.at(i)) {
                i++;
            } else {
                CLAtAlpha = ((getPolarValue(alpha, this->polars.at(i).vAOA, this->polars.at(i).vCL, this->polars.at(i).extrapolationMargin) -
                              getPolarValue(alpha, this->polars.at(i - 1).vAOA, this->polars.at(i - 1).vCL,  this->polars.at(i-1).extrapolationMargin)) /
                             (this->machnumbers.at(i) - this->machnumbers.at(i - 1)) * (machNumber - this->machnumbers.at(i - 1)))
                            + getPolarValue(alpha, this->polars.at(i - 1).vAOA, this->polars.at(i - 1).vCL,  this->polars.at(i-1).extrapolationMargin);
                return CLAtAlpha;
            }
        }
    }
    return CLAtAlpha;
}

double aerodynamics::configuration::getAlphaAtCL(const double& CL, const double& machNumber) const {
    double AlphaAtCL(0.0);
    if (machNumber <= this->machnumbers.front()) {
        AlphaAtCL = getPolarValue(CL, this->polars.front().vCL, this->polars.front().vAOA, this->polars.front().extrapolationMargin);
    } else if (machNumber >= this->machnumbers.back()) {
        AlphaAtCL = getPolarValue(CL, this->polars.back().vCL, this->polars.back().vAOA, this->polars.back().extrapolationMargin);
    } else {
        uint16_t i(1);
        while (i < this->machnumbers.size()) {
            if (machNumber > this->machnumbers.at(i)) {
                i++;
            } else {
                AlphaAtCL = ((getPolarValue(CL, this->polars.at(i).vCL, this->polars.at(i).vAOA, this->polars.at(i).extrapolationMargin)
                              - getPolarValue(CL, this->polars.at(i-1).vCL, this->polars.at(i-1).vAOA, this->polars.at(i-1).extrapolationMargin)) /
                             (this->machnumbers.at(i) - this->machnumbers.at(i - 1)) * (machNumber - this->machnumbers.at(i - 1)))
                            + getPolarValue(CL, this->polars.at(i-1).vCL, this->polars.at(i-1).vAOA, this->polars.at(i-1).extrapolationMargin);
                return AlphaAtCL;
            }
        }
    }
    return AlphaAtCL;
}

double aerodynamics::configuration::getCmAtAlpha(const double& alpha, const double& machNumber) const { // cppcheck-suppress unusedFunction
    double CmAtAtAlpha(0.0);
    if (machNumber <= this->machnumbers.front()) {
        CmAtAtAlpha = getPolarValue(alpha, this->polars.front().vAOA, this->polars.front().vCm, this->polars.front().extrapolationMargin);
    } else if (machNumber >= this->machnumbers.back()) {
        CmAtAtAlpha = getPolarValue(alpha, this->polars.back().vAOA, this->polars.back().vCm, this->polars.back().extrapolationMargin);
    } else {
        uint16_t i(1);
        while (i < this->machnumbers.size()) {
            if (machNumber > this->machnumbers.at(i)) {
                i++;
            } else {
                CmAtAtAlpha = ((getPolarValue(alpha, this->polars.at(i).vAOA, this->polars.at(i).vCm, this->polars.at(i).extrapolationMargin) -
                                getPolarValue(alpha, this->polars.at(i-1).vAOA, this->polars.at(i-1).vCm, this->polars.at(i-1).extrapolationMargin)) /
                               (this->machnumbers.at(i) - this->machnumbers.at(i-1)) * (machNumber - this->machnumbers.at(i-1)))
                              + getPolarValue(alpha, this->polars.at(i-1).vAOA, this->polars.at(i-1).vCm, this->polars.at(i-1).extrapolationMargin);
                return CmAtAtAlpha;
            }
        }
    }
    return CmAtAtAlpha;
}

double aerodynamics::configuration::getCmAtCL(const double& CL, const double& machNumber) const { // cppcheck-suppress unusedFunction
    double CmAtAtCL(0.0);
    if (machNumber <= this->machnumbers.front()) {
        CmAtAtCL = getPolarValue(CL, this->polars.front().vCL, this->polars.front().vCm, this->polars.front().extrapolationMargin);
    } else if (machNumber >= this->machnumbers.back()) {
        CmAtAtCL = getPolarValue(CL, this->polars.back().vCL, this->polars.back().vCm, this->polars.back().extrapolationMargin);
    } else {
        uint16_t i(1);
        while (i < this->machnumbers.size()) {
            if (machNumber > this->machnumbers.at(i)) {
                i++;
            } else {
                CmAtAtCL = ((getPolarValue(CL, this->polars.at(i).vCL, this->polars.at(i).vCm, this->polars.at(i).extrapolationMargin)
                             - getPolarValue(CL, this->polars.at(i-1).vCL, this->polars.at(i-1).vCm, this->polars.at(i-1).extrapolationMargin))
                             / (this->machnumbers.at(i) - this->machnumbers.at(i-1)) * (machNumber - this->machnumbers.at(i-1)))
                           + getPolarValue(CL, this->polars.at(i-1).vCL, this->polars.at(i-1).vCm, this->polars.at(i-1).extrapolationMargin);
                return CmAtAtCL;
            }
        }
    }
    return CmAtAtCL;
}

double aerodynamics::configuration::getPolarValue(const double& xValue, const std::vector<double>& x_vec, const std::vector<double>& y_vec,
                                                  const double& extrapolationMargin) const {
    if (x_vec.size() <= 1) {
        return y_vec.at(0);
    } else {
        /** lin. Interpolation **/
        std::vector<double>::const_iterator itMin = std::min_element(x_vec.begin(), x_vec.end());
        std::vector<double>::const_iterator itMax = std::max_element(x_vec.begin(), x_vec.end());
        if (xValue > *(itMax)) {
            if (xValue / *(itMax) <= extrapolationMargin) {
                uint16_t positionOfMaxElement(static_cast<int>(itMax - x_vec.begin()));
                return y_vec.at(positionOfMaxElement - 1) + ((y_vec.at(positionOfMaxElement) - y_vec.at(positionOfMaxElement - 1)) /
                        (x_vec.at(positionOfMaxElement) - x_vec.at(positionOfMaxElement - 1)) * (xValue - x_vec.at(positionOfMaxElement)));
            } else {
                throw("Requested value = " + num2Str(xValue) + " is higher than maximum calculated value = " + num2Str(*(itMax)) + ".");
            }
        } else if (xValue < * (itMin + 1)) {
            //Extrapolation
            return y_vec.at(1) - ((y_vec.at(1) - y_vec.at(0)) / (x_vec.at(1) - x_vec.at(0)) * (x_vec.at(1) - xValue));
        } else {
            //Constantly increase x-value vector
            uint16_t i(0);
            while (i < x_vec.size() - 1) {
                if (x_vec.at(i + 1) >= xValue) {
                    uint16_t foundPos(i);
                    return ((y_vec.at(foundPos + 1) - y_vec.at(foundPos)) / (x_vec.at(foundPos + 1) - x_vec.at(foundPos)) * (xValue - x_vec.at(foundPos))) + y_vec.at(foundPos);
                } else {
                    i++;
                }
            }
        }
    }
    return 0.;
}

/** Constructor aerodynamics **/
aerodynamics::aerodynamics(const node& acXML, const node& polarXML)
    :
    altPolarsInUse(false),
    cruiseConfiguration(""),
    CLmaxTakeoff(0.),
    CLmaxLanding(0.),
    CLoptimumCruise(0.),
    CLgroundRoll(0.),
    acXML(acXML),
    polarXML(polarXML),
    MMO(0.),
    wingReferenceArea(0.),
    polarFileName(acXML.at("aircraft_exchange_file/analysis/aerodynamics/polar/polar_file/value")),
    useCDalternatePolars(false),
    reqCheck(false) {
    const node& acAerodynamics(acXML.at("aircraft_exchange_file/analysis/aerodynamics"));
    //Read reference values from AcftExchangeFile
    MMO = EndnodeReadOnly<double>("requirements_and_specifications/requirements/top_level_aircraft_requirements/flight_envelope/maximum_operating_mach_number").read(acXML).value();
    wingReferenceArea = EndnodeReadOnly<double>("reference_values/S_ref").read(acAerodynamics).value();
    CLmaxTakeoff = EndnodeReadOnly<double>("lift_coefficients/C_LmaxT-O").read(acAerodynamics).value();
    CLmaxLanding = EndnodeReadOnly<double>("lift_coefficients/C_LmaxLanding").read(acAerodynamics).value();
    CLoptimumCruise = EndnodeReadOnly<double>("lift_coefficients/C_LoptimumCruise").read(acAerodynamics).value();
    CLgroundRoll = EndnodeReadOnly<double>("lift_coefficients/C_LgroundRoll").read(acAerodynamics).value();
    myRuntimeInfo->out << "Read aerodynamic data from file: " << std::string(polarXML.getName()) << std::endl;
    /** Read Configuration List and put those into the configurations std::vector **/
    for (uint16_t i(0); i < acAerodynamics.getVector("polar/configuration").size(); i++) {
        configurationList.push_back(EndnodeReadOnly<std::string>("polar/configuration@" + num2Str(i) + "/name").read(acAerodynamics).value());
        configurations.push_back(configuration(configurationList.at(i), this->MMO, polarXML));
        // Get Cruise configuration
        if (EndnodeReadOnly<std::string>("polar/configuration@" + num2Str(i) + "/type").read(acAerodynamics).value() == "cruise") {
            cruiseConfiguration = EndnodeReadOnly<std::string>("polar/configuration@" + num2Str(i) + "/name").read(acAerodynamics).value();
        }
    }
    if (cruiseConfiguration.empty()) {
        throwError(__FILE__, __func__, __LINE__, "Within the aircraft XML's polars, there is no cruise configuration. Abort program!");
    }
    /** Close pointer on polar data **/
    aixml::closeDocument(polarXML);
}

aerodynamics::~aerodynamics() {
    //dtor
}

/** Constructor configuration **/
aerodynamics::configuration::configuration(const std::string& aConfig, const double& myMMO, const node& polarXML)
    :
    configName(aConfig),
    numberOfFlapSettings(1),
    MMO(myMMO) {
    node& configuration(polarXML.at("configurations/configuration@" + aConfig + "/polars"));
    for (uint16_t machID(0); machID < configuration.getVector("polar").size(); machID++) {
        node *tmpPolar(&configuration.at("polar@" + num2Str(machID + 1)));
        machnumbers.push_back((*tmpPolar).at("machnumber"));
        reynoldsNumbers.push_back((*tmpPolar).at("reynoldsnumber"));
        altitudes.push_back((*tmpPolar).at("altitude"));
        polars.push_back(polar(aConfig, machID, (*tmpPolar)));
    }
}
aerodynamics::configuration::~configuration() {
    //dtor
}

/** Constructor polar **/
aerodynamics::configuration::polar::polar(const std::string& aConfig, const int& machID, const node& polar)
    :
    ID_firstValid(SHRT_MAX),
    ID_lastValid(SHRT_MAX),
    ID_CLmin(SHRT_MAX),
    ID_CLmax(SHRT_MAX),
    allowGridChange(polar.getBoolAttrib("AllowGridChange")),
    extrapolationMargin(polar.getDoubleAttrib("ExtrapolationMargin")) {
    /** Read coefficient matrix and save in vectors **/
    std::vector<std::string> vAOA_str, vCL_str, vCD_str, vCD_alternate_str, vCm_str, vIStab_str;
    vAOA_str = tokenize(std::string(polar.at("Alpha")), ";");
    vCL_str = tokenize(std::string(polar.at("Cl")), ";");
    vCD_str = tokenize(std::string(polar.at("Cd")), ";");
    vCm_str = tokenize(std::string(polar.at("Cm")), ";");
    this->vOrigin_polar = tokenize(std::string(polar.at("origin_polar")), ";");
    vIStab_str = tokenize(std::string(polar.at("iStab_polar")), ";");
    if (aConfig == "clean") {
        vCD_alternate_str = tokenize(std::string(polar.at("Cd_turb")), ";");
    } else {
        vCD_alternate_str = tokenize(std::string(polar.at("Cd")), ";");
    }
    //Check if polars are correct
    if (vAOA_str.size() != vCL_str.size()
        || vAOA_str.size() != vCD_str.size()
        || vAOA_str.size() != vCD_alternate_str.size()
        || vAOA_str.size() != vCm_str.size()
        || vAOA_str.size() != vIStab_str.size()
        || vAOA_str.size() != this->vOrigin_polar.size()) {
        throw("In polar XML file, configuration \"" + aConfig + "\", machID \"" + num2Str(machID) + "\": vector sizes in XML file do not have the same size!");
    }
    //TODO(peter#1#) next call already fishy. <value>seg0_15</value> is going to be zero
    if (aConfig == "clean") {
        if (vAOA_str.size() < 5) {
            throw("Error in polar file: not enough entries for Clean-Configuration. Abort program!");
        }
    } else {
        if (vAOA_str.size() < 4) {
            throw("Error in polar file: not enough entries for " + aConfig + "-Configuration. Abort program!");
        }
    }
    for (size_t aPoint(0); aPoint < vAOA_str.size(); aPoint++) {
        this->vAOA.push_back(atof(vAOA_str.at(aPoint).c_str()));
        this->vCL.push_back(atof(vCL_str.at(aPoint).c_str()));
        transform(vCD_str.at(aPoint).begin(), vCD_str.at(aPoint).end(), vCD_str.at(aPoint).begin(), [](const unsigned char& aCharacter) {
            return std::tolower(aCharacter);
        });
        if (vCD_str.at(aPoint) != "nan") {
            this->vCD.push_back(atof(vCD_str.at(aPoint).c_str()));
            this->vCD_alternate.push_back(atof(vCD_alternate_str.at(aPoint).c_str()));
        } else {
            this->vCD.push_back(NAN);
            this->vCD_alternate.push_back(NAN);
        }
        this->vCm.push_back(atof(vCm_str.at(aPoint).c_str()));
        this->vIStab.push_back(atof(vIStab_str.at(aPoint).c_str()));
    }
    /** Find and set first and last valid ID **/
    // TODO(risse#1#): validIDs previously only used in getCD -> also use in other functions, Mantis ID 0000197
    //CD
    for (uint16_t i(0); i < vCD.size(); i++) {
        if (!std::isnan(vCD.at(i))) {
            ID_firstValid = i;
            break;
        }
    }
    for (int i(vCD.size() - 1); i >= 0; i--) {
        if (!std::isnan(vCD.at(i))) {
            ID_lastValid = i;
            break;
        }
    }
    for (uint16_t i(ID_firstValid); i <= ID_lastValid; i++) {
        if (std::isnan(vCD.at(i)) || std::isinf(vCD.at(i))) {
            myRuntimeInfo->err << "Error when creating the polars!" << std::endl;
            myRuntimeInfo->err << "Configuration: " << aConfig << std::endl;
            myRuntimeInfo->err << "Mach ID: " << machID << std::endl;
            myRuntimeInfo->err << "CD = " << vCD.at(i) << std::endl;
            throw("There must be no invalid entry between the first and last valid ID! Abort program!");
        }
    }
    //CD_turb
    if (aConfig == "clean") {
        uint16_t ID_firstValid_turb(SHRT_MAX);
        uint16_t ID_lastValid_turb(SHRT_MAX);
        for (uint16_t i(0); i < vCD_alternate.size(); i++) {
            if (!std::isnan(vCD_alternate.at(i))) {
                ID_firstValid_turb = i;
                break;
            }
        }
        for (int i(vCD_alternate.size() - 1); i >= 0; i--) {
            if (!std::isnan(vCD_alternate.at(i))) {
                ID_lastValid_turb = i;
                break;
            }
        }
        for (uint16_t i(ID_firstValid_turb); i <= ID_lastValid_turb; i++) {
            if (std::isnan(vCD_alternate.at(i)) || std::isinf(vCD_alternate.at(i))) {
                myRuntimeInfo->err << "Error when creating the polars!" << std::endl;
                myRuntimeInfo->err << "Configuration: " << aConfig << std::endl;
                myRuntimeInfo->err << "Mach ID: " << machID << std::endl;
                myRuntimeInfo->err << "CD (turb) = " << vCD_alternate.at(i) << std::endl;
                throw("There must be no invalid entry between the first and last valid ID! Abort program!");
            }
        }
        if (ID_firstValid_turb != this->ID_firstValid) {
            throw("First valid IDs not equal for CD_turb " + num2Str(ID_firstValid_turb) + " and CD " + num2Str(ID_firstValid) + ". Abort program!");
        }
        if (ID_lastValid_turb != this->ID_lastValid) {
// TODO(admin#1#): check and correct this, Mantis ID 0000186
            myRuntimeInfo->warn << "Last valid IDs not equal for CD_turb " << ID_firstValid_turb << " and CD " << ID_firstValid << ". Abort program!" << std::endl;
        }
    }
    /** Check and compare CL_min, CL_max **/
    std::vector<double>::iterator itMin = std::min_element(vCL.begin() + ID_firstValid, vCL.begin() + ID_lastValid + 1);
    std::vector<double>::iterator itMax = std::max_element(vCL.begin() + ID_firstValid, vCL.begin() + ID_lastValid + 1);
    ID_CLmin = static_cast<int>(itMin - vCL.begin());
    ID_CLmax = static_cast<int>(itMax - vCL.begin());
// TODO(risse#3#): For the time being, it is assumed that CL is montonically increasing -> only take into account as soon as post-stable areas are mapped, Mantis ID  0000187
    if (ID_CLmin != ID_firstValid) {
        throw("IDs not equal for first valid " + num2Str(ID_firstValid) + " and smallest CL " + num2Str(ID_CLmin) + ". Abort program!");
    }
    if (ID_CLmax != ID_lastValid) {
        throw("IDs not equal for last valid " + num2Str(ID_lastValid) + " and CL_max " + num2Str(ID_CLmax) + ". Abort program!");
    }
}

aerodynamics::configuration::polar::~polar() {
    //dtor
}
