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

#ifndef ENGINE_ENGINE_DECK_H_
#define ENGINE_ENGINE_DECK_H_

/* === Includes === */
#include <boost/multi_array.hpp>
#include <filesystem>
#include <string>
#include <vector>

#ifdef BUILD_ENGINE_SHARED
#ifdef _WIN32
#define ENGINEDLLEXPORT __declspec(dllexport)
#else
#define ENGINEDLLEXPORT __attribute__((visibility("default")))
#endif
#elif defined(IMPORT_ENGINE_SHARED)
#ifdef _WIN32
#define ENGINEDLLEXPORT __declspec(dllimport)
#else
#define ENGINEDLLEXPORT
#endif
#else
#define ENGINEDLLEXPORT
#endif

/* === Classes === */

/**
 * @struct OperatingPoint
 * @brief Contains all relevant data of the operating point.
 * @param N [-] The engine speed compared to nominal speed.
 * @param altitude [m] The flight altitude.
 * @param Mach [-] The Mach number.
 */
struct OperatingPoint
{
    double N{0.0};        /** [-] Engine speed compared to nominal speed. */
    double altitude{0.0}; /** [m] Flight altitude */
    double Mach{0.0};     /** [-] Mach number */
};

/**
 * @struct DeckData
 * @brief Contains all relevant data of the deck value.
 * @attention This can be a large data structure! So,
 * avoid copying it and prefer to pass it by reference.
 */
struct DeckData
{
    std::string name{"unknown"};            /** The name of the deck */
    std::vector<double> FL{};               /** [m] The vector of flight levels */
    std::vector<double> Mach{};             /** [-] The Mach numbers */
    std::vector<double> N{};                /** [-] The engine speed compared to nominal speed */
    boost::multi_array<double, 3> values{}; /** The raw data of the deck value */

    /**
     * @brief Parse the CSV file and return the deck value.
     *
     * @param file_name The file name of the CSV file.
     * @param delimiter The delimiter of the CSV file. Default is ";"
     * @return DeckData The parsed deck value.
     * @throws std::runtime_error If the file could not be opened or is not a valid CSV file.
     */
    [[nodiscard]] ENGINEDLLEXPORT static auto from_csv(
        const std::filesystem::path &file_name, char delimiter = ';') -> DeckData;
};

class ENGINEDLLEXPORT DeckValue
{
  public:
    DeckValue(const DeckData &data) : data_(std::move(data)) { }

    /**
     * @brief Get the value of the deck at the given operating point.
     * Performs a linear interpolation of the deck value at the given operating point.
     *
     * @param operating_point The operating point of the engine.
     * @return double The value of the deck at the given operating point.
     * @throws std::out_of_range If the operating point is outside of the deck value range.
     */
    [[nodiscard]] auto get_value_at(const OperatingPoint &operating_point) const -> double;

    /**
     * @brief Get the minimum deck value for N1, Mach and Altitude
     *
     * @return OperatingPoint the minimal possible operating point.
     */
    [[nodiscard]] auto lower_boundary() const -> OperatingPoint;

    /**
     * @brief Get the max deck value for N1, Mach and Altitude
     *
     * @return OperatingPoint the maximal possible operating point.
     */
    [[nodiscard]] auto upper_boundary() const -> OperatingPoint;


  private:
    /**
     * @brief Check whether the operating point is within the range
     * of the underlying deckdata.
     *
     * @param operating_point The operating point of the engine.
     * @return true If the operating point is within a valid range.
     */
    [[nodiscard]] auto is_within_range(const OperatingPoint &operating_point) const -> bool;

    /* === Properties === */
    DeckData data_; /** The deck value data */
};

///** \brief Class for the description of the engineDeck
// */
//class ENGINEDLLEXPORT EngineDeck
//{
//  public:
//    /* Member functions */
//    //double getVal(double *FL_mom, double *Mach_mom, double *N_mom, const bool &checkValidValues, const bool &changeSpoolSpeed = true) const;
//    /* Constructor and destructor */
//    explicit EngineDeck(const std::string &filename);
//    EngineDeck();
//
//    virtual ~EngineDeck();
//
//  private:
//    /* Member functions */
//    std::vector<std::vector<std::vector<double>>> dtensor(const int &n, const int &m, const int &p);
//    /* Member variables */
//    double FL_min;
//    double FL_max;
//    double Mach_min;
//    double Mach_max;
//    double N_min;
//    double N_max;
//    std::vector<std::vector<std::vector<double>>> values;
//    std::vector<double> FL;
//    std::vector<double> Mach;
//    std::vector<double> N;
//    std::string aName;
//};

#endif // ENGINE_ENGINE_DECK_H_
