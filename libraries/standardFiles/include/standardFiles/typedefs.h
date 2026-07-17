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

#ifndef STANDARDFILES_INCLUDE_STANDARDFILES_TYPEDEFS_H_
#define STANDARDFILES_INCLUDE_STANDARDFILES_TYPEDEFS_H_

#define UPPERDIRECTORY ".."
#ifdef _WIN32
    #define FILESEPERATOR "\\"
    #define FILEEXTENSION ".exe"
    #define OPERATINGSYSTEM ""
#else
    #define FILESEPERATOR "/"
    #define FILEEXTENSION ""
    #define OPERATINGSYSTEM "-linux"
#endif // _WIN32

#define GNUPLOTDEFAULTDIR UPPERDIRECTORY FILESEPERATOR "gnuplot" OPERATINGSYSTEM FILESEPERATOR "gnuplot" FILEEXTENSION
#define INKSCAPEDEFAULTDIR UPPERDIRECTORY FILESEPERATOR "inkscape" OPERATINGSYSTEM FILESEPERATOR "inkscape" FILEEXTENSION

#include <svl/SVL.h>
#include <cmath>

//Enums and Structs
/** \struct Point a struct to describe a point with a x-, y-, and z-coordinate
*   \details if a point is defined, it is constructed as a NAN-point by default until it is initialized with other values.
*            The struct allows for point summation and multiplication with a constant numeric value
*/
class Point {
 public:
    //Constructor
    explicit Point(double x = NAN, double y = NAN, double z = NAN) {
        this->xCoordinate = x;
        this->yCoordinate = y;
        this->zCoordinate = z;
    }
    explicit Point(const Vec3& aPointVector) {
        this->xCoordinate = aPointVector[0];
        this->yCoordinate = aPointVector[1];
        this->zCoordinate = aPointVector[2];
    }

    //Operators
    Point operator*(double factor) {
        Point result(factor * xCoordinate, factor * yCoordinate, factor * zCoordinate);
        return result;
    }
    Point operator/(double factor) {
        Point result(xCoordinate / factor, yCoordinate / factor, zCoordinate / factor);
        return result;
    }
    Point operator+(Point aPoint) {
        Point result(aPoint.xCoordinate + xCoordinate, aPoint.yCoordinate + yCoordinate, aPoint.zCoordinate + zCoordinate);
        return result;
    }
    Point operator-(Point aPoint) {
        Point result(aPoint.xCoordinate - xCoordinate, aPoint.yCoordinate - yCoordinate, aPoint.zCoordinate - zCoordinate);
        return result;
    }
    Point operator()(const Vec3& aPointVector) {
        return Point(aPointVector[0], aPointVector[1], aPointVector[2]);
    }

    //Member
    double xCoordinate, yCoordinate, zCoordinate;
};

enum aSide {LEFT, RIGHT, UPPER, LOWER}; /**< an enum to translate the four sides (upper/lower/left/right) into unsigned numeric values */
enum stateCondition {STATIC, DYNAMIC, TOTAL};   /**< an enum to translate the three state conditions (static/dynamic/total values) into unsigned numeric values */
enum surfaceType {FUSELAGE, SURFACE, TIP};   /**< an enum to translate the three surface types (Fuselage/Surface/Tip) into unsigned numeric values */
enum geometryComponent {WING, HTP, VTP, FSLG, NACELLE, LANDINGGEAR, PYLON}; /**< an enum to translate the aircraft geometry components */
#endif // STANDARDFILES_INCLUDE_STANDARDFILES_TYPEDEFS_H_
