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

#ifndef AIXML_INCLUDE_AIXML_ENDNODE_H_
#define AIXML_INCLUDE_AIXML_ENDNODE_H_
#include <aixml/node.h>
#include <runtimeInfo/runtimeInfo.h>
#include <unitConversion/constants.h>
#include <unitConversion/unitConversion.h>
#include <standardFiles/functions.h>

#include <concepts>
#include <iostream>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <typeinfo>
#include <vector>
#include <format>

/**
 * \brief Concept for numerical or non numerical endnode
 *
 * @tparam T typename (arithmetic or std::string)
 */
template <typename T>
concept is_numeric = (std::is_arithmetic<T>::value && (!std::is_same<T, bool>::value));

/**
 * \brief Endnode Baseclass
 *
 * @tparam T standard c++ types
 */
template <typename T>
class EndnodeBase {
 public:
  /**
   * \brief Construct a new Endnode Base object
   *
   */
  EndnodeBase() = default;

  /**
   * \brief Construct a new Endnode Base object (numeric)
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   */
  explicit EndnodeBase(std::string pathToNode, std::string description)
    requires is_numeric<T>
  {  // NOLINT
    description_ = description;
    paths.push_back(pathToNode);
    paths.push_back("/value");
    paths.push_back("/unit");
    paths.push_back("/lower_boundary");
    paths.push_back("/upper_boundary");
    unit_ = "1";
    lower_boundary_ = std::numeric_limits<T>::lowest();
    upper_boundary_ = std::numeric_limits<T>::max();
    value_ = static_cast<T>(0);
  }

  /**
   * \brief Construct a new Endnode Base object (non numeric - bool)
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   */
  explicit EndnodeBase(std::string pathToNode, std::string description)
    requires(!is_numeric<T> && std::is_same<T, bool>::value)
  {  // NOLINT
    description_ = description;
    paths.push_back(pathToNode);
    paths.push_back("/value");
    value_ = true;
  }

  /**
   * \brief Construct a new Endnode Base object (non numeric - std::string)
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   */
  explicit EndnodeBase(std::string pathToNode, std::string description)
    requires(!is_numeric<T> && std::is_same<T, std::string>::value)
  {  // NOLINT
    description_ = description;
    paths.push_back(pathToNode);
    paths.push_back("/value");
    value_ = "";
  }

  /**
   * \brief Construct a new Endnode Base object
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   * \param default_value default value if node not existent when read
   */
  explicit EndnodeBase(std::string pathToNode, std::string description, T default_value)
      : EndnodeBase(pathToNode, description) {
    value_ = default_value;
  }

  /**
   * \brief Construct a new Endnode Base object (numeric)
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   * \param default_value default value if node not existent when read
   * \param default_unit default unit if node not existent when read
   */
  explicit EndnodeBase(std::string pathToNode, std::string description, T default_value, std::string default_unit)
    requires is_numeric<T>
      : EndnodeBase(pathToNode, description, default_value) {
    if (si_units.contains(default_unit)) {
      unit_ = default_unit;
    } else {
      throw std::invalid_argument("Error - unit " + default_unit + " not part of si-units");
    }
  }

  /**
   * \brief Construct a new Endnode Base object (numeric)
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   * \param default_value default value if node not existent when read
   * \param default_unit default unit if node not existent when read
   * \param default_lower_boundary default lower boundary if node not existent when read
   * \param default_upper_boundary default upper boundary if node not existent when read
   */
  explicit EndnodeBase(std::string pathToNode, std::string description, T default_value, std::string default_unit,
                       T default_lower_boundary, T default_upper_boundary)
    requires is_numeric<T>
      : EndnodeBase(pathToNode, description, default_value, default_unit) {
    lower_boundary_ = default_lower_boundary;
    upper_boundary_ = default_upper_boundary;
  }

  /**
   * \brief Read endnode  (numeric)
   *
   * \param xml node of file
   * \return EndnodeBase& instance reference of current object
   */
  EndnodeBase& read(const node& xml)
    requires is_numeric<T>
  {  // NOLINT
    try {
      description_ = xml.at(paths[0]).getStringAttrib("description");
      unit_ = static_cast<std::string>(xml.at(paths[0] + paths[2]));
      value_ = static_cast<T>(xml.at(paths[0] + paths[1]));
      lower_boundary_ = static_cast<T>(xml.at(paths[0] + paths[3]));
      upper_boundary_ = static_cast<T>(xml.at(paths[0] + paths[4]));
    } catch (...) {
      if (myRuntimeInfo != nullptr) {
        myRuntimeInfo->warn << "Node to read not existing ... " << paths.at(0) << std::endl;
      } else {
        std::cerr << "Node to read not existing ... " << paths.at(0) << std::endl;
      }
    }
    check_boundaries();
    return *this;
  }

  /**
   * \brief Read endnode (non-numeric)
   *
   * \param xml node of file
   * \return EndnodeBase& instance reference of current object
   */
  EndnodeBase& read(const node& xml)
    requires(!is_numeric<T>)
  {  // NOLINT
    try {
      description_ = xml.at(paths[0]).getStringAttrib("description");
      if (std::is_same<T, bool>::value) {
        value_ = static_cast<std::string>(xml.at(paths[0] + paths[1])) == "true";
      } else {
        value_ = static_cast<T>(xml.at(paths[0] + paths[1]));
      }
    } catch (...) {
      if (myRuntimeInfo != nullptr) {
        myRuntimeInfo->warn << "Node to read not existing ... " << paths.at(0) << std::endl;
      } else {
        std::cerr << "Node to read not existing ... " << paths.at(0) << std::endl;
      }
    }
    return *this;
  }

  /**
   * \brief Prints endnode (numeric)
   *
   */
  void print()
    requires is_numeric<T>
  {  // NOLINT
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->out << std::right << "Description: " << std::setw(10) << description_ << '\n'
                         << "Value: " << std::setw(10) << std::scientific << value_ << '\n'
                         << "Unit : " << std::setw(10) << unit_ << '\n'
                         << "lb   : " << std::setw(10) << std::scientific << lower_boundary_ << '\n'
                         << "ub   : " << std::setw(10) << std::scientific << upper_boundary_ << '\n';
    } else {
      std::cout << std::right << "Description: " << std::setw(10) << description_ << '\n'
                << "Value: " << std::setw(10) << std::scientific << value_ << '\n'
                << "Unit : " << std::setw(10) << unit_ << '\n'
                << "lb   : " << std::setw(10) << std::scientific << lower_boundary_ << '\n'
                << "ub   : " << std::setw(10) << std::scientific << upper_boundary_ << '\n';
    }
  }

  /**
   * \brief Prints endnode (non-numeric)
   *
   */
  void print()
    requires(!is_numeric<T>)
  {  // NOLINT
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->out << "Text: " << value_ << std::endl;
    } else {
      std::cout << "Text: " << value_ << std::endl;
    }
  }

  /**
   * \brief Get value from object
   *
   * \return T value of object type (si-units)
   */
  T value(void) const { return value_; }

  /**
   * \brief Get unit from object
   *
   * \return std::string unit of object (si-units)
   */
  std::string unit(void) const
    requires is_numeric<T>
  {  // NOLINT
    return unit_;
  }

  /**
   * \brief Get lower boundary from object (numeric)
   *
   * \return T lower boundary value (si-units)
   */
  T lower_boundary(void) const
    requires is_numeric<T>
  {  // NOLINT
    return lower_boundary_;
  }

  /**
   * \brief Get upper boundary from object (numeric)
   *
   * \return T upper boundary value (si-units)
   */
  T upper_boundary(void) const
    requires is_numeric<T>
  {  // NOLINT
    return upper_boundary_;
  }

  /**
   * \brief Check value if insight boundaries (numeric)
   *
   * \return bool true if checks passed, false if checks failed + warning
   */
  void check_boundaries(void) const
    requires is_numeric<T>
  {  // NOLINT
    if (value_ < lower_boundary_ || value_ > upper_boundary_) {
      throwError<std::out_of_range>(__FILE__, __func__, __LINE__, std::format("Boundary violation in {}. Current value {} is out of bounds", paths.at(0), value_));
    }
  }

  /**
   * \brief Check unit value. If part of si_units (true) - allowed custom units (see Phabricator
   * https://unicado.ilr.rwth-aachen.de/w/style_guides/unit_convension) then false -> otherwise throw exception with
   * information (numeric)
   *
   * \return true unit is part of si_unit
   * \return false part of allowed custom units
   */
  bool check_unit()
    requires is_numeric<T>
  {  // NOLINT
    if (!si_units.contains(unit_)) {
      if (!custom_to_si.contains(unit_)) {
        throw std::invalid_argument("No valid unit for " + paths.at(0) + " of type -> " + unit_);
      } else {
        return false;
      }
    }
    return true;
  }

  /**
   * \brief Convert items in object if unit part of custom units
   *
   */
  void convert_items()
    requires is_numeric<T>
  {  // NOLINT
    if (!check_unit()) {
      // Write current values to xml values
      value_ *= to_si_factors.at(unit_);
      lower_boundary_ *= to_si_factors.at(unit_);
      upper_boundary_ *= to_si_factors.at(unit_);
      unit_ = custom_to_si.at(unit_);
    }
  }

 protected:
  std::string description_{""};
  std::vector<std::string> paths;

  T value_;
  T lower_boundary_;
  T upper_boundary_;
  std::string unit_;

  const std::set<std::string> si_units{
      "m",       // meter
      "m^2",     // sqauremeter
      "m^3",     // cubicmeter
      "m/s",     // meter per second
      "rad",     // radian
      "1",       // no unit
      "Pa",      // pascal
      "kg",      // kilogram
      "kg/s",    // kilogram per second
      "kg/Ns",   // kilogram per newton second
      "s",       // second
      "J",       // Joule
      "J/kg",    // Joule per kilogram
      "J/m^3",   // Joule per cubic meter
      "W",       // Watt
      "V",       // volt
      "A",       // ampere
      "N",       // newton
      "N/m^2",   // newton per square meter
      "kg/m^2",  // kilogram per square meter
      "kg/m^3",  // kilogram per cubic meter
      "kgm^2",   // kilogram square meter
      "K",       // Kelvin
      "EUR",     // Euro
      "US",      // Dollar
  };
  const std::map<std::string, std::string> custom_to_si = {
      {"deg", "rad"}, {"ft", "m"},      {"FL", "m"},          {"NM", "m"},       {"lbs", "kg"},   {"lbs/min", "kg/s"},
      {"EUR", "EUR"}, {"US", "US"},     {"g", "1"},           {"dBA", "Pa"},     {"EPNdB", "Pa"}, {"Sone", "Pa"},
      {"h", "s"},     {"min", "s"},     {"a", "s"},           {"ft/min", "m/s"}, {"kts", "m/s"},  {"KCAS", "m/s"},
      {"l", "m^3"},   {"Celsius", "K"}, {"micro meter", "m"}, {"kg/h", "kg/s"},   {"kWh", "J"}};

  const std::map<std::string, double> to_si_factors = {  // NOLINT
      {"deg", convertUnit(DEGREE, RADIAN, 1.0)},
      {"ft", convertUnit(FOOT, METER, 1.0)},
      {"FL", convertUnit(FOOT, METER, 100.0)},
      {"NM", convertUnit(NAUTICALMILE, METER, 1.0)},
      {"lbs", convertUnit(NOPREFIX, POUND, KILO, GRAM, 1.0)},
      {"lbs/min", convertUnit(NOPREFIX, POUND, KILO, GRAM, 1.0) / convertUnit(MINUTE, SECOND, 1.0)},
      {"kg/h", 1.0 / convertUnit(HOUR, SECOND, 1.0)},
      {"EUR", 1.0},    // Missing conversion
      {"US", 1.0},     // Missing conversion
      {"g", 1.0},      // Missing conversion
      {"dBA", 1.0},    // Missing conversion
      {"EPNdB", 1.0},  // Missing conversion
      {"Sone", 1.0},   // Missing conversion
      {"h", convertUnit(HOUR, SECOND, 1.0)},
      {"min", convertUnit(MINUTE, SECOND, 1.0)},
      {"a", convertUnit(HOUR, SECOND, 24.0 * 365)},
      {"ft/min", convertUnit(FOOT, METER, 1.0) / convertUnit(MINUTE, SECOND, 1.0)},
      {"kts", convertUnit(KNOTS, METERPERSECOND, 1.0)},
      {"KCAS", convertUnit(KNOTS, METERPERSECOND, 1.0)},
      {"l", 0.001},  // Missing conversion - not populated by convertUnit
      {"Celsius", convertUnit(CELCIUS, KELVIN, 1.0)},
      {"micro meter", convertUnit(MICRO, METER, NOPREFIX, METER, 1.0)},
      {"kWh", convertUnit(KILO, WATTHOUR, NOPREFIX, JOULE, 1.0)}};
};

/**
 * \brief Endnode class - derived from EndnodeBase class
 *
 * @tparam T typename (arithmetic or std::string)
 */
template <typename T>
class Endnode : public EndnodeBase<T> {
 public:
  /**
   * \brief Construct a new Endnode object
   *
   */
  Endnode() : EndnodeBase<T>() {}

  /**
   * \brief Construct a new Endnode object
   *
   * \param pathToNode
   * \param description
   */
  explicit Endnode(std::string pathToNode, std::string description) : EndnodeBase<T>(pathToNode, description) {}

  /**
   * \brief Construct a new Endnode object
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   * \param default_value default value if node not existent when read
   */
  explicit Endnode(std::string pathToNode, std::string description, T default_value)
      : EndnodeBase<T>(pathToNode, description, default_value) {}

  /**
   * \brief Construct a new Endnode object (numeric)
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   * \param default_value default value if node not existent when read
   * \param default_unit default unit if node not existent when read
   */
  explicit Endnode(std::string pathToNode, std::string description, T default_value, std::string default_unit)
      : EndnodeBase<T>(pathToNode, description, default_value, default_unit) {}

  /**
   * \brief Construct a new Endnode object (numeric)
   *
   * \param pathToNode path to xml node without / at the end
   * \param description description of node in xml -> will be written with update method
   * \param default_value default value if node not existent when read
   * \param default_unit default unit if node not existent when read
   * \param default_lower_boundary default lower boundary if node not existent when read
   * \param default_upper_boundary default upper boundary if node not existent when read
   */
  explicit Endnode(std::string pathToNode, std::string description, T default_value, std::string default_unit,
                   T default_lower_boundary, T default_upper_boundary)
      : EndnodeBase<T>(pathToNode, description, default_value, default_unit, default_lower_boundary,
                       default_upper_boundary) {}

  /**
   * \brief Read endnode element from xml file at pathToNode (numeric)
   *
   * \param xml node of file
   * \return Endnode& Endnode object
   */
  Endnode& read(const node& xml)  // NOLINT
    requires is_numeric<T>
  {  // NOLINT
    EndnodeBase<T>::read(xml);
    if (!EndnodeBase<T>::check_unit()) {
      EndnodeBase<T>::convert_items();
    }
    return *this;
  }

  /**
   * \brief Read endnode element from xml file at pathToNode (non-numeric)
   *
   * \param xml node of file
   * \return Endnode& Endnode object
   */
  Endnode& read(const node& xml)  // NOLINT
    requires(!is_numeric<T>)
  {  // NOLINT
    EndnodeBase<T>::read(xml);
    return *this;
  }

  /**
   * \brief Update endnode element in xml at pathToNode (numeric)
   *
   * \param xml node of file
   */
  void update(node& xml)  // NOLINT
    requires is_numeric<T>
  {  // NOLINT
    EndnodeBase<T>::check_boundaries();
    std::string origin = EndnodeBase<T>::paths[0];
    xml[origin].setAttrib<std::string>("description", EndnodeBase<T>::description_);
    xml[origin + EndnodeBase<T>::paths[1]] = EndnodeBase<T>::value_;
    xml[origin + EndnodeBase<T>::paths[2]] = EndnodeBase<T>::unit_;
    xml[origin + EndnodeBase<T>::paths[3]] = EndnodeBase<T>::lower_boundary_;
    xml[origin + EndnodeBase<T>::paths[4]] = EndnodeBase<T>::upper_boundary_;
  }

  /**
   * \brief Update endnode element in xml at pathToNode (non-numeric - bool)
   *
   * \param xml node of file
   */
  void update(node& xml)  // NOLINT
    requires(!is_numeric<T> && std::is_same<T, bool>::value)
  {  // NOLINT
    std::string origin = EndnodeBase<T>::paths[0];
    xml[origin].setAttrib<std::string>("description", EndnodeBase<T>::description_);
    xml[origin + EndnodeBase<T>::paths[1]] = (EndnodeBase<T>::value_ == true) ? "true" : "false";
  }

  /**
   * \brief Update endnode element in xml at pathToNode (non-numeric - std::string)
   *
   * \param xml node of file
   */
  void update(node& xml)  // NOLINT
    requires(!is_numeric<T> && std::is_same<T, std::string>::value)
  {  // NOLINT
    std::string origin = EndnodeBase<T>::paths[0];
    xml[origin].setAttrib<std::string>("description", EndnodeBase<T>::description_);
    xml[origin + EndnodeBase<T>::paths[1]] = EndnodeBase<T>::value_;
  }

  /**
   * \brief Set the value object
   *
   * \param val value to set
   */
  void set_value(const T val) { EndnodeBase<T>::value_ = val; }

  /**
   * \brief Set the unit object
   *
   * \param unit unit to set -> must be part of si-units
   */
  void set_unit(const std::string& unit)
    requires is_numeric<T>
  {  // NOLINT
    if (EndnodeBase<T>::si_units.contains(unit)) {
      EndnodeBase<T>::unit_ = unit;
    } else {
      throw std::invalid_argument("Error - no si-unit: " + unit);
    }
  }

  /**
   * \brief Set lower boundary of object (numeric)
   *
   * \param lb lower boundary
   */
  void set_lower_boundary(const T lb)  // NOLINT
    requires is_numeric<T>
  {  // NOLINT
    EndnodeBase<T>::lower_boundary_ = lb;
  }

  /**
   * \brief Set upper boundary of object (numeric)
   *
   * \param ub upper boundary
   */
  void set_upper_boundary(const T ub)
    requires is_numeric<T>
  {  // NOLINT
    EndnodeBase<T>::upper_boundary_ = ub;
  }

  /**
   * \brief Set the boundaries object (numeric)
   *
   * \param lb lower boundary
   * \param ub upper boundary
   */
  void set_boundaries(const T lb, const T ub)
    requires is_numeric<T>
  {  // NOLINT
    set_lower_boundary(lb);
    set_upper_boundary(ub);
  }

  /**
   * \brief Assign operator value
   *
   * \param value value to assign
   */
  void operator=(const T value) { this->set_value(value); }

  /**
   * \brief Add operator (single value)
   *
   * \param val value of type T
   * \return Endnode<T>& reference to current class
   */
  Endnode<T>& operator+=(const T& val)
    requires is_numeric<T>
  {  // NOLINT
    this->set_value(this->value() + val);
    return *this;
  }

  /**
   * \brief Subtract operator (single value)
   *
   * \param val value of type T
   * \return Endnode<T>& reference to current class
   */
  Endnode<T>& operator-=(const T& val)
    requires is_numeric<T>
  {  // NOLINT
    this->set_value(this->value() - val);
    return *this;
  }

  /**
   * \brief Multiply operator (single value)
   *
   * \param val value of type T
   * \return Endnode<T>& reference to current class
   */
  Endnode<T>& operator*=(const T& val)
    requires is_numeric<T>
  {  // NOLINT
    this->set_value(this->value() * val);
    return *this;
  }

  /**
   * \brief Divide operator (single value)
   *
   * \param val value of type T
   * \return Endnode<T>& reference to current class
   */
  Endnode<T>& operator/=(const T& val)
    requires is_numeric<T>
  {  // NOLINT
    this->set_value(this->value() / val);
    return *this;
  }

  /**
   * \brief Assign operator (numeric)
   *
   * \param boundaries boundaries to assign as tuple
   */
  void operator=(const std::tuple<T, T> boundaries)
    requires is_numeric<T>
  {  // NOLINT
    set_lower_boundary(std::get<0>(boundaries));
    set_upper_boundary(std::get<1>(boundaries));
  }

  /**
   * \brief Assign operator
   *
   * \param info unit and boundaries to assign as tuple
   */
  void operator=(const std::tuple<std::string, T, T> info)
    requires is_numeric<T>
  {  // NOLINT
    set_unit(std::get<0>(info));
    set_lower_boundary(std::get<1>(info));
    set_upper_boundary(std::get<2>(info));
  }
};

/**
 * \brief EndnodeReadOnly class - derived from EndnodeBase class
 *
 * @tparam T typename (arithmetic or std::string)
 */
template <typename T>
class EndnodeReadOnly : public EndnodeBase<T> {
 public:
  /**
   * \brief Construct a new Endnode Read Only object
   *
   */
  EndnodeReadOnly() : EndnodeBase<T>() {}

  /**
   * \brief Construct a new Endnode Read Only object
   *
   * \param pathToNode
   */
  explicit EndnodeReadOnly(std::string pathToNode) : EndnodeBase<T>(pathToNode, "") {}

  /**
   * \brief Construct a new Endnode Read Only object
   *
   * \param pathToNode
   * \param default_value
   */
  explicit EndnodeReadOnly(std::string pathToNode, T default_value) : EndnodeBase<T>(pathToNode, "", default_value) {
    value_xml_ = default_value;
  }

  /**
   * \brief Construct a new Endnode Read Only object
   *
   * \param pathToNode
   * \param default_value
   * \param default_unit
   */
  explicit EndnodeReadOnly(std::string pathToNode, T default_value, std::string default_unit)
      : EndnodeBase<T>(pathToNode, "", default_value, default_unit) {
    value_xml_ = default_value;
    unit_xml_ = default_unit;
  }

  /**
   * \brief Construct a new Endnode Read Only object
   *
   * \param pathToNode
   * \param default_value
   * \param default_unit
   * \param default_lower_boundary
   * \param default_upper_boundary
   */
  explicit EndnodeReadOnly(std::string pathToNode, T default_value, std::string default_unit, T default_lower_boundary,
                           T default_upper_boundary)
      : EndnodeBase<T>(pathToNode, "", default_value, default_unit, default_lower_boundary, default_upper_boundary) {
    value_xml_ = default_value;
    unit_xml_ = default_unit;
    lower_boundary_xml_ = default_lower_boundary;
    upper_boundary_xml_ = default_upper_boundary;
  }

  /**
   * \brief Read endnode element from xml file at pathToNode (numeric)
   *
   * \param xml node of file
   * \return EndnodeReadOnly& EndnodeReadOnly object reference
   */
  EndnodeReadOnly& read(const node& xml)
    requires is_numeric<T>
  {  // NOLINT
    EndnodeBase<T>::read(xml);
    value_xml_ = EndnodeBase<T>::value_;
    unit_xml_ = EndnodeBase<T>::unit_;
    lower_boundary_xml_ = EndnodeBase<T>::lower_boundary_;
    upper_boundary_xml_ = EndnodeBase<T>::upper_boundary_;
    if (!EndnodeBase<T>::check_unit()) {
      EndnodeBase<T>::convert_items();
    }
    return *this;
  }

  /**
   * \brief Read endnode element from xml file at pathToNode (non-numeric)
   *
   * \param xml node of file
   * \return EndnodeReadOnly& EndnodeReadOnly object reference
   */
  EndnodeReadOnly& read(const node& xml)
    requires(!is_numeric<T>)
  {  // NOLINT
    EndnodeBase<T>::read(xml);
    return *this;
  }

  /**
   * \brief Value from xml (unparsed - numeric)
   *
   * \return T value of endnode element
   */
  T value_xml() const { return value_xml_; }

  /**
   * \brief Unit from xml (unparsed - numeric)
   *
   * \return std::string unit of endnode element
   */
  std::string unit_xml() const
    requires is_numeric<T>
  {  // NOLINT
    return unit_xml_;
  }

  /**
   * \brief Lower boundary from xml (unparsed - numeric)
   *
   * \return T lower boundary of endnode element
   */
  T lower_boundary_xml() const
    requires is_numeric<T>
  {  // NOLINT
    return lower_boundary_xml_;
  }

  /**
   * \brief upper_boundary from xml (unparsed - numeric)
   *
   * \return T upper boundary of endnode element
   */
  T upper_boundary_xml() const
    requires is_numeric<T>
  {  // NOLINT
    return upper_boundary_xml_;
  }

  /**
   * \brief Print unparsed xml values (numeric)
   *
   */
  void print_xml() const
    requires is_numeric<T>
  {  // NOLINT
    if (myRuntimeInfo != nullptr) {
      myRuntimeInfo->out << std::right << "Description: " << std::setw(10) << EndnodeBase<T>::description_ << '\n'
                         << "Value: " << std::setw(10) << std::scientific << value_xml_ << '\n'
                         << "Unit : " << std::setw(10) << unit_xml_ << '\n'
                         << "lb   : " << std::setw(10) << std::scientific << lower_boundary_xml_ << '\n'
                         << "ub   : " << std::setw(10) << std::scientific << upper_boundary_xml_ << '\n';
    } else {
      std::cout << std::right << "Description: " << std::setw(10) << EndnodeBase<T>::description_ << '\n'
                << "Value: " << std::setw(10) << std::scientific << value_xml_ << '\n'
                << "Unit : " << std::setw(10) << unit_xml_ << '\n'
                << "lb   : " << std::setw(10) << std::scientific << lower_boundary_xml_ << '\n'
                << "ub   : " << std::setw(10) << std::scientific << upper_boundary_xml_ << '\n';
    }
  }

  /**
   * \brief Print parsed xml values
   *
   */
  void print_xml() const
    requires(!is_numeric<T>)
  {  // NOLINT
    EndnodeBase<T>::print();
  }

 protected:
  T value_xml_;
  std::string unit_xml_;
  T lower_boundary_xml_;
  T upper_boundary_xml_;
};

#endif  // AIXML_INCLUDE_AIXML_ENDNODE_H_
  // AIXML_INCLUDE_AIXML_ENDNODE_H_
