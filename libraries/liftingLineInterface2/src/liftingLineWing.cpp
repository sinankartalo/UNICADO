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

#include "liftingLineInterface2/liftingLineWing.h"
#include <runtimeInfo/runtimeInfo.h>
#include <standardFiles/functions.h>
#include <unitConversion/constants.h>
#include <unitConversion/unitConversion.h>
#include <aircraftGeometry2/airfoil_surface.h>
#include <aircraftGeometry2/processing/measure.h>
#include <svl/Vec2.h>
#include <algorithm>
#include <numeric>

using std::stringstream;

liftingLineWing::liftingLineWing()
    :
    theInputPanelDistribution({0, 0, 0, 0}),
    N_PW_Chord(0),
    N_PW_Span(0),
    totalPanel(0),
    numberOfWingSegments(0) {
    // ctor
}

void liftingLineWing::generateLILIgeometry(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface,
                                            const double& fuselageWidth,
                                            const double& reductionFactorHTPpanels, const bool& untwistFslgSeg) {
    double scaleSpanErrors = 1.0;
    // Set panel distribution
    myRuntimeInfo->out << "Set panel distribution for the " << anAircraftLiftingSurface.name << " lifting surface ..." << std::endl;
    this->setPanelDistribution(anAircraftLiftingSurface, reductionFactorHTPpanels);
    // Set panel properties (chord, referencePoint, coupling condition for each panel)
    this->setPanelProperties(anAircraftLiftingSurface, fuselageWidth, scaleSpanErrors, untwistFslgSeg);
}

void liftingLineWing::setPanelDistribution(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const double& reductionFactorHTPpanels) {
    double halfSpan(fabs(anAircraftLiftingSurface.sections.front().origin.z() - anAircraftLiftingSurface.sections.back().origin.z()));
    this->N_PW_Chord = this->theInputPanelDistribution.numberOfChordwisePanels;
    this->N_PW_Span  = this->theInputPanelDistribution.numberOfAdditionalSpanwisePanels;
    // Panel reduction depending on name not possible with new aircraft geometry!
    if (anAircraftLiftingSurface.name != "main_wing") {
        this->N_PW_Span *= reductionFactorHTPpanels;
    }
    /** Panel per wing Segment */
    this->N_P.clear();
    this->numberOfWingSegments = anAircraftLiftingSurface.sections.size() - 1;
    // if (anAircraftLiftingSurface.RightWing.at(anAircraftLiftingSurface.RightWing.size() - 2).s_segment == 0.) <- this is weird and fucks up single segment wings!
    if (accuracyCheck(getTrueCoordinates(anAircraftLiftingSurface, 0).yCoordinate -
                        getTrueCoordinates(anAircraftLiftingSurface, 1).yCoordinate, 0., ACCURACY_HIGH)) {
        this->numberOfWingSegments--;
    }
    for (unsigned int i(0); i < this->numberOfWingSegments; i++) {
        int min_value_tmp = this->theInputPanelDistribution.numberOfMinimumSpanwisePanelsPerSegment;
        /*  Panel reduction depending on name not possible with new aircraft geometry!
        if (anAircraftLiftingSurface.sections.at(i).name == "Tip") {
            min_value_tmp = this->theInputPanelDistribution.numberOfMinimumSpanwisePanelsForTipSegment;
        }
        */
        if (fabs(getTrueCoordinates(anAircraftLiftingSurface, i).yCoordinate - getTrueCoordinates(anAircraftLiftingSurface, i + 1).yCoordinate) > 0.) {
            this->N_P.push_back(std::max(min_value_tmp,
                                         static_cast<int>(1 + floor(static_cast<double>(this->N_PW_Span) *
                                        (fabs(getTrueCoordinates(anAircraftLiftingSurface, i).yCoordinate
                                            - getTrueCoordinates(anAircraftLiftingSurface, i + 1).yCoordinate)) / halfSpan))));
        }
    }
    this->N_PW_Span = accumulate(this->N_P.begin(), this->N_P.end(), 0);
    this->totalPanel = this->N_PW_Chord * this->N_PW_Span;
}

void liftingLineWing::setPanelProperties(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface,
                                        const double& fuselageWidth,
                                        const double& myScaleSpanErrors, const bool& untwistFslgSeg) {
    int theLiftingSurfaceSegmentSize(anAircraftLiftingSurface.sections.size() - 1);
    // dont throw a range error for 1 segment wings YET !
    if (theLiftingSurfaceSegmentSize > 1) {
        // Skip last wing segment if segment span is zero (but why? LN 2024)
        if (accuracyCheck(getTrueCoordinates(anAircraftLiftingSurface, theLiftingSurfaceSegmentSize).yCoordinate -
                        getTrueCoordinates(anAircraftLiftingSurface, theLiftingSurfaceSegmentSize - 1).yCoordinate, 0., ACCURACY_HIGH)) {
            theLiftingSurfaceSegmentSize--;
        }
    }
    for (unsigned int chordwisePanelID(0); chordwisePanelID <= this->N_PW_Chord - 1; ++chordwisePanelID) {
        // Set properties for each wing segment (excluding twist and virtual segment)
        for (int anLiftingSurfaceSectionID = theLiftingSurfaceSegmentSize -1 ; anLiftingSurfaceSectionID >= 0; anLiftingSurfaceSectionID--) {
            // Set relative chord of panel (equidistant distribution)
            this->panelRight.absoluteChord_PW.push_back(anAircraftLiftingSurface.sections.at(anLiftingSurfaceSectionID + 1).get_chord_length() / this->N_PW_Chord);
            this->panelLeft.absoluteChord_PW.push_back(anAircraftLiftingSurface.sections.at(anLiftingSurfaceSectionID).get_chord_length() / this->N_PW_Chord);
            // Calculate panel coordinates for the quarter chord of each panel as interpolation between leading edge and trailing edge
            Point panelCoordinate_right(0., 0., 0.), panelCoordinate_left(0., 0., 0.);
            Point LE_point_inboard;   // left
            Point LE_point_outboard;  // right
            Point TE_point_inboard;   // left
            Point TE_point_outboard;  // right
            LE_point_inboard = getTrueCoordinates(anAircraftLiftingSurface, anLiftingSurfaceSectionID);
            LE_point_outboard = getTrueCoordinates(anAircraftLiftingSurface, anLiftingSurfaceSectionID + 1);
            TE_point_inboard = getTECoordinates(LE_point_inboard, anAircraftLiftingSurface.sections.at(anLiftingSurfaceSectionID).get_chord_length(),
                                                anAircraftLiftingSurface.sections.at(anLiftingSurfaceSectionID).get_twist_angle());
            TE_point_outboard = getTECoordinates(LE_point_outboard, anAircraftLiftingSurface.sections.at(anLiftingSurfaceSectionID + 1).get_chord_length(),
                                                anAircraftLiftingSurface.sections.at(anLiftingSurfaceSectionID + 1).get_twist_angle());
            double x_rel = (chordwisePanelID + 0.25) / this->N_PW_Chord;  // Rel. quarterchord point of each panel chord
            panelCoordinate_right.xCoordinate = ((1. - x_rel) * LE_point_outboard.xCoordinate + x_rel * TE_point_outboard.xCoordinate);
            panelCoordinate_left.xCoordinate = ((1. - x_rel) * LE_point_inboard.xCoordinate + x_rel * TE_point_inboard.xCoordinate);
            panelCoordinate_right.yCoordinate = (((1. - x_rel) * LE_point_outboard.yCoordinate + x_rel * TE_point_outboard.yCoordinate) * myScaleSpanErrors);
            panelCoordinate_left.yCoordinate = (((1. - x_rel) * LE_point_inboard.yCoordinate + x_rel * TE_point_inboard.yCoordinate) * myScaleSpanErrors);
            panelCoordinate_right.zCoordinate = ((1. - x_rel) * LE_point_outboard.zCoordinate + x_rel * TE_point_outboard.zCoordinate);
            panelCoordinate_left.zCoordinate = ((1. - x_rel) * LE_point_inboard.zCoordinate + x_rel * TE_point_inboard.zCoordinate);
            this->panelRight.panelReferencePoint.push_back(panelCoordinate_right);
            this->panelLeft.panelReferencePoint.push_back(panelCoordinate_left);
            // Define coupling conditions between part wings
            if (anLiftingSurfaceSectionID == theLiftingSurfaceSegmentSize - 1) {
                // Set special wingtip conditions for right side of last segment
                this->panelRight.COUPLING_CONDITIONS.push_back("T F F F F F");
                // Set normal conditions for left side
                if (theLiftingSurfaceSegmentSize - 1 == 0) {
                    this->panelLeft.COUPLING_CONDITIONS.push_back("F T F F F F");
                } else {
                    this->panelLeft.COUPLING_CONDITIONS.push_back("F F T T F F");
                }
            } else if (anLiftingSurfaceSectionID == 0) {
                this->panelRight.COUPLING_CONDITIONS.push_back("F F T T F F");
                this->panelLeft.COUPLING_CONDITIONS.push_back("F T F F F F");
            } else {
                this->panelRight.COUPLING_CONDITIONS.push_back("F F T T F F");
                this->panelLeft.COUPLING_CONDITIONS.push_back("F F T T F F");
            }
            // Camber line
            camberLine tmpCamberLine_left(this->setPanelCamberLine(anAircraftLiftingSurface, anLiftingSurfaceSectionID));
            camberLine tmpCamberLine_right(this->setPanelCamberLine(anAircraftLiftingSurface, anLiftingSurfaceSectionID + 1));
            this->panelRight.panelCamberLine.push_back(tmpCamberLine_right);
            this->panelLeft.panelCamberLine.push_back(tmpCamberLine_left);
            // Segment twist
            // the twist is multiplied by -1 because of all the coordinate transformations, I dont understand but it works (LN 2024)
            double tmpTwist_left(this->setPanelTwist(anAircraftLiftingSurface, anLiftingSurfaceSectionID, chordwisePanelID,
                                                        this->panelLeft.panelCamberLine.back(), this->panelLeft.absoluteChord_PW.back()) * (-1.));
            double tmpTwist_right(this->setPanelTwist(anAircraftLiftingSurface, anLiftingSurfaceSectionID + 1 , chordwisePanelID,
                                                        this->panelRight.panelCamberLine.back(), this->panelRight.absoluteChord_PW.back()) * (-1.));
            // Check if segment is within the fuselage
            if (untwistFslgSeg) {  // Fuselage segment
                if (fabs(LE_point_outboard.yCoordinate) - (fabs(fuselageWidth) / 2.) < ACCURACY_LOW) {
                    tmpTwist_left = 0.;
                    tmpTwist_right = 0.;
                }
            }
            this->panelLeft.TWIST.push_back(tmpTwist_left);
            this->panelRight.TWIST.push_back(tmpTwist_right);
        }
    }
}

liftingLineWing::camberLine liftingLineWing::setPanelCamberLine(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface,
                                                                const size_t& sectionID) {
    // Declaration list
    liftingLineWing::camberLine thePanelCamberLine;
    std::vector<Point> alignedLowerAirfoil;
    std::vector<Point> camberLineResult;
    std::vector<Point> upperSectionTmp;
    std::vector<Point> lowerSectionTmp;
    upperSectionTmp = this->splitAirfoilCoordinates(anAircraftLiftingSurface.sections.at(sectionID), true);
    lowerSectionTmp = this->splitAirfoilCoordinates(anAircraftLiftingSurface.sections.at(sectionID), false);
    // Interpolate lowerSide xCoordinates to upperSide xCoordinates
    alignedLowerAirfoil = alignLeftAndRightVectorPoints(upperSectionTmp, lowerSectionTmp);
    // Camber point of section i
    for (unsigned int i(0); i < upperSectionTmp.size(); i++) {
        Point result = (alignedLowerAirfoil.at(i) + upperSectionTmp.at(i)) * 0.5;
        camberLineResult.push_back(result);
    }
    thePanelCamberLine.camberLinePoints = camberLineResult;
    return thePanelCamberLine;
}

double liftingLineWing::setPanelTwist(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const size_t& sectionID,
                                      unsigned int currentChordwisePanelID, const camberLine& thePanelCamberLine, const double& absolutePanelChord) {
    double panelTwist(NAN);
    panelTwist = convertUnit(RADIAN, DEGREE, anAircraftLiftingSurface.sections.at(sectionID).get_twist_angle());
    double zeroLift(this->getZeroLiftOfPanel(anAircraftLiftingSurface, sectionID, currentChordwisePanelID, thePanelCamberLine, absolutePanelChord));
    panelTwist -= zeroLift;
    return (-1.) * panelTwist;
}

double liftingLineWing::getZeroLiftOfPanel(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const size_t& sectionID,
        unsigned int currentChordwisePanelID, const camberLine& thePanelCamberLine, const double& absolutePanelChord) {
    double zeroLift(0.), gradTangent(0.);  // Angles in [deg]
    Point theInterpolatedCamberLineStart(0., 0., 0.), theInterpolatedCamberLineEnd(0., 0., 0.), theTangentStart(0., 0., 0.), theTangentEnd(0., 0., 0.);
    unsigned int startID(0), endID(0);
    /* Determination of Zero-Lift AoA:
       1. x-coordinates for every chordwise panel */
    theInterpolatedCamberLineStart.xCoordinate = static_cast<double>(currentChordwisePanelID) * absolutePanelChord;
    theInterpolatedCamberLineEnd.xCoordinate = static_cast<double>(currentChordwisePanelID + 1) * absolutePanelChord;
    /* 2. Normalizing to chord length of 1 */
    theInterpolatedCamberLineStart.xCoordinate /= anAircraftLiftingSurface.sections.at(sectionID).get_chord_length();
    theInterpolatedCamberLineEnd.xCoordinate /= anAircraftLiftingSurface.sections.at(sectionID).get_chord_length();
    /* 3. Calculate Zero-lift-angle for every panel row
       Note: It is the angle of a tangent which is attached the skeleton at 3/4 of its chord. Normally better suited (and thus used here) is the way to
             draw a line between 1/2 chord and the trailing edge - which is (for a parabola skeleton) parallel to the former mentioned tangent (s. Fig. 3 in LL-Manual).
    */
    theTangentStart.xCoordinate = 0.5 * (theInterpolatedCamberLineStart.xCoordinate + theInterpolatedCamberLineEnd.xCoordinate);
    theTangentEnd.xCoordinate = theInterpolatedCamberLineEnd.xCoordinate;
    while (thePanelCamberLine.camberLinePoints.at(startID).xCoordinate < theTangentStart.xCoordinate && startID < thePanelCamberLine.camberLinePoints.size() - 1) {
        startID++;
    }
    while (thePanelCamberLine.camberLinePoints.at(endID).xCoordinate < theTangentEnd.xCoordinate && endID < thePanelCamberLine.camberLinePoints.size() - 1) {
        endID++;
    }
    if (startID == endID) {
        if (endID < thePanelCamberLine.camberLinePoints.size()-1) {
            endID++;
        } else if (startID > 0) {
            startID--;
        } else {
           myRuntimeInfo->err << anAircraftLiftingSurface.sections.at(sectionID).name
                              << ".dat contains not enough coordinates to create correct liftingLine input. Abort program!" << std::endl;
           exit(1);
        }
    }
    gradTangent = (thePanelCamberLine.camberLinePoints.at(endID).zCoordinate - thePanelCamberLine.camberLinePoints.at(startID).zCoordinate) /
                  (thePanelCamberLine.camberLinePoints.at(endID).xCoordinate - thePanelCamberLine.camberLinePoints.at(startID).xCoordinate);
    zeroLift = (-1.) * convertUnit(RADIAN, DEGREE, atan(gradTangent));
    return zeroLift;
}

std::vector <Point> liftingLineWing::splitAirfoilCoordinates(const geom2::AirfoilSection& aSection, bool upperSide) {
    std::vector<Point> upperSectionTmp;
    std::vector<Point> lowerSectionTmp;
    // check where left, right, upper, and lower vertex points are
    size_t leftVertexID(0), rightVertexID(0), topVertexID(0), bottomVertexID(0);
    for (size_t i(0); i < aSection.get_contour().size(); ++i) {
        if (accuracyCheck(aSection.get_contour().container().at(i).x(), aSection.get_contour().left_vertex()->x(), ACCURACY_HIGH)) {
            leftVertexID = i;
        }
        if (accuracyCheck(aSection.get_contour().container().at(i).x(), aSection.get_contour().right_vertex()->x(), ACCURACY_HIGH)) {
            rightVertexID = i;
        }
        if (accuracyCheck(aSection.get_contour().container().at(i).y(), aSection.get_contour().top_vertex()->y(), ACCURACY_HIGH)) {
            bottomVertexID = i;
        }
        if (accuracyCheck(aSection.get_contour().container().at(i).y(), aSection.get_contour().bottom_vertex()->y(), ACCURACY_HIGH)) {
            topVertexID = i;
        }
    }
    // check if coordinate squence starts from TE or LE
    bool coordinateStartFromLE(true);
    if (leftVertexID < rightVertexID) {
        coordinateStartFromLE = true;
    } else if (leftVertexID > rightVertexID) {
        coordinateStartFromLE = false;
    } else {
        stringstream errorMessage;
        errorMessage << "Some airfoil coordinates are corruprted! - unclear where LE and TE are";
        throwError(__FILE__, __func__, __LINE__, errorMessage.str());
    }
    // check wether upper or lower section comes first in sequence
    bool upperSectionFirst(true);
    if (topVertexID < bottomVertexID) {
        upperSectionFirst = true;
    } else if (topVertexID > bottomVertexID) {
        upperSectionFirst = false;
    } else {
        stringstream errorMessage;
        errorMessage << "Some airfoil coordinates are corruprted! - unclear where upper and lower sections are";
        throwError(__FILE__, __func__, __LINE__, errorMessage.str());
    }
    // decide if points get initially pushed into upper- or lowerSectiontmp
    bool upperSection = upperSectionFirst;
    for (size_t i(0); i < aSection.get_contour().size(); ++i) {
        Point tmp_Coordinate;
        tmp_Coordinate.xCoordinate = aSection.get_contour().container().at(i).x();
        tmp_Coordinate.yCoordinate = 0.0;
        tmp_Coordinate.zCoordinate = aSection.get_contour().container().at(i).y() * (-1.0);
        // if coordinates start from LE
        if (coordinateStartFromLE) {
            if (upperSectionFirst) {
                if (upperSection) {
                    upperSectionTmp.push_back(tmp_Coordinate);
                    // check if sections do switch
                    if (accuracyCheck(tmp_Coordinate.xCoordinate, aSection.get_contour().right_vertex()->x(), ACCURACY_HIGH)) {
                        upperSection = false;
                        lowerSectionTmp.push_back(tmp_Coordinate);
                    }
                } else {
                    lowerSectionTmp.push_back(tmp_Coordinate);
                }
            // if lower section first
            } else {
                // if lower section
                if (!upperSection) {
                    lowerSectionTmp.push_back(tmp_Coordinate);
                    // check if sections do switch
                    if (accuracyCheck(tmp_Coordinate.xCoordinate, aSection.get_contour().right_vertex()->x(), ACCURACY_HIGH)) {
                        upperSection = true;
                        upperSectionTmp.push_back(tmp_Coordinate);
                    }
                // if upper section
                } else {
                    upperSectionTmp.push_back(tmp_Coordinate);
                }
            }
        // if coordinates start from TE
        } else {
            if (upperSectionFirst) {
                if (upperSection) {
                    upperSectionTmp.push_back(tmp_Coordinate);
                    // check if sections do switch
                    if (accuracyCheck(tmp_Coordinate.xCoordinate, aSection.get_contour().left_vertex()->x(), ACCURACY_HIGH)) {
                        upperSection = false;
                        lowerSectionTmp.push_back(tmp_Coordinate);
                    }
                } else {
                    lowerSectionTmp.push_back(tmp_Coordinate);
                }
            // if lower section first
            } else {
                // if lower section
                if (!upperSection) {
                    lowerSectionTmp.push_back(tmp_Coordinate);
                    // check if sections do switch
                    if (accuracyCheck(tmp_Coordinate.xCoordinate, aSection.get_contour().left_vertex()->x(), ACCURACY_HIGH)) {
                        upperSection = true;
                        upperSectionTmp.push_back(tmp_Coordinate);
                    }
                // if upper section
                } else {
                    upperSectionTmp.push_back(tmp_Coordinate);
                }
            }
        }
    }
    // push back fist coordinate, so that both vectors go fron 0 to 1
    Point lastCoordinate;
    lastCoordinate.xCoordinate = aSection.get_contour().container().front().x();
    lastCoordinate.yCoordinate = 0.0;
    lastCoordinate.zCoordinate = aSection.get_contour().container().front().y() * (-1.0);
    if (upperSectionFirst) {
        lowerSectionTmp.push_back(lastCoordinate);
    } else {
        upperSectionTmp.push_back(lastCoordinate);
    }
    // reverse one of the vectors
    if (coordinateStartFromLE && upperSectionFirst) {
        std::reverse(lowerSectionTmp.begin(), lowerSectionTmp.end());
    } else if (coordinateStartFromLE && !upperSectionFirst) {
        std::reverse(upperSectionTmp.begin(), upperSectionTmp.end());
    } else if (!coordinateStartFromLE && upperSectionFirst) {
        std::reverse(upperSectionTmp.begin(), upperSectionTmp.end());
    } else if (!coordinateStartFromLE && !upperSectionFirst) {
        std::reverse(lowerSectionTmp.begin(), lowerSectionTmp.end());
    }
    if (upperSide) {
        return upperSectionTmp;
    } else {
        return lowerSectionTmp;
    }
}

Point liftingLineWing::getTECoordinates(const Point& LEPoint, const double& chordLength, const double & LERotationInRad) {
    if (LERotationInRad > (PI / 2)) {
        stringstream errorMessage;
        errorMessage << "airfoil rotation of " << LERotationInRad << " is greater than PI/2, rotation equations invalid!";
        throwError(__FILE__, __func__, __LINE__, errorMessage.str());
    }
    Point TEPoint;
    TEPoint.xCoordinate = LEPoint.xCoordinate + chordLength * cos(LERotationInRad);
    TEPoint.yCoordinate = LEPoint.yCoordinate;
    TEPoint.zCoordinate = LEPoint.zCoordinate - chordLength * sin(LERotationInRad);
    return TEPoint;
}

Point liftingLineWing::getTrueCoordinates(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const size_t& sectionID) {
    Point TrueCoordinates;
    TrueCoordinates.xCoordinate = anAircraftLiftingSurface.origin.x() + anAircraftLiftingSurface.sections.at(sectionID).origin.x();
    TrueCoordinates.yCoordinate = anAircraftLiftingSurface.origin.y() - anAircraftLiftingSurface.sections.at(sectionID).origin.z();
    TrueCoordinates.zCoordinate = anAircraftLiftingSurface.origin.z() + anAircraftLiftingSurface.sections.at(sectionID).origin.y();
    return TrueCoordinates;
}
