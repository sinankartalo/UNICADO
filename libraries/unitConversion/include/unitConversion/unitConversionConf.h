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

#ifndef UNITCONVERSION_UNITCONVERSIONCONF_H_
#define UNITCONVERSION_UNITCONVERSIONCONF_H_

#include <atmosphere/atmosphere.h>
#include <cmath>
#include "unitBase.h"

//!   If you make any alterations be sure to update ALL other tools as well.
//!   This works likes a no dependencies library in ALL tools.

//own namespace to not pollute the namespace as well as
// unpredictable behavior if these functions are used without the
// necessary safety checks performed by convertUnit()
namespace UnitConversionConfiguration
{

//!!!!!!!!!LENGTH!!!!!!!!!!!!!
template <typename T>
T convertFoot(unitBase to, T value)
{
    switch (to)
    {
    case FOOT : return value; break;
    case INCH : return value * 12.; break;
    case MILE : return value * 0.000189393939; break;
    case METER : return value * 0.3048; break;
    case NAUTICALMILE : return value * 0.000164578834; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertInch(unitBase to, T value)
{
    switch (to)
    {
    case FOOT : return value * 0.0833333333; break;
    case INCH : return value; break;
    case MILE : return value * 1.57828283 * pow(10, -5); break;
    case METER : return value * 0.0254; break;
    case NAUTICALMILE : return value * 1.37149028 * pow(10,-5); break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertMile(unitBase to, T value)
{
    switch (to)
    {
    case FOOT : return value * 5280.; break;
    case INCH : return value * 63360.; break;
    case MILE : return value; break;
    case METER : return value * 1609.344; break;
    case NAUTICALMILE : return value * 0.868976242; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertMeter(unitBase to, T value)
{
    switch (to)
    {
    case FOOT : return value * 3.2808399; break;
    case INCH : return value * 39.3700787; break;
    case MILE : return value * 0.000621371192; break;
    case METER : return value; break;
    case NAUTICALMILE : return value * 0.000539956803; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertNauticalMile(unitBase to, T value)
{
    switch (to)
    {
    case FOOT : return value * 6076.11549; break;
    case INCH : return value * 72913.3858; break;
    case MILE : return value * 1.15077945; break;
    case METER : return value * 1852.; break;
    case NAUTICALMILE : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertLength function handles all length types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertLength(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case FOOT : return convertFoot(to, value); break;
    case INCH : return convertInch(to, value); break;
    case MILE : return convertMile(to, value); break;
    case METER : return convertMeter(to, value); break;
    case NAUTICALMILE : return convertNauticalMile(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!!VOLUME!!!!!!!!!!!!!
template <typename T>
T convertGallon(unitBase to, T value)
{
    switch (to)
    {
    case GALLON : return value; break;
    case CUBICMETER : return value * 0.00378541178; break;
    case LITER : return value * 3.78541178; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertCubicMeter(unitBase to, T value)
{
    switch (to)
    {
    case GALLON : return value * 264.172052; break;
    case CUBICMETER : return value; break;
    case LITER : return value * 1000.; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertLiter(unitBase to, T value)
{
    switch (to)
    {
    case GALLON : return value * 0.264172052; break;
    case CUBICMETER : return value * 0.001; break;
    case LITER : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertVolume function handles some Volume types.
//Cubic length types are handled in convertUnit(VOLUME,..)
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertVolume(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case GALLON : return convertGallon(to, value); break;
    case CUBICMETER : return convertCubicMeter(to, value); break;
    case LITER : return convertLiter(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!!!ENERGY!!!!!!!!!!!!!!!
template <typename T>
T convertBritishThermalUnit(unitBase to, T value)
{
    switch (to)
    {
    case BRITISHTHERMALUNIT : return value; break;
    case FOOTPOUND : return value * 778.169262; break;
    case WATTHOUR : return value * 0.29307107; break;
    case GRAMFORCEMETER : return value * 107585.756; break;
    case NEWTONMETER : return value * 1055.05585; break;
    case JOULE : return value * 1055.05585; break;
    case CALORIES : return value * 252.164401; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertFootPound(unitBase to, T value)
{
    switch (to)
    {
    case BRITISHTHERMALUNIT : return value * 0.00128506746; break;
    case FOOTPOUND : return value; break;
    case WATTHOUR : return value * 0.000376616097; break;
    case GRAMFORCEMETER : return value * 138.254954; break;
    case NEWTONMETER : return value * 1.35581795; break;
    case JOULE : return value * 1.35581795; break;
    case CALORIES : return value * 0.324048267; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertWattHour(unitBase to, T value)
{
    switch (to)
    {
    case BRITISHTHERMALUNIT : return value * 3.41214163; break;
    case FOOTPOUND : return value * 2655.22374; break;
    case WATTHOUR : return value; break;
    case GRAMFORCEMETER : return value * 367097.837; break;
    case NEWTONMETER : return value * 3600.; break;
    case JOULE : return value * 3600.; break;
    case CALORIES : return value * 860.42065; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertGramForceMeter(unitBase to, T value)
{
    switch (to)
    {
    case BRITISHTHERMALUNIT : return value * 9.29491076 * pow(10,-6); break;
    case FOOTPOUND : return value * 0.00723301385; break;
    case WATTHOUR : return value * 2.72406944 * pow(10,-6) ; break;
    case GRAMFORCEMETER : return value * 107.585756; break;
    case NEWTONMETER : return value * 0.00980665; break;
    case JOULE : return value * 0.00980665; break;
    case CALORIES : return value * 0.0023438456; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertNewtonMeter(unitBase to, T value)
{
    switch (to)
    {
    case BRITISHTHERMALUNIT : return value * 0.239005736; break;
    case FOOTPOUND : return value * 0.737562149; break;
    case WATTHOUR : return value * 0.000277777778; break;
    case GRAMFORCEMETER : return value * 101.971621; break;
    case NEWTONMETER : return value; break;
    case JOULE : return value; break;
    case CALORIES : return value * 0.239005736; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertJoule(unitBase to, T value)
{
    switch (to)
    {
    case BRITISHTHERMALUNIT : return value * 0.239005736; break;
    case FOOTPOUND : return value * 0.737562149; break;
    case WATTHOUR : return value * 0.000277777778; break;
    case GRAMFORCEMETER : return value * 101.971621; break;
    case NEWTONMETER : return value; break;
    case JOULE : return value; break;
    case CALORIES : return value * 0.239005736; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertCalories(unitBase to, T value)
{
    switch (to)
    {
    case BRITISHTHERMALUNIT : return value * 0.00396566683; break;
    case FOOTPOUND : return value * 3.08596003; break;
    case WATTHOUR : return value * 0.00116222222; break;
    case GRAMFORCEMETER : return value * 426.649264; break;
    case NEWTONMETER : return value * 4.18400; break;
    case JOULE : return value * 4.18400; break;
    case CALORIES : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertEnergy function handles all energy types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertEnergy(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case BRITISHTHERMALUNIT : return convertBritishThermalUnit(to, value); break;
    case FOOTPOUND : return convertFootPound(to, value); break;
    case WATTHOUR : return convertWattHour(to, value); break;
    case GRAMFORCEMETER : return convertGramForceMeter(to, value); break;
    case NEWTONMETER : return convertNewtonMeter(to, value); break;
    case JOULE : return convertJoule(to, value); break;
    case CALORIES : return convertCalories(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}
//!!!!!!!!!!!!!!!POWER!!!!!!!!!!!!!!!!!
template <typename T>
T convertHorsePower(unitBase to, T value)
{
    switch (to)
    {
    case HORSEPOWER : return value; break;
    case WATT : return value * 745.699872; break;
    case FOOTPOUNDFORCEPERSECOND : return value * 550.; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertWatt(unitBase to, T value)
{
    switch (to)
    {
    case HORSEPOWER : return value * 0.00134102209; break;
    case WATT : return value; break;
    case FOOTPOUNDFORCEPERSECOND : return value * 0.737562149; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertFootPoundForcePerSecond(unitBase to, T value)
{
    switch (to)
    {
    case HORSEPOWER : return value * 0.001818182; break;
    case WATT : return value * 1.355817948; break;
    case FOOTPOUNDFORCEPERSECOND : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertPower function handles all power types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertPower(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case HORSEPOWER : return convertHorsePower(to, value); break;
    case WATT : return convertWatt(to, value); break;
    case FOOTPOUNDFORCEPERSECOND : return convertFootPoundForcePerSecond(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!!!!SPEED!!!!!!!!!!!!!!!!!!
//functions with name tas2cas/cas2tas/tas2mach/mach2tas/cas2mach/mach2tas
// are take from previous mission analysis project at revision 3902
// air dependent calculations are handled in them
template <typename T>
T tas2cas(T TAS, double FL, const atmosphere &atm)
{
    double mu((1.4-1)/1.4);
    const double &p_0 = atm.p_0;
    const double &r_0 = atm.rho_0;
    double p = atm.getPressure(FL);
    double r = atm.getDensity(FL);

    return sqrt(2.*p_0/mu/r_0*(pow((1.+p/p_0*(pow((1+mu/2*r/p*TAS*TAS),1./mu)-1.)),mu)-1.));
}
template <typename T>
T cas2tas(T CAS, double FL, const atmosphere &atm)
{
    double mu((1.4-1)/1.4);
    const double &p_0 = atm.p_0;
    const double &r_0 = atm.rho_0;
    double p = atm.getPressure(FL);
    double r = atm.getDensity(FL);

    return sqrt(2.*p/mu/r*(pow((1.+p_0/p*(pow((1+mu/2*r_0/p_0*CAS*CAS),1./mu)-1.)),mu)-1.));
}
template <typename T>
T tas2mach(T TAS, double FL, const atmosphere &atm)
{
    return TAS/atm.getSpeedOfSound(FL);
}
template <typename T>
T mach2tas(T M, double FL, const atmosphere &atm)
{
    return M*atm.getSpeedOfSound(FL);
}
template <typename T>
T cas2mach(T CAS, double FL, const atmosphere &atm)
{
    double mu((1.4-1)/1.4);
    const double &p_0 = atm.p_0;
    const double &r_0 = atm.rho_0;
    double p = atm.getPressure(FL);
    double r = atm.getDensity(FL);

    return sqrt(2.*p/mu/r*(pow((1.+p_0/p*(pow((1+mu/2*r_0/p_0*CAS*CAS),1./mu)-1.)),mu)-1.))/atm.getSpeedOfSound(FL);
}
template <typename T>
T mach2cas(T M, double FL, const atmosphere &atm)
{
    double mu((1.4-1)/1.4);
    const double &p_0 = atm.p_0;
    const double &r_0 = atm.rho_0;
    double p = atm.getPressure(FL);
    double r = atm.getDensity(FL);
    double a = atm.getSpeedOfSound(FL);

    return sqrt(2.*p_0/mu/r_0*(pow((1.+p/p_0*(pow((1+mu/2*r/p*M*M*a*a),1./mu)-1.)),mu)-1.));
}
template <typename T>
T convertKnots(unitBase to, T value)
{
    switch (to)
    {
    case KNOTS : return value; break;
    case METERPERSECOND : return value * 0.514444444; break;
    case KILOMETERPERHOUR : return value * 1.85200; break;
    case FOOTPERMINUTE : return value * 101.2677165354331; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertMeterPerSecond(unitBase to, T value)
{
    switch (to)
    {
    case KNOTS : return value * 1.94384449; break;
    case METERPERSECOND : return value; break;
    case KILOMETERPERHOUR : return value * 3.6; break;
    case FOOTPERMINUTE : return value * 196.8503937; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertKiloMeterPerHour(unitBase to, T value)
{
    switch (to)
    {
    case KNOTS : return value * 0.539956803; break;
    case METERPERSECOND : return value * 0.277777778; break;
    case KILOMETERPERHOUR : return value; break;
    case FOOTPERMINUTE : return value * 54.68064; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertFootPerMinute(unitBase to, T value)
{
    switch (to)
    {
    case KNOTS : return value * 0.009874815333; break;
    case METERPERSECOND : return value * 0.00508; break;
    case KILOMETERPERHOUR : return value * 0.018288; break;
    case FOOTPERMINUTE : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertSpeed function handles air independent speed types.
//conversion of air dependent to air independent speed is handled
// in convertUnit(...,...,FlightLevel,atm,...)
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertSpeed(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case KNOTS : return convertKnots(to, value); break;
    case METERPERSECOND : return convertMeterPerSecond(to, value); break;
    case KILOMETERPERHOUR : return convertKiloMeterPerHour(to, value); break;
    case FOOTPERMINUTE : return convertFootPerMinute(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!TIME!!!!!!!!!!!!!!!!!!!!!
template <typename T>
T convertSecond(unitBase to, T value)
{
    switch (to)
    {
    case SECOND : return value; break;
    case MINUTE : return value * 0.0166666667; break;
    case HOUR : return value * 0.000277777778; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertMinute(unitBase to, T value)
{
    switch (to)
    {
    case SECOND : return value * 60.; break;
    case MINUTE : return value; break;
    case HOUR : return value * 0.0166666667; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertHour(unitBase to, T value)
{
    switch (to)
    {
    case SECOND : return value * 3600.; break;
    case MINUTE : return value * 60.; break;
    case HOUR : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertTime function handles all time types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertTime(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case SECOND : return convertSecond(to, value); break;
    case MINUTE : return convertMinute(to, value); break;
    case HOUR : return convertHour(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!!!!!!!!RADIAN/DEGREE!!!!!!!!!!!!!!!!!!!!!
template <typename T>
T convertRadian(unitBase to, T value)
{
    switch (to)
    {
    case RADIAN : return value; break;
    case DEGREE : return value * 57.2957795; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertDegree(unitBase to, T value)
{
    switch (to)
    {
    case RADIAN : return value * 0.0174532925; break;
    case DEGREE : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertRadianDegree function handles all Radian or Degree types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertRadianDegree(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case RADIAN : return convertRadian(to, value); break;
    case DEGREE : return convertDegree(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!!!!!!!PRESSURE!!!!!!!!!!!!!!!!!!!!!!!
template <typename T>
T convertPascal(unitBase to, T value)
{
    switch (to)
    {
    case PASCAL : return value; break;
    case BAR : return value * 1.0 * pow(10,-5); break;
    case ATMOSPHERE : return value * 9.86923267 * pow(10,-6); break;
    case PSI : return value * 0.000145037738; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertBar(unitBase to, T value)
{
    switch (to)
    {
    case PASCAL : return value * 100000.; break;
    case BAR : return value; break;
    case ATMOSPHERE : return value * 0.986923267; break;
    case PSI : return value * 14.5037738; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertAtmosphere(unitBase to, T value)
{
    switch (to)
    {
    case PASCAL : return value * 101325.; break;
    case BAR : return value * 1.01325; break;
    case ATMOSPHERE : return value; break;
    case PSI : return value * 14.6959488; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertPsi(unitBase to, T value)
{
    switch (to)
    {
    case PASCAL : return value * 6894.75729; break;
    case BAR : return value * 0.0689475729; break;
    case ATMOSPHERE : return value * 0.0680459639; break;
    case PSI : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertPressure function handles all pressure types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertPressure(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case PASCAL : return convertPascal(to, value); break;
    case BAR : return convertBar(to, value); break;
    case ATMOSPHERE : return convertAtmosphere(to, value); break;
    case PSI : return convertPsi(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!!!!!!!MASS!!!!!!!!!!!!!!!!!!!!!!
template <typename T>
T convertGram(unitBase to, T value)
{
    switch (to)
    {
    case GRAM : return value; break;
    case POUND : return value * 0.00220462262; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertPound(unitBase to, T value)
{
    switch (to)
    {
    case GRAM : return value * 453.59237; break;
    case POUND : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertMass function handles all mass types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertMass(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case GRAM : return convertGram(to, value); break;
    case POUND : return convertPound(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}
//!!!!!!!!!!!!!!!!!FORCE!!!!!!!!!!!!!!!!!!!!!!
template <typename T>
T convertNewton(unitBase to, T value)
{
    switch (to)
    {
    case NEWTON : return value; break;
    case DYN : return value * 1.0 * pow(10,5.); break;
    case KILOPOND : return value * 0.10197162129779282425700927431896; break;
    case POUNDFORCE : return value * 0.22480894309971048291003941340318; break;
    case POUNDAL : return value * 7.2330138512098943806749934824484; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertDyn(unitBase to, T value)
{
    switch (to)
    {
    case NEWTON : return value * 1.0 * pow(10,-5.); break;
    case DYN : return value; break;
    case KILOPOND : return value * 0.10197162129779282425700927431896 * pow(10,-5.); break;
    case POUNDFORCE : return value * 0.22480894309971048291003941340318  * pow(10,-5.); break;
    case POUNDAL : return value * 7.2330138512098943806749934824484 * pow(10,-5.); break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertKilopond(unitBase to, T value)
{
    switch (to)
    {
    case NEWTON : return value * 9.80665; break;
    case DYN : return value * 9.80665 * pow(10,5.); break;
    case KILOPOND : return value; break;
    case POUNDFORCE : return value * 2.2046226218487758072297380134503; break;
    case POUNDAL : return value * 70.931635283967510728246424834653; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertPoundforce(unitBase to, T value)
{
    switch (to)
    {
    case NEWTON : return value * 4.4482216152605; break;
    case DYN : return value  * 4.4482216152605 * pow(10,5.); break;
    case KILOPOND : return value * 0.45359237; break;
    case POUNDFORCE : return value; break;
    case POUNDAL : return value * 32.174048556430446194225721784777; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertPoundal(unitBase to, T value)
{
    switch (to)
    {
    case NEWTON : return value * 0.138254954376; break;
    case DYN : return value  * 0.138254954376 * pow(10,5.); break;
    case KILOPOND : return value * 0.01409808185017309682715300331918; break;
    case POUNDFORCE : return value * 1.0 / 32.174048556430446194225721784777; break;
    case POUNDAL : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertForce function handles all force types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertForce(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case NEWTON : return convertNewton(to, value); break;
    case DYN : return convertDyn(to, value); break;
    case KILOPOND : return convertKilopond(to, value); break;
    case POUNDFORCE : return convertPoundforce(to, value); break;
    case POUNDAL : return convertPoundal(to, value); break;
    default : throw "Wrong Dimension in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!!!!!!!TEMPERATURE!!!!!!!!!!!!!!!!!
// Conversion equations taken from "en.wikipedia.org/wiki/Conversion_of_units_of_temperature"
template <typename T>
T convertFahrenheit(unitBase to, T value)
{
    switch (to)
    {
    case FAHRENHEIT : return value; break;
    case KELVIN : return (value + 459.67) * 5 / 9; break;
    case CELCIUS : return (value - 32) * 5 / 9; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertKelvin(unitBase to, T value)
{
    switch (to)
    {
    case FAHRENHEIT : return value * 1.8 - 459.67; break;
    case KELVIN : return value; break;
    case CELCIUS : return value - 273.15; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
template <typename T>
T convertCelcius(unitBase to, T value)
{
    switch (to)
    {
    case FAHRENHEIT : return value * 1.8 + 32; break;
    case KELVIN : return value + 273.15; break;
    case CELCIUS : return value; break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}
//convertTemperature function handles all temperature types.
//Makes sure input base to output base uses right value.
//Calls specific input base function.
template <typename T>
T convertTemperature(unitBase from, unitBase to, T value)
{
    switch (from)
    {
    case FAHRENHEIT : return convertFahrenheit(to, value); break;
    case KELVIN : return convertKelvin(to, value); break;
    case CELCIUS : return convertCelcius(to, value); break;
    default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
    }
}

//!!!!!!!!!!!!!!PREFIX!!!!!!!!!!!!!!!!!!!!!!
//Standard SI unit prefix operator
inline long double getPrefix(unitPrefix pref)
{
    switch (pref)
    {
        case YOTTA : return pow(10,24); break;
        case ZETTA : return pow(10,21); break;
        case EXA : return pow(10,18); break;
        case PETA : return pow(10,15); break;
        case TERA : return pow(10,12); break;
        case GIGA : return pow(10,9); break;
        case MEGA : return pow(10,6); break;
        case KILO : return pow(10,3); break;
        case HECTO : return pow(10,2); break;
        case DECA : return pow(10,1); break;
        case NOPREFIX : return pow(10,0); break;
        case DECI : return pow(10,-1); break;
        case CENTI : return pow(10,-2); break;
        case MILLI : return pow(10,-3); break;
        case MICRO : return pow(10,-6); break;
        case NANO : return pow(10,-9); break;
        case PIKO : return pow(10,-12); break;
        case FEMTO : return pow(10,-15); break;
        case ATTO : return pow(10,-18); break;
        case ZEPTO : return pow(10,-21); break;
        case YOKTO : return pow(10,-24); break;
        default : throw "Wrong Prefix in function convertUnit. Check enum unitPrefix for types.";
    }
}

}
#endif // UNITCONVERSION_UNITCONVERSIONCONF_H_
