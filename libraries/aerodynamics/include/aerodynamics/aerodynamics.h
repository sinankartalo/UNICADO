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

#ifndef AERODYNAMICS_AERODYNAMICS_H_
#define AERODYNAMICS_AERODYNAMICS_H_

#include <string>
#include <cstdint>
#include <vector>

#ifdef BUILD_AERODYNAMICS_SHARED
    #ifdef _WIN32
        #define AERODYNAMICSDLLEXPORT __declspec(dllexport)
    #else
        #define AERODYNAMICSDLLEXPORT __attribute__ ((visibility ("default")))
    #endif
#elif defined(IMPORT_AERODYNAMICS_SHARED)
    #ifdef _WIN32
        #define AERODYNAMICSDLLEXPORT __declspec(dllimport)
    #else
        #define AERODYNAMICSDLLEXPORT
    #endif
#else
    #define AERODYNAMICSDLLEXPORT
#endif

class atmosphere;
class configuration;
class node;

/** \brief Class to describe the aerodynamics
 */
class AERODYNAMICSDLLEXPORT aerodynamics {
 public:
    /* Typedefs and Enums */
    /** \brief Class describes the configuration of aerodynamics
     */
    class AERODYNAMICSDLLEXPORT configuration {
     public:
        /* Typedefs and Enums */
        /** \brief Class describes aerodynamic polar and coeffients
         */
        class AERODYNAMICSDLLEXPORT polar {
         public:
            /* Vectors with aerodynamic coefficients */
            std::vector<double> vAOA;
            std::vector<double> vCL;
            std::vector<double> vCD;
            std::vector<double> vCD_alternate;
            std::vector<double> vCm;
            std::vector<double> vIStab;
            std::vector<std::string> vOrigin_polar;

            uint16_t ID_firstValid;
            uint16_t ID_lastValid;

            uint16_t ID_CLmin;
            uint16_t ID_CLmax;

            polar(const std::string& aConfig, const int& machID, const node& configuration); /**< Constructor */
            virtual ~polar(); /**< Destructor */

            /* Function interpolates linearly between the data points and returns a CD */
            double getCD(const double& CL, const bool& useAlternateDragPolars, const bool& reqCheck) const;
            double getIStab(const double& CL, const bool& reqCheck) const;
            double getCL_firstValid() const;
            double getCL_lastValid() const;
            bool isValid(const double& CL) const;
            bool allowGridChange;
            double extrapolationMargin;  /* Extrapolation margin (1: no extrapolation allowed, 1.05 -> 5% extrapolation allowed, etc) */
            std::string getOriginPolar(const double& CL, const bool& useAlternateDragPolars, const bool& reqCheck) const;
        };
        /* Constructor and destructor */
        configuration(const std::string& aConfig, const double& myMMO, const node& polarXML); /**< Constructor */
        virtual ~configuration();    /**< Destructor */
        /* Member functions */
        double getCD(const double& CL, const double& machNumber, const bool& useAlternateDragPolars, const bool& reqCheck) const;
        double getIStab(const double& CL, const double& machNumber, const bool& reqCheck) const;
        double getCLAtAlpha(const double& alpha, const double& machNumber) const;
        double getAlphaAtCL(const double& CL, const double& machNumber) const;
        double getCmAtAlpha(const double& alpha, const double& machNumber) const;
        double getCmAtCL(const double& CL, const double& machNumber) const;
        double getCmAtCLmax(const double& machNumber) const;
        /** \brief Function returns the interpolated polar value for a given key point.
         * \param xValue Target x value for interpolation (can be AoA, CL, etc)
         * \param x_vec vector corresponding to target x value
         * \param y_vec vector of y values used for interpolation
         * \param extrapolationMargin extrapolation margin (1: no extrapolation allowed, 1.05 -> 5% extrapolation allowed, etc)
         * \return interpolated polar value  
         */
        double getPolarValue(const double& xValue, const std::vector<double>& x_vec, const std::vector<double>& y_vec, const double& extrapolationMargin) const;
        std::vector< std::vector<double> > getOriginPolar(const double& CL, const double& machNumber, const bool& useAlternateDragPolars, const bool& reqCheck);
        /* Member variables */
        std::string configName;
        int numberOfFlapSettings;
        std::vector<double> machnumbers; /**< Mach numbers of the polars [-] */
        std::vector<double> altitudes; /**< Altitudes of the polars [m] */
        std::vector<double> reynoldsNumbers; /**< Reynolds number of the related polar */
        std::vector<polar> polars;

     private:
        double MMO;
    };
    /* Constants */
    /* Constructors */
    aerodynamics(const node& acXML, const node& polarXML);
    /* Destructors */
    virtual ~aerodynamics();
    /* Methods */
    double getCruiseDrag(const double& machNumber, const double& altitude, const double& bankAngle, const double& m_acft, const std::string& config, const atmosphere& atm) const;
    std::vector< std::vector<double> > getCruiseOriginPolar(const double& machNumber, const double& altitude, const double& bankAngle, const double& m_acft,
                                                            const std::string& config, const atmosphere& atm);
    double getIStabPolar(const double& machNumber, const double& CL, const std::string& config) const;
    double getCruiseLoverD(const double& machNumber, const double& altitude, const double& bankAngle, const double& m_acft, const std::string& config, const atmosphere& atm) const;
    /** \brief Function to determine the resistance at a given CL. Application in the start calculation
     * \param machNumber Mach number
     * \param altitude a flight altitude [m]
     * \param CL
     * \param config configuration
     * \param atm atmosphere
     */
    double getCLDrag(const double& machNumber, const double& altitude, const double& CL, const std::string& config, const atmosphere& atm) const;
    /** \brief Function returns CLmax for configuration and Mach. If necessary, min. between two Mach numbers
     * \param machNumber
     * \param config configuration
     */
    double getCLmax(const double& machNumber, const std::string& config) const;
    /** \brief Function to determine altitude [m] at a given CL and airspeed
     * \param machNumber
     * \param bankAngle bank angle of the aircraft
     * \param CL
     * \param m_acft aircraft mass
     * \param atm atmosphere
     * \return altitude double: altitude [m] for a given lift coefficient
     */
    double getAltitudeAtCL(const double& machNumber, const double& bankAngle, const double& CL, const double& m_acft, const atmosphere& atm) const;
    /** \brief Function to determine CL
     * \param altitude a flight altitude [m]
     * \param machNumber
     * \param bankAngle bank angle of the aircraft
     * \param m_acft aircraft mass
     * \param atm atmosphere
     */
    double getCL(const double& altitude, const double& machNumber, const double& bankAngle, const double& m_acft, const atmosphere& atm) const;
    /**< Determine L/D for a CL of a specific configuration */
    double getCLLoverD(const double& machNumber, const double& CL, const std::string& config) const;
    /** \brief  Function to determine the angle of attack of the aircraft for a given flight condition
    *   \param M a Mach number
    *   \param config configuration of the secondary control devices
    *   \param liftCoefficient a lift coefficient [-]
    */
    double getAlphaAtFlightCondition(const double& machNumber, const std::string& config, const double& liftCoefficient) const;
    /** \brief Function to determine the max range CL for Rmax (CL/CD^2)max
     * \param machNumber
     * \param config: configuration
     */
    double maxRangeCL(const double& machNumber, const std::string& config) const;
    /** \brief Function to switch between turbulent and (if present) laminar polar set
     */
    void switchToAlternateDragPolars();
    /** \brief Function to switch to use of default polars
     */
    void switchToDefltDragPolars();
    bool altPolarsInUse;

    void switchOnReqSwitch();
    void switchOffReqSwitch();

    /** \brief function returns complete configuration information of the private vector configurations
     *  \param numberOfConfiguration a specific number of a configuration in the list
     *  \return an object of the class configuration with complete configuration information
    */
    configuration getConfiguration(const uint8_t& numberOfConfiguration) const;

    /* Data Members */
    std::vector<std::string> configurationList;
    std::vector<configuration> configurations;
    std::string cruiseConfiguration;
    double CLmaxTakeoff;
    double CLmaxLanding;
    double CLoptimumCruise;
    double CLgroundRoll; /**< CL during rolling */

 private:
    /* Typedefs and Enums */
    /* Constants */
    const node& acXML;
    const node& polarXML;
    /* Member variables */
    double MMO;
    double wingReferenceArea;
    std::string polarFileName;
    bool useCDalternatePolars;
    bool reqCheck;
};
#endif // AERODYNAMICS_AERODYNAMICS_H_
