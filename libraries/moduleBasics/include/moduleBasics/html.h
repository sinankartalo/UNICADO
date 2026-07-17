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

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_HTML_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_HTML_H_

/* === Includes === */
#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

/* === Functions === */
namespace html {

    /**
     * @struct Element
     * @brief Helper struct to combine html elements text and attribute.
     * 
     * This struct is used to group the text and attribute of html elements together.
     * This way you can easily loop through a list of elements and have access to the
     * text and corresponding attribute of each element.
     */
    struct Element {
        /* === Default Constructors === */
        Element() = default;
        Element(const Element &) = default;
        Element(Element &&) = default;
        auto operator=(const Element &) -> Element & = default;
        auto operator=(Element &&) -> Element & = default;
        ~Element() = default;

        /* === Custom constructors so that struct appears as a string view === */
        Element(const std::string_view &text, const std::string_view &attribute) : text(text), attribute(attribute) {}
        explicit Element(const std::string_view &text) : Element(text, "") {}
        Element(const char *text) : Element(std::string_view(text), "") {} //NOLINT (google-explicit-constructor) Otherwise the initializer list does not work

        /* === Properties === */
        std::string_view text{}; /**< The text this html element contains. */
        std::string_view attribute{}; /**< The attribute text of this html element. */
    };

    /**
     * @brief Add a h1 header
     * 
     * @param text The text of the header
     * @return std::string The html formatted header
     */
    inline auto h1(const std::string_view &text) -> std::string {
        std::stringstream stream;
        stream << "<h1>" << text << "</h1>"
               << "\n";
        return stream.str();
    }

    /**
     * @brief Add a h2 header
     * 
     * @param text The text of the header
     * @return std::string The html formatted header
     */
    inline auto h2(const std::string_view &text) -> std::string {
        std::stringstream stream;
        stream << "<h2>" << text << "</h2>"
               << "\n";
        return stream.str();
    }

    /**
     * @brief Add a h3 header
     * 
     * @param text The text of the header
     * @return std::string The html formatted header
     */
    inline auto h3(const std::string_view &text) -> std::string {
        std::stringstream stream;
        stream << "<h3>" << text << "</h3>"
               << "\n";
        return stream.str();
    }

    /**
     * @brief Start a div column with the optional additional
     * attributes. The attributes can also be empty.
     * 
     * Can be used like this: `html_stream << html::div_start("id=\\"leftCol\\"");`
     * 
     * @attention Do not forget to close the div with `html_stream << html::div_end;` again.
     * This does not keep track of the divs that are started and closed!
     * @param attributes The attributes of the div
     * @return std::string The html formatted div
     */
    inline auto div_start(const std::string_view &attributes) -> std::string {
        std::stringstream stream;
        stream << "<div " << attributes << ">"
               << "\n";
        return stream.str();
    }

    /**
     * @brief Add an image with the given source path and attributes.
     * 
     * @param src Path to the image
     * @param attributes The attributes of the image
     * @return std::string The html formatted image
     */
    inline auto image(const std::string_view &src, const std::string_view &attributes) -> std::string {
        std::stringstream stream;
        stream << "<img src=\"" << src << "\" " << attributes << ">"
               << "\n";
        return stream.str();
    }

    /**
     * @brief Start a table with the given headers.
     * 
     * @attention Do not forget to close the table with `html_stream << html::table_end;` again.
     * This does not keep track of the tables that are started and closed!
     * @param headers The list with the header elements of the table. Each element can have an attribute.
     * @param caption Optional, the caption of the table.
     * @param attributes Optional, additional attributes for the table.
     * @return std::string The html formatted start of the table
     */
    inline auto table_begin(const std::vector<Element> &headers, const std::string_view caption = "", const std::string_view attributes = "") -> std::string {
        /* Start the table */
        std::stringstream stream;
        stream << "<table class=\"content-table\""
               << attributes << ">"
               << "\n";

        /* Add the caption */
        if (!caption.empty()) {
            stream << "<caption>" << caption << "</caption>"
                   << "\n";
        }

        /* Add the headers and their attributes */
        stream << "<thead>" << "\n"
               << "<tr>" << "\n";
        std::for_each(headers.begin(), headers.end(), [&stream](const Element &header)
                      { stream << "<th " << header.attribute << ">" << header.text << "</th>"
                               << "\n"; });
        stream << "</tr>" << "\n"
               << "</thead>" << "\n";
        return stream.str();
    }

    /**
     * @brief Insert a table data item,
     * 
     * @tparam T The type of the value
     * @param value The value to insert
     * @return std::string The html formatted table data
     */
    template <typename T>
    inline auto table_data(const T &value) -> std::string {
        std::stringstream stream;
        stream << "<td>" << value << "</td>";
        return stream.str();
    }

    /**
     * @brief Recursively add all the values to a table row
     * @details This is a recursive function that puts a
     * single data item into a table row until the last
     * data item is reached.
     * 
     * @tparam T The type of the value
     * @tparam Targs The type of the other values
     * @param value The value to insert
     * @param values The other values to insert
     * @return std::string The html formatted table data items
     */
    template <typename T, typename... Targs>
    inline auto table_data(const T &value, const Targs &... values) -> std::string {
        std::stringstream stream;
        stream << table_data(value)
               << table_data(values...);
        return stream.str();
    }

    /**
     * @brief Add all the values to a table row
     * The values are added to the table row in the order they are given.
     * @note You can use as many table entries as you want, but make sure
     * that within one table each row has the same number of entries. This
     * does not track the number of columns of the table!
     * 
     * @tparam Targs The list of types of the values
     * @param values The values to insert
     * @return std::string The html formatted table row
     */
    template<typename... Targs>
    inline auto table_row(const Targs &...values) -> std::string {
        std::stringstream stream;
        stream << "<tr>"
               << "\n";
        stream << table_data(values...);
        stream << "</tr>"
               << "\n";
        return stream.str();
    }

    /**
     * @brief Finish the table
     * 
     * Can be used like this: `html_stream << html::table_end;`
     * 
     * @param stream The stream to finish the table
     * @return std::basic_ostream<_CharT, _Traits>& Returns the stream
     */
    template <class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &
    table_end(std::basic_ostream<_CharT, _Traits> &stream) { // NOLINT
        stream << "</table>"
               << "\n";
        return stream;
    }

    /**
     * @brief Finish the div
     * 
     * Can be used like this: `html_stream << html::div_end;`
     * 
     * @param stream The stream to finish the div
     * @return std::basic_ostream<_CharT, _Traits>& Returns the stream
     */
    template <class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &
    div_end(std::basic_ostream<_CharT, _Traits> &stream) { // NOLINT
        stream << "</div>"
               << "\n";
        return stream;
    }

    /**
     * @brief Add a horizontal rule
     * 
     * Can be used like this: `html_stream << html::hrule;`
     * 
     * @param stream The stream to add the rule
     * @return std::basic_ostream<_CharT, _Traits>& Returns the stream
     */
    template <class _CharT, class _Traits>
    inline std::basic_ostream<_CharT, _Traits> &
    hrule(std::basic_ostream<_CharT, _Traits> &stream) { // NOLINT
        stream << "<hr>"
               << "\n";
        return stream;
    }
}; // namespace html

#endif // MODULEBASICS_INCLUDE_MODULEBASICS_HTML_H_
