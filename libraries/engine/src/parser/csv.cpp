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

#include "engine/engine_deck.h"
#include <algorithm>
#include <array>
#include <boost/multi_array.hpp>
#include <exception>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

/* === Typedefs === */
using Row = std::pair<double, std::vector<double>>;

/* === CSV Parser === */
/**
 * @class Parser
 * @brief Parses the CSV file and returns the row values.
 * @attention This parser does not check for escaped delimiters
 * or other special cases. It is a simple parser for the
 * specific CSV format used in the engine deck values and designed
 * to be as fast as possible.
 */
class Parser
{
  public:
    /**
     * @brief Construct a new Parser object
     *
     * @param delimiter The delimiter of the CSV file
     */
    explicit Parser(const char delimiter) : delimiter_{delimiter} {};

    /**
     * @brief Parse the first value of the given line.
     *
     * @param line The line to parse
     * @return double The parsed first value
     */
    auto get_first_token(const std::string &line) const -> double
    {
        // Get the iterator the first delimiter
        auto token_start = std::size_t{0};
        auto token_end = line.find_first_of(this->delimiter_, token_start);

        // Parse the first value
        return std::stod(line.substr(token_start, token_end - token_start));
    }

    /**
     * @brief Parse the row values of the given line.
     * @attention This function dynamically allocates memory
     * for the resulting row values!
     *
     * @param line The line to parse
     * @return Row The parsed row values
     */
    auto get_row(const std::string &line) const -> Row
    {
        // Get the iterator the first delimiter
        auto token_start = std::size_t{0};
        auto token_end = line.find_first_of(this->delimiter_, token_start);

        // Parse the first value
        const double first = std::stod(line.substr(token_start, token_end - token_start));

        // Parse the remaining values
        std::vector<double> values;
        token_start = token_end + 1;
        token_end = line.find_first_of(this->delimiter_, token_start);
        while (token_end != std::string::npos)
        {
            const double value = std::stod(line.substr(token_start, token_end - token_start));
            values.emplace_back(value);
            token_start = token_end + 1;
            token_end = line.find_first_of(this->delimiter_, token_start);
        }

        // Return the parsed row
        return {first, values};
    }

    /**
     * @brief Parse the row values of the given line.
     * This function takes the row to be updated as
     * an input value to skip memory allocation once
     * the row size is known to improve speed.
     * @attention The second element of the row must be
     * initialized with the correct size before calling
     * this function!
     *
     * @param line The line to parse
     * @param row The row data to be updated.
     */
    void get_row(const std::string &line, Row *row) const
    {
        // Get the iterator the first delimiter
        auto token_start = std::size_t{0};
        auto token_end = line.find_first_of(this->delimiter_, token_start);

        // Parse the first value
        (*row).first = std::stod(line.substr(token_start, token_end - token_start));

        // Parse the remaining values
        std::size_t index = 0;
        token_start = token_end + 1;
        token_end = line.find_first_of(this->delimiter_, token_start);
        while (token_end != std::string::npos)
        {
            const double value = std::stod(line.substr(token_start, token_end - token_start));
            (*row).second.at(index++) = value;
            token_start = token_end + 1;
            token_end = line.find_first_of(this->delimiter_, token_start);
        }
    }

  private:
    char delimiter_; /** The delimiter for the csv data. */
};

auto DeckData::from_csv(
    const std::filesystem::path &file_name,
    const char delimiter) -> DeckData
{
    /* Open the file stream if its a csv file */
    if (file_name.extension() != ".csv")
    {
        throw std::runtime_error("The file is not a CSV file: " + file_name.string());
    }

    /* Open in binary mode to be a bit faster */
    std::ifstream file(file_name, std::ios::binary);
    std::ios::sync_with_stdio(false); // Get every bit of performance

    /* Check if the file is open */
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file: " + file_name.string());
    }

    /* Create the iterator to read the file lines */
    std::istream_iterator<std::string> line(file);

    /* Create the tokenizer with the custom delimiter */
    Parser parser(delimiter);

    /* Create the data containers */
    std::vector<double> N{};
    std::vector<double> FL{};
    std::vector<double> Mach{};

    /*
     * Parse the first line which defines
     * the length of the Mach vector and
     * the first N value.
     */
    auto row = parser.get_row(*line);
    N.emplace_back(row.first);
    std::copy(
        row.second.begin(),
        row.second.end(),
        std::back_inserter(Mach));

    /* Parse the FL values */
    while (++line != std::istream_iterator<std::string>{})
    {
        /* Parse the row */
        parser.get_row(*line, &row);

        /* Loop until the Mach line is reached again */
        const bool is_Mach_line = std::equal(
            row.second.begin(),
            row.second.end(),
            Mach.begin());

        if (is_Mach_line)
        {
            /* Append the N value */
            N.emplace_back(row.first);

            /* Break the loop */
            break;
        }

        /* Append the FL Vector until it is complete */
        FL.emplace_back(row.first);
    }

    /* Keep on parsing the N values */
    std::size_t table_count{0};
    while (++line != std::istream_iterator<std::string>{})
    {
        /* Only parse the lines which contains N values */
        if (table_count++ == FL.size())
        {
            /* Append the N Vector until it is complete */
            N.emplace_back(parser.get_first_token(*line));

            /* Reset the count */
            table_count = 0;
        }
    }

    /* Resize the value array to fit the data */
    boost::multi_array<double, 3> values{boost::extents[N.size()][FL.size()][Mach.size()]};

    /* Index to access the first two dimensions of the value array */
    std::array index = {std::size_t{0}, std::size_t{0}};

    /* Reset the line iterator */
    file.clear();
    file.seekg(0);
    line = std::istream_iterator<std::string>{file};

    /* Parse the data values now */
    table_count = 0;
    while (++line != std::istream_iterator<std::string>{})
    {
        /* Check if the current line has value data or is a Mach line */
        if (table_count++ < FL.size())
        {
            /* Parse the row */
            parser.get_row(*line, &row);

            /* Copy the data to the value array at the current N and FL position */
            std::copy(
                row.second.begin(),
                row.second.end(),
                values[index[0]][index[1]].begin());

            /* Increment the index of the FL dimension */
            ++index[1];
        }
        else
        {
            /* Go to next table page */
            ++index[0];
            index[1] = 0;
            table_count = 0;
        }
    }
    file.close();

    /* Create the deck data */
    return DeckData{
        file_name.stem().string(), FL, Mach, N, values}; // Return value optimization (RVO) should be applied here
}
