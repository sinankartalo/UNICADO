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

#ifndef LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEWING_H_
#define LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEWING_H_

#include <standardFiles/typedefs.h>
#include <aircraftGeometry2/airfoil_surface.h>
#include <aircraftGeometry2/processing/measure.h>
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

/** \class liftingLineWing Class for managing liftingLineWing data.
 */
class LIFTINGLINEINTERFACEDLLEXPORT liftingLineWing {
 public:
    /** \brief Default Destructor for liftingLineWing.
     */
    virtual ~liftingLineWing() {}

 private:
    friend class liftingLineInput; /**< Definition of liftingLineInput as friend class */
    friend class liftingLineOutput; /**< Definition of liftingLineOutput as friend class */
    /** \struct liftingLinePanels Struct for managing liftingLinePanels data.
     */
    struct liftingLinePanels {
        unsigned int numberOfChordwisePanels; /**< Number of panels in chordwise direction */
        unsigned int numberOfMinimumSpanwisePanelsPerSegment; /**< Minimum number of panels in spanwise direction per segment */
        unsigned int numberOfAdditionalSpanwisePanels; /**< Number of panels in spanwise direction (in addition to wing segments) */
        unsigned int numberOfMinimumSpanwisePanelsForTipSegment; /**< Minimum number of panels in spanwise direction for tip segment */
    } theInputPanelDistribution; /**< Object for liftingLinePanels */
    /** \struct camberLine Struct for managing camberLine data.
     */
    struct camberLine {
        std::vector<Point> camberLinePoints; /**< Vector holding coordinates of camberline points, [m] */
    };

    /** \brief Default Constructor for liftingLineWing.
     */
    liftingLineWing();
    /** \brief This function generates LiLi geometry
     *  \param anAircraftLiftingSurface Object holding data of liftingSurface
     *  \param theFuselage Constant reference to fuselage geometry object holding data of aircraft fuselage
     *  \param reductionFactorHTPpanels Reduction factor for panels in spanwise direction on HTP
     *  \param untwistFslgSeg Untwist panel of fuselage segment to achieve planar calculation for this segment, [dimensionless]
     *  \return void
     */
    void generateLILIgeometry(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface,
                                const double& fuselageWidth,
                                const double& reductionFactorHTPpanels, const bool& untwistFslgSeg);
    /** \brief Method to set panel distribution.
     *  \details The member variables N_PW_Chord, N_PW_Span, numberSegments, N_P, totalPanel are initialized.
     *  \param anAircraftLiftingSurface Object holding data of liftingSurface
     *  \param reductionFactorHTPpanels Reduction factor for panels in spanwise direction on HTP
     *  \return void
     */
    void setPanelDistribution(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const double& reductionFactorHTPpanels);
    /** \brief calculation of panel coordinates in chordwise direction
     *  \details the member variables absoluteChord_PW, panelReferencePoint, and COUPLING_CONDITIONS are initialized
     *  \param anAircraftLiftingSurface Object holding data of liftingSurface
     *  \param theFuselage Constant reference to fuselage geometry object holding data of aircraft fuselage
     *  \param myScaleSpanErrors Factor for scale span erros
     *  \param untwistFslgSeg Untwist panel of fuselage segment to achieve planar calculation for this segment, [dimensionless]
     *  \return void
     */
    void setPanelProperties(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface,
                            const double& fuselageWidth, const double& myScaleSpanErrors, const bool& untwistFslgSeg);
    /** \brief Method to set panel camberline.
     *  \param anAircraftLiftingSurfaceSegment Object holding data of liftingSurfaceSegment
     *  \return camberLine struct holding {x,y,z}-coordinates, [m]
     */
    camberLine setPanelCamberLine(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const size_t& sectionID);
    /** \brief Method to calculate zero lift of each panel.
     *  \param anAircraftLiftingSurfaceSegment Object holding data of liftingSurfaceSegment
     *  \param currentChordwisePanelID Current chordwise panel ID (function is called in a loop)
     *  \param thePanelCamberLine Camberline of current panel
     *  \param absolutePanelChord Absolute chord of current panel
     *  \return zeroLift Zero-lift angle of the panel, [deg]
     */
    double getZeroLiftOfPanel(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const size_t& sectionID,
                              unsigned int currentChordwisePanelID, const camberLine& thePanelCamberLine, const double& absolutePanelChord);
    /** \brief Method to set the twist of each panel.
     *  \param anAircraftLiftingSurfaceSegment Object holding data of liftingSurfaceSegment
     *  \param currentChordwisePanelID Current chordwise panel ID (function is called in a loop)
     *  \param thePanelCamberLine Camberline of current panel
     *  \param absolutePanelChord Absolute chord of current panel
     *  \return panelTwist Twist which is defined as actual twist angle minus zero-lift angle of the panel, [deg]
     */
    double setPanelTwist(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const size_t& sectionID,
                         unsigned int currentChordwisePanelID, const camberLine& thePanelCamberLine, const double& absolutePanelChord);

    /** \brief Method to converts the geom2 Polygons to point vectors splittet in upper and lower section.
     *  \param aSection wing section wich gets splitted
     *  \param upperSide switch wether upper or lower side points are returned
     *  \return points of the upper or lower side of an airfoil
     */
    std::vector <Point> splitAirfoilCoordinates(const geom2::AirfoilSection& aSection, bool upperSide);

    /** \brief Calculates true Coordinates from true LE Coordinates, chordlenght and rotation, so basicly a geom2 workaround.
     *  \param LEPoint coordinate of the leading edge point in [m]
     *  \param chordLength local length of the chord in [m]
     *  \param LERotationInRad local twist in [rad]
     *  \return coordinates of the trailing edge point in [m]
     */
    Point getTECoordinates(const Point& LEPoint, const double& chordLength, const double & LERotationInRad);

    /** \brief Calculates true Coordinates for a LE point of the geom2 liftingsurface, so basicly a geom2 workaround. Assumes that lifing surface is extruded in y-direction!
     *  \param anAircraftLiftingSurface the lifting surface
     *  \param sectionID the index of the section
     *  \return coordinates of the leading edge point of the section at the section index in [m]
     */
    Point getTrueCoordinates(const geom2::MultisectionSurface<geom2::AirfoilSection>& anAircraftLiftingSurface, const size_t& sectionID);

    // initialized via setPanelDistribution
    unsigned int N_PW_Chord; /**< Number of panels in chordwise direction */
    unsigned int N_PW_Span; /**< Number of panels in spanwise direction */
    unsigned int totalPanel; /**< Total number of panels of a liftingSurface */
    unsigned int numberOfWingSegments; /**< Number of segments of a liftingSurface */
    std::vector<int> N_P; /**< Number of panels of a segment in spanwise direction */

    // initialized via setPanelProperties
    struct panelProperties {
        std::vector<Point> panelReferencePoint;/**< {x,y,z}-coordinates for each panel of lifting surface in chordwise direction, [m] */
        std::vector<camberLine> panelCamberLine; /**< Camberline of each panel */
        std::vector<double> absoluteChord_PW; /**< Relative chord of each panel (reference length for part wings) */
        std::vector<std::string> COUPLING_CONDITIONS; /**< Coupling condition for each panel */
        std::vector<double> TWIST; /**< Vector holding twist angles for each panel, [deg] */
    };
    panelProperties panelLeft;
    panelProperties panelRight;
};
#endif  // LIFTINGLINEINTERFACE2_INCLUDE_LIFTINGLINEINTERFACE2_LIFTINGLINEWING_H_
