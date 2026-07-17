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

#ifndef UNITCONVERSION_UNITBASE_H_
#define UNITCONVERSION_UNITBASE_H_

//Pascal
//The SI-Unit Pascal has to be seperated of the keyword _pascal in C++
#ifdef PASCAL
    #undef PASCAL
#endif

enum unitBase
{
    //Length
    FOOT,
    INCH,
    MILE,
    METER,
    NAUTICALMILE,

    //Volume
    GALLON,
    LITER,

    //Energy
    BRITISHTHERMALUNIT,
    FOOTPOUND,
    WATTHOUR,
    GRAMFORCEMETER,
    NEWTONMETER,
    JOULE,
    CALORIES,

    //Power
    HORSEPOWER,
    WATT,
    FOOTPOUNDFORCEPERSECOND,

    //Speed
    KNOTS,
    METERPERSECOND,
    KILOMETERPERHOUR,
    FOOTPERMINUTE,
    //Air dependend
    MACH,
    TRUEAIRSPEED,
    CALIBRATEDAIRSPEED,

    //Time
    SECOND,
    MINUTE,
    HOUR,

    //Radian/Degree
    RADIAN,
    DEGREE,

    //Pressure
    PASCAL,
    BAR,
    ATMOSPHERE,
    PSI,

    //Mass
    GRAM,
    POUND,

    //Force
    NEWTON,
    DYN,
    KILOPOND,
    POUNDFORCE,
    POUNDAL,

    //Temperature
    FAHRENHEIT,
    KELVIN,
    CELCIUS,

    //Internally Used Types
    //!DO NOT USE
    CUBICMETER
};

enum unitPrefix
{
    YOTTA,
    ZETTA,
    EXA,
    PETA,
    TERA,
    GIGA,
    MEGA,
    KILO,
    HECTO,
    DECA,
    NOPREFIX,
    DECI,
    CENTI,
    MILLI,
    MICRO,
    NANO,
    PIKO,
    FEMTO,
    ATTO,
    ZEPTO,
    YOKTO
};

enum unitDimension
{
    LENGTH,
    AREA,
    VOLUME
};

#endif // UNITCONVERSION_UNITBASE_H_
