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

#ifndef STANDARDFILES_INCLUDE_STANDARDFILES_FUNCTIONS_H_
#define STANDARDFILES_INCLUDE_STANDARDFILES_FUNCTIONS_H_

#include <runtimeInfo/runtimeInfo.h>
#include <unitConversion/constants.h>
#include <Eigen/Dense>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include "typedefs.h"

#ifdef BUILD_STANDARDFILES_SHARED
#ifdef _WIN32
#define STANDARDFILESDLLEXPORT __declspec(dllexport)
#else
#define STANDARDFILESDLLEXPORT __attribute__((visibility("default")))
#endif
#elif defined(IMPORT_STANDARDFILES_SHARED)
#ifdef _WIN32
#define STANDARDFILESDLLEXPORT __declspec(dllimport)
#else
#define STANDARDFILESDLLEXPORT
#endif
#else
#define STANDARDFILESDLLEXPORT
#endif

/** \brief Execution of child processes with return of exitValues. If not other specified, standard max Runtime is 10min.
 *   \param processName: name of the child process
 *   \param relExecDir: directory of the child process as relative path
 *   \param maxRuntime: maximum runtime in minutes
 */
STANDARDFILESDLLEXPORT int handleChildProcess(const std::string& processName, const std::string& relExecDir, double maxRuntime = 10.);

/** \brief Execution of child processes in aonother directory with return of exitValues. If not other specified, standard max Runtime is 10min.
 *   \param processName: name of the child process
 *   \param relExecDir: directory of the child process as relative path
 *   \param workingDir: current working directory
 *   \param maxRuntime: maximum runtime in minutes
 *   \return exit code
 */
STANDARDFILESDLLEXPORT int handleChildProcessOtherDirectory(const std::string& processName, const std::string& relExecDir, const std::string& workingDir, double maxRuntime = 10.);

/** \brief Deletion of obsolete files
 *   \param filename: path to the file to delete
 *   \return exit code
 */
STANDARDFILESDLLEXPORT void deleteObsoleteFiles(const std::string& filename);

/** \brief Deletion of multiple obsolete files with the same file ending or same name
 *   \param filesPath: path to the files to delete
 *   \param filesDir: directory of the files
 */
STANDARDFILESDLLEXPORT void wildCardDeleteFiles(const std::string& filesPath, const std::string& filesDir);

/** \brief Lists all files with a given names in a directory
 *   \param filesDir: directory that contains the files
 *   \param fileName: name of the files
 *   \return listDirFiles: list of the files in the directory as std::string<vector>
 */
STANDARDFILESDLLEXPORT std::vector<std::string> listDirFiles(const std::string& filesDir, const std::string& fileName = "*");

/** \brief Function to rename or to relocate files
 *   \param oldname: old name of the file
 *   \param newname: new name of the file
 *   \param newfiletype: new file type (default value: "" to keep the same filetype)
 *   \param backup: determines whether to keep a backup of the original file (default value: false)
 */
STANDARDFILESDLLEXPORT void renameFiles(const std::string& oldname, const std::string& newname, const std::string& newfiletype = "", const bool& backup = false);

/** \brief Function to copy files or directories. Wildcards are allowed. Multiple files/folders need to be spearated with \0.
 *   \param from: old file path(s)
 *   \param to: new file path(s)
 *   \param renameOnCollision: If a file with the same name is already in the "to" directory, this will prevent the original file to be overwritten.
 */
STANDARDFILESDLLEXPORT bool copyFiles(const std::string& from, const std::string& to, const bool overwriteExistingFile = true);

/** \brief Function determines existence of a file
 *   \param filename: path to the file
 *   \return false: if file does not exist; true: if file exists
 */
STANDARDFILESDLLEXPORT bool fileExists(const std::string& filename);

/** \brief Function returns the absokute path of a file or a directory
 *   \param filename: relative path to the file
 *   \return full path of the file/directory as std::string
 */
STANDARDFILESDLLEXPORT std::string getFullPathString(const std::string& filename);

/** \brief Function to read a text file
 *   \param filename: path to the file
 *   \param specialCase: if special case is true, the end-line char is deleted from read line (sometimes important for linux)
 *   \return file content separated by lines as vector<std::string>
 */
STANDARDFILESDLLEXPORT std::vector<std::string> readFile(const std::string& filename, const bool& specialCase = false);

/** \brief Function to find a word inside a file
 *   \param filename: path to the file
 *   \param searchWord: search word
 *   \param specialCase: if special case is true, the readFile function is deleting the end-line char from all lines
 *   \return number of the line that contains the file
 */
STANDARDFILESDLLEXPORT size_t findInFile(const std::string& filename, const std::string& searchWord, bool specialCase = false);

/** \brief Compares two std::strings
 *   \param str1: first std::string
 *   \param str2: second std::string
 *   \param exact: set to true if the std::strings should match exactly. Set to false if str2 should be contained in str1.
 *   \return false if std::strings do not match; true if std::strings match
 */
STANDARDFILESDLLEXPORT bool compareStrings(const std::string& str1, const std::string& str2, bool exact);

/** \brief Function to replace certain characters inside a std::string
 *   \param Wort: std::string that contains the characters to be replaced
 *   \param oldCharacter: character to be replaced
 *   \param newCharacter: new character
 *   \return new std::string with replaced characters
 */
STANDARDFILESDLLEXPORT std::string replaceAll(std::string Wort, const std::string& oldCharacter, const std::string& newCharacter);

/** \brief Function to replace special characters for Tex output
 *   \param theData: std::string that contains the characters to be replaced
 *   \return new std::string with replaced characters
 */
STANDARDFILESDLLEXPORT std::string stringForTex(std::string theData);

/** \brief Function to convert a relative DOS path to Linux path
 *   \param path: DOS path
 *   \return Linux path
 */
STANDARDFILESDLLEXPORT std::string win2lin(const std::string& path);

/** \brief Convert a path to a relative path starting from a base path
 *   \param Path: full path
 *   \param basePath: base path
 *   \param useUpperLevel: switch if upper level (../) should be added to relative path
 *   \return relative path
 */
STANDARDFILESDLLEXPORT std::string relativePath(const std::string& Path, const std::string& basePath, bool useUpperLevel = true);

/** \brief Deletes a directory and all of its content recursively
 *   \param dir: dreictory to delete
 *   \param recycleBin: Set to true to put the directory into the recycle bin. Set to false to delete the directory irreversibly (currently recyclebin has no effect)
 *   \return true if successful; false if not successful
 */
STANDARDFILESDLLEXPORT bool deleteDirectory(const std::string& dir, bool recycleBin = false);
/** \brief Rounding of Double values
 *   \param number: Double value that needs to be rounded
 *   \param digits: number of digits for rounding
 *   \return rounded Double value
 */
STANDARDFILESDLLEXPORT double Rounding(double number, int digits);
/** \brief Rounding up of Double values
 *   \param number: Double value that needs to be rounded up
 *   \param digits: number of digits for rounding
 *   \return rounded Double value
 */
STANDARDFILESDLLEXPORT double RoundUp(double number, int digits);
/** \brief Rounding down of Double values
 *   \param number: Double value that needs to be rounded down
 *   \param digits: number of digits for rounding
 *   \return rounded Double value
 */
STANDARDFILESDLLEXPORT double RoundDown(double number, int digits);
/** \brief Function tests whether a variable is matches a target variable within a given accuracy
 *   \param value: variable to be tested
 *   \param targetValue: target variable
 *   \param accuracy: tolerance interval
 *   \return false if not within tolerated accuracy interval; true if within tolerated accuracy interval
 */
STANDARDFILESDLLEXPORT bool accuracyCheck(double value, double targetValue, double accuracy);
/** \brief function for 1D linear interpolation
 *   \details function interpolates the value for a given pos between the values of posA and posB; extrapolation is allowed by default.
 *   \param pos: current x position
 *   \param posA: x position of first value
 *   \param valA: f(x) at first x value
 *   \param posB: x position of second value
 *   \param valB: f(x) at second x value
 *   \return interpolated value (f(x) at current x value)
 */
STANDARDFILESDLLEXPORT double linearInterpolation(const double& pos, const double& posA, const double& valA, const double& posB, const double& valB,
                                                  const bool& allowExtrapolation = true);
/** \brief function for trilinear-interpolation (3D-interpolation)
 *   \details function interpolates the value for a given pos in a Matrix (input1_tmp, input2_tmp, input3_tmp) between the values of the 3D output vector;
 *            extrapolation is allowed by default.
 *   \param output: 3D matrix with all available values
 *   \param input1: vector of first value
 *   \param input2: vector of second value
 *   \param input3: vector of third value
 *   \param input1_tmp: first position to be interpolated
 *   \param input2_tmp: second position to be interpolated
 *   \param input3_tmp: third position to be interpolated
 *   \return interpolated value (f(input1_tmp, input2_tmp, input3_tmp))
 */
STANDARDFILESDLLEXPORT double trilinearInterpolation(const std::vector<std::vector<std::vector<double>>>& output, const std::vector<double>& input1,
                                                     const std::vector<double>& input2, const std::vector<double>& input3, const double& input1_tmp, const double& input2_tmp,
                                                     const double& input3_tmp);
/** \brief Function to calculate regression coefficients by using QR decomposition.
 *   \param vecX: vector holding x-values for regression
 *   \param vecY: vector holding y-values for regression
 *   \param orderOfRegression: Order for regression which leads to number of regression coefficients
 *   \return Vector holding regression coefficients
 */
// STANDARDFILESDLLEXPORT std::vector<double> calcRegressionCoefficientsUsingQRdecomp(std::vector<double> vecX, std::vector<double> vecY, uint16_t orderOfRegression);
STANDARDFILESDLLEXPORT std::vector<double> calcRegressionCoefficientsUsingQRdecomp(std::vector<double> vecX, std::vector<double> vecY, uint16_t orderOfRegression);
/** \brief Divides a std::string into multiple characters based on a delimiter
 *   \param str: std::string to be splitted
 *   \param delimiters: delimiters at which str should be splitted
 *   \return tokens: Return value as vector<std::string> holding the splitted parts of std::string
 */
STANDARDFILESDLLEXPORT std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters);

/** \brief Write a file with circle points
 * \details Function writes a file to aFilename containing a description of a circle with numberOfCirclePoints
 * \param aFilename: full path with filename, where the circle is stored in
 * \param numberOfCirclePoint: Number of points of the whole circle; default value is set to 40 as it has been used so far in fuselageDesign
 */
STANDARDFILESDLLEXPORT void writeCircleCoordinates(const std::string& aFilename, const uint16_t& numberOfCirclePoints = 40);

// Inline functions
inline std::string getSettingsFilename(const std::string& aToolname) {
  return aToolname + "_conf.xml";
}

// Templates
template <typename T>
/** \brief Function to cast a number to a std::string
 *   \param aNum: number to be casted
 *   \return std::string
 */
std::string num2Str(T aNum) {
  std::stringstream ss_tmp;
  ss_tmp << aNum;
  return ss_tmp.str();
}

template <typename T>
/** \brief Function to cast a std::string to a number
 *   \param astd::string: std::string to be casted
 *   \return number
 */
T str2Num(const std::string& aString) {
  std::stringstream ss_tmp;
  ss_tmp << aString;
  T result = 0;
  ss_tmp >> result;
  return result;
}

template <typename T>
/** \brief Mathematical sign function
 *   \details Function returns -1 if aValue is < 0, 0 if aValue is = 0, and 1 if aValue is > 0 for all type of numbers
 *   \param aValue: number to be analyzed
 *   \return int: the sign of the number
 */
int signum(const T& aValue) { // used from https://stackoverflow.com/a/4609795
  return (T(0) < aValue) - (aValue < T(0));
}

template <typename T>
/** \brief function for 1D linear vector interpolation
 *   \details function interpolates the vector for a given target position between the vector values of leftPosition and rightPosition;
 *            allowed types are vector<double> and vector<Point>;
 *   \param target: current x position
 *   \param leftPosition: x position of first vector of values
 *   \param leftVector: vector<f(x)> at first x value
 *   \param rightPosition: x position of second vector of values
 *   \param rightVector: vector<f(x)> at second x value
 *   \return interpolated vector of values (vector<f(x)> at current x value)
 */
inline std::vector<T> linearVectorInterpolation(const double& target, const double& leftPosition, const std::vector<T>& leftVector, const double& rightPosition,
                                                const std::vector<T>& rightVector) {
  std::vector<T> result(leftVector.size());
  if (leftVector.size() != rightVector.size()) {
    throw "Vectors of unequal size in linearVectorInterpolation. Abort program.";
  } else {
    result = linearVectorInterpolation(target, leftPosition, leftVector, rightPosition, rightVector);
  }
  return result;
}
template <>
/** \brief function for 1D linear point vector interpolation
 *   \details function interpolates the vector of points for a given target position between the vector values of leftPosition and rightPosition;
 *   \param target: current x position
 *   \param leftPosition: x position of first vector of values
 *   \param leftVector: vector<f(x)> at first x value
 *   \param rightPosition: x position of second vector of values
 *   \param rightVector: vector<f(x)> at second x value
 *   \return interpolated vector of values (vector<f(x)> at current x value)
 */
inline std::vector<Point> linearVectorInterpolation<Point>(const double& target, const double& leftPosition, const std::vector<Point>& leftVector, const double& rightPosition,
                                                           const std::vector<Point>& rightVector) {
  std::vector<Point> result;
  Point tmpResult;
  for (size_t index = 0; index < leftVector.size(); index++) {
    tmpResult.xCoordinate = linearInterpolation(target, leftPosition, leftVector[index].xCoordinate, rightPosition, rightVector[index].xCoordinate);
    tmpResult.yCoordinate = linearInterpolation(target, leftPosition, leftVector[index].yCoordinate, rightPosition, rightVector[index].yCoordinate);
    tmpResult.zCoordinate = linearInterpolation(target, leftPosition, leftVector[index].zCoordinate, rightPosition, rightVector[index].zCoordinate);
    result.push_back(tmpResult);
  }
  return result;
}
template <>
/** \brief function for 1D linear numeric vector interpolation
 *   \details function interpolates the vector of numeric double values for a given target position between the vector values of leftPosition and rightPosition;
 *   \param target: current x position
 *   \param leftPosition: x position of first vector of values
 *   \param leftVector: vector<f(x)> at first x value
 *   \param rightPosition: x position of second vector of values
 *   \param rightVector: vector<f(x)> at second x value
 *   \return interpolated vector of values (vector<f(x)> at current x value)
 */
inline std::vector<double> linearVectorInterpolation<double>(const double& target, const double& leftPosition, const std::vector<double>& leftVector, const double& rightPosition,
                                                             const std::vector<double>& rightVector) {
  std::vector<double> result;
  for (size_t index = 0; index < leftVector.size(); index++) {
    result.push_back(linearInterpolation(target, leftPosition, leftVector[index], rightPosition, rightVector[index]));
  }
  return result;
}

/** \brief function compares if a number is in between two values
 *   \param low lowerBound
 *   \param includeLow bool if lowerBound is included
 *   \param high higherBound
 *   \param includeHigh bool if higherBound is included
 *   \param value the value to be compared
 *   \return true if value is within the bounds
 */
inline bool inRange(const double& low, const double& high, const double& value, const bool& includeLow = true, const bool& includeHigh = true) {
  bool isInRange(true);
  if (includeLow && includeHigh)
    isInRange = ((value - high) * (value - low) <= ACCURACY_HIGH);
  else if (!includeLow && !includeHigh)
    isInRange = ((value - high) * (value - low) < ACCURACY_HIGH);
  else if (includeLow && !includeHigh)
    isInRange = ((value - high) < ACCURACY_HIGH) ? inRange(low, high, value) : false;
  else if (!includeLow && includeHigh)
    isInRange = ((value - low) > ACCURACY_HIGH) ? inRange(low, high, value) : false;
  else
    {}
  return isInRange;
}

/** \brief function aligns x-coordinates of one vector to those of another one
 *   \details the x-coordinates of theVectorToAlign are adapted to those of theFixedVector. The corresponding y- and z-coordinates are interpolated correspondingly.
 *   \param theFixedVector const reference to points of the vector that should be kept constant in x-values
 *   \param theVectorToAlign const reference to points of the vector which x-coordinates are aligned
 *   \return theAlignedVector result of the aligned points
 */
STANDARDFILESDLLEXPORT std::vector<Point> alignLeftAndRightVectorPoints(const std::vector<Point>& theFixedVector, const std::vector<Point>& theVectorToAlign);

/** \brief Calculates the upper and lower boundary (neighbors) to a value within a vector
 *   \param vec const std::vector<T>&: vector in which the value is contained
 *   \param val const T&: value, for which the boundaries shall be found
 *   \param upB typename std::vector<T>::const_iterator*: upper boundary (right neighbor) of the value "val"
 *   \param lowB typename std::vector<T>::const_iterator*: lower boundary (left neighbor) of the value "val"
 *   \return void
 *
 */
template <typename T>
inline void getVectorBounds(const std::vector<T>& vec, const T& val, typename std::vector<T>::const_iterator* upB, typename std::vector<T>::const_iterator* lowB) {
  /* Determine bounds in vec*/
  if (vec.size() < 2) {
    *upB = vec.begin();
    *lowB = vec.begin();
  } else {
    if (std::is_sorted(vec.begin(), vec.end())) { // Vector is sorted in increasing order
      *upB = std::upper_bound(vec.begin(), vec.end(), val);
    } else if (std::is_sorted(vec.begin(), vec.end(), std::greater<>())) { // Vector is sorted in decreasing order
      *upB = std::upper_bound(vec.begin(), vec.end(), val, [](T a, T b) { return a >= b; });
    } else {
      throw("In function \"getVectorBounds()\": Vector is not sorted!");
    }
    if (*upB == vec.begin())
      ++*upB;
    else if (*upB == vec.end())
      --*upB;
    *lowB = std::prev(*upB);
  }
}

/** \brief interpolates value with 1 input dimension: f(input)
 *   \param values: 1-dimensional map with values for interpolation
 *   \param input_vec: input vector
 *   \param input: input value
 *   \return value: interpolated output value with respect to input value: f(input)
 */
template <typename T>
inline T interpn(const std::vector<T>& values, const std::vector<T>& input_vec, const T& input) {
  /* Determine bounds in input_vec*/
  typename std::vector<T>::const_iterator upperBound, lowerBound;
  getVectorBounds(input_vec, input, &upperBound, &lowerBound);
  /* Interpolate value */
  T value_LB = values.at(lowerBound - input_vec.begin());
  T value_UB = values.at(upperBound - input_vec.begin());
  /* Interpolate final value value */
  T value = linearInterpolation(input, input_vec.at(lowerBound - input_vec.begin()), value_LB, input_vec.at(upperBound - input_vec.begin()), value_UB, true);
  return value;
}

/** \brief interpolates value with 2 input dimensions: f(input1, input2)
 *   \param values: 2-dimensional map with values for interpolation
 *   \param input1_vec: input 1 vector
 *   \param input2_vec: input 2 vector
 *   \param input1: input value on 1st level
 *   \param input2: input value on 2nd level
 *   \return value: interpolated output value with respect to input values: f(input1,input2)
 */
template <typename T>
inline T interpn(const std::vector<std::vector<T>>& values, const std::vector<T>& input1_vec, const std::vector<T>& input2_vec, const T& input1, const T& input2) {
  /* Determine bounds in input2_vec*/
  typename std::vector<T>::const_iterator upperBound, lowerBound;
  getVectorBounds(input2_vec, input2, &upperBound, &lowerBound);
  /* Interpolate value at input2 bounds */
  T value_LB = interpn(values.at(lowerBound - input2_vec.begin()), input1_vec, input1);
  T value_UB = interpn(values.at(upperBound - input2_vec.begin()), input1_vec, input1);
  /* Interpolate final value */
  T value = linearInterpolation(input2, input2_vec.at(lowerBound - input2_vec.begin()), value_LB, input2_vec.at(upperBound - input2_vec.begin()), value_UB, true);
  return value;
}

/** \brief interpolates value with 3 input dimensions: f(input1, input2, input3)
 *   \param values: 3-dimensional map with values for interpolation
 *   \param input1_vec: input 1 vector
 *   \param input2_vec: input 2 vector
 *   \param input3_vec: input 3 vector
 *   \param input1: input value on 1st level
 *   \param input2: input value on 2nd level
 *   \param input3: input value on 3rd level
 *   \return value: interpolated output value with respect to input values: f(input1, input2, input3)
 */
template <typename T>
inline T interpn(const std::vector<std::vector<std::vector<T>>>& values, const std::vector<T>& input1_vec, const std::vector<T>& input2_vec, const std::vector<T>& input3_vec,
                 const T& input1, const T& input2, const T& input3) {
  /* Determine bounds in input3_vec*/
  typename std::vector<T>::const_iterator upperBound, lowerBound;
  getVectorBounds(input3_vec, input3, &upperBound, &lowerBound);
  /* Interpolate value at input3 bounds */
  T value_LB = interpn(values.at(lowerBound - input3_vec.begin()), input1_vec, input2_vec, input1, input2);
  T value_UB = interpn(values.at(upperBound - input3_vec.begin()), input1_vec, input2_vec, input1, input2);
  /* Interpolate final value */
  T value = linearInterpolation(input3, input3_vec.at(lowerBound - input3_vec.begin()), value_LB, input3_vec.at(upperBound - input3_vec.begin()), value_UB, true);
  return value;
}

/** \brief interpolates value with 4 input dimensions: f(input1, input2, input3, input4)
 *   \param values: 3-dimensional map with values for interpolation
 *   \param input1_vec: input 1 vector
 *   \param input2_vec: input 2 vector
 *   \param input3_vec: input 3 vector
 *   \param input4_vec: input 4 vector
 *   \param input1: input value on 1st level
 *   \param input2: input value on 2nd level
 *   \param input3: input value on 3rd level
 *   \param input4: input value on 4th level
 *   \return value: interpolated output value with respect to input values: f(input1, input2, input3, input4)
 */
template <typename T>
inline T interpn(const std::vector<std::vector<std::vector<std::vector<T>>>>& values, const std::vector<T>& input1_vec, const std::vector<T>& input2_vec,
                 const std::vector<T>& input3_vec, const std::vector<T>& input4_vec, const T& input1, const T& input2, const T& input3, const T& input4) {
  /* Determine bounds in input3_vec*/
  typename std::vector<T>::const_iterator upperBound, lowerBound;
  getVectorBounds(input4_vec, input4, &upperBound, &lowerBound);
  /* Interpolate value at input3 bounds */
  T value_LB = interpn(values.at(lowerBound - input4_vec.begin()), input1_vec, input2_vec, input3_vec, input1, input2, input3);
  T value_UB = interpn(values.at(upperBound - input4_vec.begin()), input1_vec, input2_vec, input3_vec, input1, input2, input3);
  /* Interpolate final value */
  T value = linearInterpolation(input4, input4_vec.at(lowerBound - input4_vec.begin()), value_LB, input4_vec.at(upperBound - input4_vec.begin()), value_UB, true);
  return value;
}

/** \brief interpolates value with 5 input dimensions: f(input1, input2, input3, input4, input5)
 *   \param values: 3-dimensional map with values for interpolation
 *   \param input1_vec: input 1 vector
 *   \param input2_vec: input 2 vector
 *   \param input3_vec: input 3 vector
 *   \param input4_vec: input 4 vector
 *   \param input5_vec: input 5 vector
 *   \param input1: input value on 1st level
 *   \param input2: input value on 2nd level
 *   \param input3: input value on 3rd level
 *   \param input4: input value on 4th level
 *   \param input5: input value on 5th level
 *   \return value: interpolated output value with respect to input values: f(input1, input2, input3, input4, input5)
 */
template <typename T>
inline T interpn(const std::vector<std::vector<std::vector<std::vector<std::vector<T>>>>>& values, const std::vector<T>& input1_vec, const std::vector<T>& input2_vec,
                 const std::vector<T>& input3_vec, const std::vector<T>& input4_vec, const std::vector<T>& input5_vec, const T& input1, const T& input2, const T& input3,
                 const T& input4, const T& input5) {
  /* Determine bounds in input3_vec*/
  typename std::vector<T>::const_iterator upperBound, lowerBound;
  getVectorBounds(input5_vec, input5, &upperBound, &lowerBound);
  /* Interpolate value at input3 bounds */
  T value_LB = interpn(values.at(lowerBound - input5_vec.begin()), input1_vec, input2_vec, input3_vec, input4_vec, input1, input2, input3, input4);
  T value_UB = interpn(values.at(upperBound - input5_vec.begin()), input1_vec, input2_vec, input3_vec, input4_vec, input1, input2, input3, input4);
  /* Interpolate final value */
  T value = linearInterpolation(input5, input5_vec.at(lowerBound - input5_vec.begin()), value_LB, input5_vec.at(upperBound - input5_vec.begin()), value_UB, true);
  return value;
}

/** \brief interpolates value with 6 input dimensions: f(input1, input2, input3, input4, input5, input6)
 *   \param values: 3-dimensional map with values for interpolation
 *   \param input1_vec: input 1 vector
 *   \param input2_vec: input 2 vector
 *   \param input3_vec: input 3 vector
 *   \param input4_vec: input 4 vector
 *   \param input5_vec: input 5 vector
 *   \param input6_vec: input 6 vector
 *   \param input1: input value on 1st level
 *   \param input2: input value on 2nd level
 *   \param input3: input value on 3rd level
 *   \param input4: input value on 4th level
 *   \param input5: input value on 5th level
 *   \param input6: input value on 6th level
 *   \return value: interpolated output value with respect to input values: f(input1, input2, input3, input4, input5, input6)
 */
template <typename T>
inline T interpn(const std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<T>>>>>>& values, const std::vector<T>& input1_vec, const std::vector<T>& input2_vec,
                 const std::vector<T>& input3_vec, const std::vector<T>& input4_vec, const std::vector<T>& input5_vec, const std::vector<T>& input6_vec, const T& input1,
                 const T& input2, const T& input3, const T& input4, const T& input5, const T& input6) {
  /* Determine bounds in input3_vec*/
  typename std::vector<T>::const_iterator upperBound, lowerBound;
  getVectorBounds(input6_vec, input6, &upperBound, &lowerBound);
  /* Interpolate value at input3 bounds */
  T value_LB = interpn(values.at(lowerBound - input6_vec.begin()), input1_vec, input2_vec, input3_vec, input4_vec, input5_vec, input1, input2, input3, input4, input5);
  T value_UB = interpn(values.at(upperBound - input6_vec.begin()), input1_vec, input2_vec, input3_vec, input4_vec, input5_vec, input1, input2, input3, input4, input5);
  /* Interpolate final value */
  T value = linearInterpolation(input6, input6_vec.at(lowerBound - input6_vec.begin()), value_LB, input6_vec.at(upperBound - input6_vec.begin()), value_UB, true);
  return value;
}

/** \brief Throws an error and exits the program
 * \details Template Function can be called from the code like this: throwError(__FILE__, __FUNCTION__, __LINE__, "Your own error message"), or like this: throwError(__FILE__,
 * __func__, __LINE__, "Your own error message"). throwError(...) throws a std::string as exception. If you want to use other exception type, use e.g.
 * throwError<std::invalid_argument>(...) \param aFile: file in which the error occurred \param aFunction: function in which the error occurred \param aLine: line in which the
 * error occurred \param anErrorMessage: Additional error message
 */
template <typename ExceptionType = std::string>
void throwError(const std::string& aFile, const std::string& aFunction, const int& aLine, const std::string& anErrorMessage = "") {
  std::string theFilename(aFile.substr(aFile.rfind(FILESEPERATOR) + 1));
  if (myRuntimeInfo != nullptr) {
    myRuntimeInfo->err << "In file \"" << theFilename << "\", function \"" << aFunction << "\", line " << aLine << ":" << std::endl;
    throw ExceptionType(anErrorMessage);
  } else {
    throw ExceptionType("In file \"" + theFilename + "\", function \"" + aFunction + "\", line " + std::to_string(aLine) + " : " + anErrorMessage);
  }
}

/** \brief Returns folder size in bytes (recursively)
 *   \param path: folder path
 *   \return folder size in bytes
 */
STANDARDFILESDLLEXPORT std::uintmax_t getDirectorySize(const std::string& path);

/** \brief Converts a file size (bytes) into a std::string with unit (kB, MB, etc.)
 *   \param size: size in bytes
 *   \return size in kB, MB, etc.
 */
STANDARDFILESDLLEXPORT std::string size2Str(std::int64_t size);

/** \brief Creates a directory
 *   \param directoryPath directory path to create
 */
STANDARDFILESDLLEXPORT void createFolder(const std::string& directoryPath);

/** \brief Method handles the signal given by the timeout alarm
 *   \param sig: signature of process
 */
STANDARDFILESDLLEXPORT void handle_timeout(int sig);

/** \brief Method handles the signal given by the child process alarm
 *   \param sig: signature of process
 */
STANDARDFILESDLLEXPORT void handle_child(int sig);

#endif // STANDARDFILES_INCLUDE_STANDARDFILES_FUNCTIONS_H_
