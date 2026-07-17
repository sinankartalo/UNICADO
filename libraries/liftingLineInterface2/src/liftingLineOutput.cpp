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

#include "liftingLineInterface2/liftingLineOutput.h"
#include <aixml/node.h>
#include <runtimeInfo/runtimeInfo.h>
#include <standardFiles/functions.h>
#include <algorithm>

liftingLineOutput::liftingLineOutput(const liftingLineInput &myLLinput)
    :
    myLLinput(myLLinput) {
    // ctor
}

void liftingLineOutput::readLiftingLineResults(int numberLiftingSurfaces) {  // cppcheck-suppress unusedFunction
    myRuntimeInfo->out << "Read LiftingLine results (" << myLLinput.LILIfilename << ") ..." << std::endl;
    std::string polarFile;
    double refAreaTotal(0.), refLengthTotal(0.);
    std::vector<double> refAreaWings, refLengthWings, alpha_temp;
    int numbResults(0);
    polarFile = myLLinput.ioDir + myLLinput.LILIfilename + ".lili.V" + this->myLLinput.myLLsettings.LL_version + FILESEPERATOR + "export" + FILESEPERATOR +
                myLLinput.LILIfilename + ".xml";
    const node& LL_XML_tmp = aixml::openDocument(polarFile);
    /* Initialize LiftingLine polars */
    for (int j = 0; j <= numberLiftingSurfaces - 1; j++) {
        llPolarWings.push_back(liftingLinePolarWings());
        std::stringstream temp1;
        temp1.width(6);
        temp1 << j + 1;
        std::string str_id = temp1.str();
        refAreaWings.push_back(LL_XML_tmp.at("lifting_line-exchange-file/wing_data/wing@number=" + str_id + "/geometry_parameters/reference_area"));
        refLengthWings.push_back(LL_XML_tmp.at("lifting_line-exchange-file/wing_data/wing@number=" + str_id + "/geometry_parameters/reference_length_cmy"));
        llPolarWings.back().numberOfSegments = LL_XML_tmp.at("lifting_line-exchange-file/wing_data/wing@number=" + str_id + "/segment_data").getIntAttrib("count");
        for (int k(0); k < llPolarWings.back().numberOfSegments; k++) {
            std::stringstream seg_ID;
            seg_ID.width(6);
            seg_ID << k + 1;
            llPolarWings.back().yPositions.push_back(LL_XML_tmp.at("lifting_line-exchange-file/wing_data/wing@number=" + str_id
                    + "/segment_data/segment@number=" + seg_ID.str() + "/geometry_parameters/moment_reference_point_y"));
        }
    }
    refAreaTotal    = LL_XML_tmp.at("lifting_line-exchange-file/global_data/geometry_parameters/reference_area");
    refLengthTotal  = LL_XML_tmp.at("lifting_line-exchange-file/global_data/geometry_parameters/reference_length_cmy");
    for (int i(0); i < this->myLLinput.myLLsettings.Polar.numberAoA + this->myLLinput.myLLsettings.Polar.numberCL; i++) {
        llPolar.push_back(liftingLinePolar());  // Total aircraft
        for (int j(0); j < numberLiftingSurfaces; j++) {
            llPolarWings.at(j).llPolarW.push_back(liftingLinePolarWithSegments());  // With reference to liftingSurface
            for (int k(0); k < llPolarWings.at(j).numberOfSegments; k++) {
                llPolarWings.at(j).llPolarW.back().wingSegments.push_back(liftingLineSegment());
            }
        }
    }
    numbResults = LL_XML_tmp.at("lifting_line-exchange-file/result_data").getIntAttrib("count");  // Equals number of alphas/CLs
    double polarP[4][numbResults];
    // Reading and assigning the LiftingLine results (aircraft)
    for (int i = 0; i <= numbResults - 1; i++) {  // Loop over AoAs
        std::stringstream temp1;
        temp1.width(6);
        temp1 << i + 1;
        std::string str_id = temp1.str();
        polarP[0][i] = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id + "/angle_of_attack");
        // For global aircraft the coefficient in z-wise direction is used.
        polarP[1][i] = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id + "/cfz");
        polarP[2][i] = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id + "/cfx_from_cdi");
        polarP[3][i] = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id + "/cmy");
        if (std::isnan(polarP[1][i]) || std::isnan(polarP[2][i]) || std::isnan(polarP[3][i])) {
            myRuntimeInfo->err << "Invalid results in file " << polarFile << "!" << std::endl;
            myRuntimeInfo->err << "AoA=" << polarP[0][i] << ", Cfz=" << polarP[1][i] << ", Cfx=" << polarP[2][i] << ", Cmy=" << polarP[3][i] << ". Abort program!" << std::endl;
            exit(1);
        }
        alpha_temp.push_back(polarP[0][i]);
    }
    // Sort polar points by angles of attack
    sort(alpha_temp.begin(), alpha_temp.end());
    for (int i = 0; i <= numbResults - 1; i++) {
        for (int j = 0; j <= numbResults - 1; j++) {
            if (polarP[0][j] == alpha_temp.at(i)) {
                llPolar.at(i).AoA = polarP[0][j];
                llPolar.at(i).CL = polarP[1][j];
                llPolar.at(i).CDind = polarP[2][j];
                llPolar.at(i).CM = polarP[3][j];
                break;
            }
        }
    }
    alpha_temp.clear();
    // Reading and assigning the LiftingLine results (for each wing individually)
    for (size_t n = 0; n <= llPolarWings.size() - 1; n++) {
        for (int i = 0; i <= numbResults - 1; i++) {  // Loop over all AoAs
            std::stringstream temp1;
            std::stringstream temp2;
            temp1.width(6);
            temp2.width(6);
            temp1 << i + 1;
            temp2 << n + 1;
            std::string str_id = temp1.str();  // Refers to alpha
            std::string str_id2 = temp2.str();  // Refers to wingNumber
            polarP[0][i] = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id + "/angle_of_attack");
            // For wings (global) the coefficient in z-wise direction is used.
            polarP[1][i] = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id + "/wing@number=" + str_id2 + "/cfz");
            polarP[2][i] = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id + "/wing@number=" + str_id2 + "/cfx_from_cdi");
            polarP[3][i] = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id + "/wing@number=" + str_id2 + "/cmy");
            for (int k(0); k < llPolarWings.at(n).numberOfSegments; k++) {  // Loop over all LiftingLine segments
                std::stringstream temp3;
                temp3.width(6);
                temp3 << k + 1;
                std::string str_id3 = temp3.str();  // Refers to segmentNumber
                llPolarWings.at(n).llPolarW.at(i).wingSegments.at(k).yPos = llPolarWings.at(n).yPositions.at(k);
                llPolarWings.at(n).llPolarW.at(i).wingSegments.at(k).llPolarSeg = new liftingLinePolar();
                llPolarWings.at(n).llPolarW.at(i).wingSegments.at(k).llPolarSeg->AoA = polarP[0][i];
                // In contrast to global CL, for segment CL the coefficient perpendicular to the surface is used to take dihedral into account!
                llPolarWings.at(n).llPolarW.at(i).wingSegments.at(k).llPolarSeg->CL    = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id +
                                                                                                       "/wing@number=" + str_id2 + "/segment@number=" + str_id3 + "/cfnormal")
                                                                                         * refAreaWings.at(n) / refAreaTotal;
                // Get segment area
                double segmentArea = LL_XML_tmp.at("lifting_line-exchange-file/wing_data/wing@number=" + str_id2 + "/segment_data/segment@number=" + str_id3 +
                                                   "/geometry_parameters/reference_area");
                // Get CDind of segment and account for the segments area. Attention: This is only for one wing side.
                llPolarWings.at(n).llPolarW.at(i).wingSegments.at(k).llPolarSeg->CDind = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id +
                                                                                                       "/wing@number=" + str_id2 + "/segment@number=" + str_id3 + "/cfx_from_cdi")
                                                                                         * segmentArea / refAreaTotal;
                llPolarWings.at(n).llPolarW.at(i).wingSegments.at(k).llPolarSeg->CM    = LL_XML_tmp.at("lifting_line-exchange-file/result_data/result@number=" + str_id +
                                                                                                       "/wing@number=" + str_id2 + "/segment@number=" + str_id3 + "/cmy");
            }
            alpha_temp.push_back(polarP[0][i]);
        }
        /* Sort polar points by angles of attack */
        sort(alpha_temp.begin(), alpha_temp.end());
        for (int i = 0; i <= numbResults - 1; i++) {
            for (int j = 0; j <= numbResults - 1; j++) {
                if (polarP[0][j] == alpha_temp.at(i)) {
                    llPolarWings.at(n).llPolarW.at(i).AoA = polarP[0][j];
                    llPolarWings.at(n).llPolarW.at(i).CL = polarP[1][j] * refAreaWings.at(n) / refAreaTotal;
                    llPolarWings.at(n).llPolarW.at(i).CDind = polarP[2][j] * refAreaWings.at(n) / refAreaTotal;
                    llPolarWings.at(n).llPolarW.at(i).CM = polarP[3][j] * refAreaWings.at(n) / refAreaTotal * refLengthWings.at(n) / refLengthTotal;
                    break;
                }
            }
        }
        alpha_temp.clear();
    }
    aixml::closeDocument(LL_XML_tmp);
}

void liftingLineOutput::getAnalyticalPolar() {  // cppcheck-suppress unusedFunction
    std::vector<double> alpha, liftCoeff, indDragCoeff, momentCoeff, liftRegrCoeff, momRegrCoeff, dragRegrCoeff;
    for (size_t i = 0; i <= llPolar.size() - 1; i++) {
        alpha.push_back(llPolar.at(i).AoA);
        liftCoeff.push_back(llPolar.at(i).CL);
        indDragCoeff.push_back(llPolar.at(i).CDind);
        momentCoeff.push_back(llPolar.at(i).CM);
    }
    /* Determination of analytical lift/induced drag polars */
    // Linear fit for CL-Alpha-Polar
    liftRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(alpha, liftCoeff, 1);
    this->dCLtodAoA = liftRegrCoeff.at(1);
    this->CLatAoA0 = liftRegrCoeff.at(0);
    liftRegrCoeff.clear();
    // Linear fit for CM-Alpha-Polar
    momRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(alpha, momentCoeff, 1);
    this->dCMtodAoA = momRegrCoeff.at(1);
    this->CMatAoA0  = momRegrCoeff.at(0);
    momRegrCoeff.clear();
    // Linear fit for CM-CL-Polar
    momRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(liftCoeff, momentCoeff, 1);
    this->dCMtodCL = momRegrCoeff.at(1);
    this->CMatCL0  = momRegrCoeff.at(0);
    momRegrCoeff.clear();
    /* Quadratic fit for CD-ind-CL-Polar.
     * Starting point: CDind = a + b*CL + c*CL^2
     * CDmin = dCDind/dCL = 0
     */
    dragRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(liftCoeff, indDragCoeff, 2);
    this->CDmin     = dragRegrCoeff.at(0) - 0.25 * pow(dragRegrCoeff.at(1), 2) / dragRegrCoeff.at(2);
    this->kFactor   = dragRegrCoeff.at(2);
    this->CLatCDmin = -0.5 * dragRegrCoeff.at(1) / dragRegrCoeff.at(2);
    dragRegrCoeff.clear();
}

// cppcheck-suppress unusedFunction
void liftingLineOutput::getAnalyticalPolarLiftingSurface(const liftingLinePolars::liftingLinePolarWings& aLiftingSurfacePolar, const uint16_t& llPolarSize) {
    std::vector<double> alpha, liftCoeff, indDragCoeff, momentCoeff, liftRegrCoeff, momRegrCoeff, dragRegrCoeff;
    for (int i(0); i <= llPolarSize - 1; i++) {
        alpha.push_back(aLiftingSurfacePolar.llPolarW.at(i).AoA);
        liftCoeff.push_back(aLiftingSurfacePolar.llPolarW.at(i).CL);
        indDragCoeff.push_back(aLiftingSurfacePolar.llPolarW.at(i).CDind);
        momentCoeff.push_back(aLiftingSurfacePolar.llPolarW.at(i).CM);
    }
    /* Determination of analytical lift/induced drag polars */
    // Linear fit for CL-Alpha-Polar
    liftRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(alpha, liftCoeff, 1);
    this->liftingSurface_dCLtodAoA.push_back(liftRegrCoeff.at(1));
    this->liftingSurface_CLatAoA0.push_back(liftRegrCoeff.at(0));
    liftRegrCoeff.clear();
    // Linear fit for CM-Alpha-Polar
    momRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(alpha, momentCoeff, 1);
    this->liftingSurface_dCMtodAoA.push_back(momRegrCoeff.at(1));
    this->liftingSurface_CMatAoA0.push_back(momRegrCoeff.at(0));
    momRegrCoeff.clear();
    // Linear fit for CM-CL-Polar
    momRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(liftCoeff, momentCoeff, 1);
    this->liftingSurface_dCMtodCL.push_back(momRegrCoeff.at(1));
    this->liftingSurface_CMatCL0.push_back(momRegrCoeff.at(0));
    momRegrCoeff.clear();
    /* Quadratic fit for CD-ind-CL-Polar.
     * Starting point: CDind = a + b*CL + c*CL^2
     * CDmin = dCDind/dCL = 0
     */
    dragRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(liftCoeff, indDragCoeff, 2);
    this->liftingSurface_CDmin.push_back(dragRegrCoeff.at(0) - 0.25 * pow(dragRegrCoeff.at(1), 2) / dragRegrCoeff.at(2));
    this->liftingSurface_kFactor.push_back(dragRegrCoeff.at(2));
    this->liftingSurface_CLatCDmin.push_back(-0.5 * dragRegrCoeff.at(1) / dragRegrCoeff.at(2));
    dragRegrCoeff.clear();
}

// cppcheck-suppress unusedFunction
void liftingLineOutput::getAnalyticalPolarLiftingSurfacePart(const liftingLinePolars::liftingLinePolarWings& aLiftingSurfacePolar, const uint16_t& llPolarSize) {
    std::vector<double> alpha, liftCoeff, indDragCoeff, momentCoeff, liftRegrCoeff, momRegrCoeff, dragRegrCoeff;
    for (int i(0); i <= llPolarSize - 1; i++) {
        alpha.push_back(aLiftingSurfacePolar.llPolarW.at(i).AoA);
        liftCoeff.push_back(aLiftingSurfacePolar.llPolarW.at(i).CL);
        indDragCoeff.push_back(aLiftingSurfacePolar.llPolarW.at(i).CDind);
        momentCoeff.push_back(aLiftingSurfacePolar.llPolarW.at(i).CM);
    }
    /* Determination of analytical lift/induced drag polars */
    // Linear fit for CL-Alpha-Polar
    liftRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(alpha, liftCoeff, 1);
    this->liftingSurfacePart_dCLtodAoA = liftRegrCoeff.at(1);
    this->liftingSurfacePart_CLatAoA0 = liftRegrCoeff.at(0);
    liftRegrCoeff.clear();
    // Linear fit for CM-Alpha-Polar
    momRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(alpha, momentCoeff, 1);
    this->liftingSurfacePart_dCMtodAoA = momRegrCoeff.at(1);
    this->liftingSurfacePart_CMatAoA0 = momRegrCoeff.at(0);
    momRegrCoeff.clear();
    // Linear fit for CM-CL-Polar
    momRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(liftCoeff, momentCoeff, 1);
    this->liftingSurfacePart_dCMtodCL = momRegrCoeff.at(1);
    this->liftingSurfacePart_CMatCL0 = momRegrCoeff.at(0);
    momRegrCoeff.clear();
    /* Quadratic fit for CD-ind-CL-Polar.
     * Starting point: CDind = a + b*CL + c*CL^2
     * CDmin = dCDind/dCL = 0
     */
    dragRegrCoeff = calcRegressionCoefficientsUsingQRdecomp(liftCoeff, indDragCoeff, 2);
    this->liftingSurfacePart_CDmin = dragRegrCoeff.at(0) - 0.25 * pow(dragRegrCoeff.at(1), 2) / dragRegrCoeff.at(2);
    this->liftingSurfacePart_kFactor = dragRegrCoeff.at(2);
    this->liftingSurfacePart_CLatCDmin = -0.5 * dragRegrCoeff.at(1) / dragRegrCoeff.at(2);
    dragRegrCoeff.clear();
}
