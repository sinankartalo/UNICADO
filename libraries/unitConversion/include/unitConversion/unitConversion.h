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

#ifndef UNITCONVERSION_UNITCONVERSION_H_
#define UNITCONVERSION_UNITCONVERSION_H_

#include <atmosphere/atmosphere.h>
#include "unitBase.h"
#include "unitConversionConf.h"

using namespace UnitConversionConfiguration;

//!   If you make any alterations be sure to update ALL other tools as well.
//!   This works likes a no dependencies library in ALL tools.

/*
 *  Unit Conversion Library
 *  Convert available types from
 *
 *  Length | Area | Volume | Energy | Power | Speed | Air pressure dependent Speed
 *  Time | Radian/Degree | Pressure | Mass | Temperature
 *
 *  Check unitBase.h for various Unit Bases. As well as SI Unit Prefixes.
 *
 *  Use the overloaded convertUnit() function for conversion.
 *  Choose from :
 *
 *  convertUnit(unitBase from, unitBase  to, value)
 *  convertUnit(unitPrefix fromPref, unitBase from, unitBase  to, T value)
 *  convertUnit(unitBase from, unitPrefix toPref, unitBase to, T value)
 *  convertUnit(unitPrefix fromPref, unitBase from, unitPrefix toPref, unitBase to, T value)
 *  //Preferred for Area und Volume conversion but Length is also possible
 *  convertUnit(unitDimension dimension, unitPrefix fromPref, unitBase from, unitPrefix toPref, unitBase to, T value)
 *  //Preferred for air dependent speed to air independent speed and the other way round
 *  convertUnit(unitBase from, unitBase to, double FlightLevel, const atmosphere &atm, T value)
 *
 *  Examples :
 *
 *  Convert 2.5 Hours to Seconds
 *  convertUnit(HOUR, SECOND, 2.5);
 *
 *  Convert 11.1 KiloWatt to Horsepower
 *  convertUnit(KILO, WATT, HORSEPOWER, 11.1);
 *
 *  Convert 2534.1233 British Thermal Units to Mega Joule
 *  convertUnit(BRITISHTHERMALUNIT, MEGA, JOULE, 2534.1233);
 *
 *  Convert 2.3 KiloWattHours to Mega Joule
 *  convertUnit(KILO, WATTHOUR, MEGA, JOULE, 2.3);
 *
 *  Convert 13.2 Gallons to Deci Cubic Meters
 *  convertUnit(VOLUME, NOPREFIX, GALLON, DECI, METER, 13.2);
 *
 *  Convert 0.85 Mach to Knots at Flight Level 10000.
 *  convertUnit(MACH, KNOTS, 10000., atm, 0.85);
 *
 *  Written by Lukas Nolte under supervision from Florian Schueltke at ILR RWTH AACHEN.
 */


//Standard converUnit function
// Checks input type unitBase from and calls specific type function
// which returns value time conversion factor based on output unitBase to
// If unitBase from can't be handled here (like LITER) it throws a char*
// with error message
template <typename T>
T convertUnit(unitBase from, unitBase  to, T value)
{
    //Length
    if(from == FOOT || from == INCH || from == MILE || from == METER ||
       from == NAUTICALMILE)
    {
        return convertLength(from, to, value);
    }
    //Energy
    if(from == BRITISHTHERMALUNIT || from == FOOTPOUND || from == WATTHOUR
       || from == GRAMFORCEMETER || from == NEWTONMETER ||from == JOULE ||
       from == CALORIES)
    {
        return convertEnergy(from, to, value);
    }
    //Power
    if(from == HORSEPOWER || from == WATT || from == FOOTPOUNDFORCEPERSECOND)
    {
        return convertPower(from, to, value);
    }
    //Speed
    if(to == KNOTS || to == METERPERSECOND || to == KILOMETERPERHOUR || to == FOOTPERMINUTE)
    {
        return convertSpeed(from, to , value);
    }
    //Time
    if(from == SECOND|| from == MINUTE || from == HOUR)
    {
        return convertTime(from, to, value);
    }
    //Radian/Degree
    if(from == RADIAN || from == DEGREE)
    {
        return convertRadianDegree(from, to, value);
    }
    //Pressure
    if(from == PASCAL || from == BAR || from == ATMOSPHERE || from == PSI)
    {
        return convertPressure(from, to, value);
    }
    //Mass
    if(from == GRAM || from == POUND)
    {
        return convertMass(from, to, value);
    }
    //Force
    if(from == NEWTON || from == DYN || from == KILOPOND || from == POUNDFORCE || from == POUNDAL)
    {
        return convertForce(from, to, value);
    }
    //Temperature
    if(from == FAHRENHEIT || from == KELVIN || from == CELCIUS)
    {
        return convertTemperature(from, to, value);
    }
    //From Unit is not known
    throw "Wrong Input Unit in function convertUnit. Check enum unitBase for types.";
}

//convertUnit function with prefixes
// calls standard convertUnit() and calculates return value with prefixes
template <typename T>
T convertUnit(unitPrefix fromPref, unitBase from, unitBase  to, T value)
{
    return convertUnit(from, to, value) * getPrefix(fromPref);
}
template <typename T>
T convertUnit(unitBase from, unitPrefix toPref, unitBase to, T value)
{
    return convertUnit(from, to, value) / getPrefix(toPref);
}
template <typename T>
T convertUnit(unitPrefix fromPref, unitBase from, unitPrefix toPref, unitBase to, T value)
{
    return convertUnit(from, to, value) * getPrefix(fromPref) / getPrefix(toPref);
}

//Special convertUnit function to handle Volume and Area conversion
template <typename T>
T convertUnit(unitDimension dimension, unitPrefix fromPref, unitBase from, unitPrefix toPref, unitBase to, T value)
{
    switch (dimension)
    {
        //Length is handled with convertUnit function with prefixes
        case LENGTH : return convertUnit(fromPref, from, toPref, to, value); break;
        //Area is calculated by taking the conversion factor and the prefixes square multiplied by the value
        case AREA : return pow(convertUnit(from, to, 1.),2)* value * pow(getPrefix(fromPref),2) / pow(getPrefix(toPref),2); break;
        //In the case of Volume there are irregular types (Liter, Gallon) where the prefix is not taken to the power of three
        case VOLUME :
        {
                T volumeInMETER;
                //Are there irregular types to be converted?
                //Or are from and to unit bases of length?
                bool LengthIn = true, LengthOut = true;
                if(from == GALLON || from == LITER)
                {
                    LengthIn = false;
                }
                if(to == GALLON || to == LITER)
                {
                    LengthOut = false;
                }
                //If both are irregular there can just be converted
                if(!LengthIn && !LengthOut)
                {
                    return convertVolume(from, to, value) * getPrefix(fromPref) / getPrefix(toPref);
                }
                //If there is an irregular input unit base and an unit base of length output
                //this is converted in cubic meter and then calls convertUnit() recursively
                //with unit base of length inputs and outputs
                if(!LengthIn && LengthOut)
                {
                    volumeInMETER = convertVolume(from, CUBICMETER, value) * getPrefix(fromPref);
                    return convertUnit(VOLUME, NOPREFIX, METER, toPref, to, volumeInMETER);
                }
                //If there is a regular input unit base of length and an irregular unit base output
                //this is converted to cubic meter and then calls convertUnit() recursively
                //with both irregular types in input and output
                if(LengthIn && !LengthOut)
                {
                    volumeInMETER = convertUnit(VOLUME, fromPref, from, NOPREFIX, METER, value);
                    return convertVolume(CUBICMETER, to, volumeInMETER) / getPrefix(toPref);
                }
                //If both input and output are unit bases of length there are calculated like area
                //with the difference that the factors are taken to the power of 3
                if(LengthIn && LengthOut)
                {
                    return pow(convertUnit(from, to, 1.),3)* value  * pow(getPrefix(fromPref),3) / pow(getPrefix(toPref),3);
                }
        } break;
        default : throw "Wrong Dimension in function convertUnit. Check enum unitDimension for types.";
    }
    throw "Wrong Dimension in function convertUnit. Check enum unitDimension for types.";
}
//Overloded function which is only for Speed
template <typename T>
T convertUnit(unitBase from, unitBase to, double FlightLevel, const atmosphere &atm, T value)
{
    T speedInMeterPerSecond;
    //Do you need to know the "air"(flight level/temperature/pressure) to calculate the speed?
    bool airDependentIn = true, airDependentOut = true;
    if(from == KNOTS || from == METERPERSECOND || from == KILOMETERPERHOUR || from == FOOTPERMINUTE)
    {
        airDependentIn = false;
    }
    if(to == KNOTS || to == METERPERSECOND || to == KILOMETERPERHOUR || to == FOOTPERMINUTE)
    {
        airDependentOut = false;
    }
    //If both types are air independent it is a simple factor calculation
    if(!airDependentIn && !airDependentOut)
    {
        return convertSpeed(from, to, value);
    }
    //Extra If to break the function running indefinitely
    //Meter per second to Mach can easily be converted if you know the speed of sound
        if(from == METERPERSECOND && to == MACH)
        {
            return value / atm.getSpeedOfSound(FlightLevel);
        }
        if(from == MACH && to == METERPERSECOND)
        {
            return value * atm.getSpeedOfSound(FlightLevel);
        }
    //If there is an air independent input type it is converted to meter per second
    //Then there is a recursive conversion between meter per second and Mach
    //Then Mach is converted to the air dependent output type
    if(!airDependentIn && airDependentOut)
    {
        speedInMeterPerSecond = convertSpeed(from, METERPERSECOND, value);
        speedInMeterPerSecond = convertUnit(METERPERSECOND, MACH, FlightLevel, atm, speedInMeterPerSecond);
        return convertUnit(MACH, to, FlightLevel, atm, speedInMeterPerSecond);
    }
    //If there is an air dependent input type it is converted to Mach
    //Then there is a recursive conversion between Mach and meter per second
    //Then meter per second is converted to the air independent output type
    if(airDependentIn && !airDependentOut)
    {
        speedInMeterPerSecond = convertUnit(from, MACH, FlightLevel, atm, value);
        speedInMeterPerSecond = convertUnit(MACH, METERPERSECOND, FlightLevel, atm, speedInMeterPerSecond);
        return convertSpeed(METERPERSECOND, to, speedInMeterPerSecond);
    }
    //Both types depend on FlightLevel
    //Switch case construct to call the specific conversion functions in unitConversionConf.h
    if(airDependentIn && airDependentOut)
    {
        switch (from)
        {
        case MACH :
            {
                switch (to)
                {
                case MACH : return value; break;
                case TRUEAIRSPEED : return mach2tas(value, FlightLevel, atm); break;
                case CALIBRATEDAIRSPEED : return mach2cas(value, FlightLevel, atm); break;
                default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
                }
            }
        case TRUEAIRSPEED :
            {
                switch (to)
                {
                case MACH : return tas2mach(value, FlightLevel, atm); break;
                case TRUEAIRSPEED : return value; break;
                case CALIBRATEDAIRSPEED : return tas2cas(value, FlightLevel, atm); break;
                default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
                }
            }
        case CALIBRATEDAIRSPEED :
            {
                switch (to)
                {
                case MACH : return cas2mach(value, FlightLevel, atm); break;
                case TRUEAIRSPEED : return cas2tas(value, FlightLevel, atm); break;
                case CALIBRATEDAIRSPEED : return value; break;
                default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
                }
            }
        default : throw "Wrong Output Unit in function convertUnit. Check enum unitBase for types.";
        }
    }
    throw "Wrong Input Unit in function convertUnit. Check enum unitBase for types.";
}
#endif // UNITCONVERSION_UNITCONVERSION_H_
