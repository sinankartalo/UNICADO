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

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_SVG_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_SVG_H_

/* === Includes === */
#include <ranges>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <tuple>

/* === Functions === */
namespace svg {
/**
 * @brief Represents a 2D coordinate
 *
 */
struct Coordinate {
    double x{0.0}; /** [m] The x coordinate */
    double y{0.0}; /** [m] The y coordinate */

    /**
     * @brief Add a coordinate to another coordinate
     * 
     * @param other The other coordinate to add
     * @return Coordinate The sum of the two coordinates
     */
    auto operator+(const Coordinate &other) const -> Coordinate {
        return {this->x + other.x, this->y + other.y};
    }

    /**
     * @brief Subtract a coordinate from another coordinate
     * 
     * @param other The other coordinate to subtract
     * @return Coordinate The difference of the two coordinates
     */
    auto operator-(const Coordinate &other) const -> Coordinate {
        return {this->x - other.x, this->y - other.y};
    }

    /**
     * @brief Multiply a coordinate with a scalar
     * 
     * @param scalar The scalar to multiply with
     * @return Coordinate The scaled coordinate
     */
    auto operator*(const double scalar) const -> Coordinate {
        return {this->x * scalar, this->y * scalar};
    }

    /**
     * @brief Divide a coordinate by a scalar
     * 
     * @param scalar The scalar to divide by
     * @return Coordinate The scaled coordinate
     */
    auto operator/(const double scalar) const -> Coordinate {
        return {this->x / scalar, this->y / scalar};
    }
};

/**
 * @brief Represents a 2D size
 */
struct Size {
    double width{0.0};  /** [m] The width */
    double height{0.0}; /** [m] The height */

    /**
     * @brief Add a size to another size
     * 
     * @param other The other size to add
     * @return Size The sum of the two sizes
     */
    auto operator+(const Size &other) const -> Size {
        return {this->width + other.width, this->height + other.height};
    }

    /**
     * @brief Subtract a size from another size
     * 
     * @param other The other size to subtract
     * @return Size The difference of the two sizes
     */
    auto operator-(const Size &other) const -> Size {
        return {this->width - other.width, this->height - other.height};
    }

    /**
     * @brief Multiply a size with a scalar
     * 
     * @param scalar The scalar to multiply with
     * @return Size The scaled size
     */
    auto operator*(const double scalar) const -> Size {
        return {this->width * scalar, this->height * scalar};
    }

    /**
     * @brief Divide a size by a scalar
     * 
     * @param scalar The scalar to divide by
     * @return Size The scaled size
     */
    auto operator/(const double scalar) const -> Size {
        return {this->width / scalar, this->height / scalar};
    }
};

/* === Operator* overloads for double === */
inline auto operator*(const double scalar, const Coordinate &coord) -> Coordinate {
    return coord * scalar;
};

inline auto operator*(const double scalar, const Size &size) -> Size {
    return size * scalar;
};

namespace Colors {
/**
 * @brief The color type
 */
using Color_t = std::tuple<uint8_t, uint8_t, uint8_t>;

/* Predefined colors */
constexpr Color_t None{0, 0, 0}; /** Special construct to catch a none color */
constexpr Color_t Black{1, 1, 1};
constexpr Color_t White{255, 255, 255};
constexpr Color_t Red{255, 0, 0};
constexpr Color_t Green{0, 255, 0};
constexpr Color_t Blue{0, 0, 255};

/**
 * @brief Get the string representation of a color
 *
 * @param color The color to convert
 * @return std::string The hex string representation of the color
 */
inline auto to_string(const Color_t color) -> std::string {
    /* Catch the none color construct */
    if (color == None) {
        return "none";
    } else if (color == Black) {
        return "#000000";
    }

    /* Convert all other colors to a hex string */
    std::stringstream ss;
    const int red = std::get<0>(color);
    const int green = std::get<1>(color);
    const int blue = std::get<2>(color);
    ss << "#"
       << std::hex << std::setfill('0') << std::setw(2) << red
       << std::hex << std::setfill('0') << std::setw(2) << green
       << std::hex << std::setfill('0') << std::setw(2) << blue;
    return ss.str();
};
}; // namespace Colors

/**
 * @brief The base class for all SVG objects.
 * @details This class defines the base properties of all SVG objects.
 * It is used as the abstract interface for the svg output stream 
 * to add objects to the stream.
 */
class Object {
 public:
    /**
     * @brief Default constructor
     */
    Object() = default;

    /**
     * @brief Construct a new SVG object with all its base properties.
     * 
     * @param color The stroke color of the object.
     * @param fill_color The fill color of the object.
     * @param stroke_width The stroke width of the object.
     */
    Object(const Colors::Color_t &color, const Colors::Color_t &fill_color, const double stroke_width)
        : color{color}, fill_color{fill_color}, stroke_width{stroke_width} { }

    /**
     * @brief Construct a new SVG object with a stroke and fill color.
     * The stroke width is set to 1.0.
     * 
     * @param color The stroke color of the object.
     * @param fill_color The fill color of the object.
     */
    Object(const Colors::Color_t &color, const Colors::Color_t &fill_color)
        : Object(color, fill_color, 1.0) { }

    /**
     * @brief Construct a new SVG object with a stroke color.
     * The fill color is set to none and the stroke width is set to 1.0.
     * 
     * @param color The stroke color of the object.
     */
    explicit Object(const Colors::Color_t &color)
        : Object(color, Colors::None, 1.0) { }

    /**
     * @brief Virtual destructor as this is a base class.
     * 
     */
    virtual ~Object() = default;

    /**
     * @brief Output the object to the stream.
     * @attention The scale factor is automatically
     * calculated by the SVG ostream.
     *
     * @param stream The stream to write to.
     * @param scale How the object coordinates should be scaled.
     */
    virtual void to_stream(std::ostream &stream, const double scale) const = 0;

    /**
     * @brief Get the style string of the object
     * 
     * @param custom A custom style string to insert into the style
     * @return std::string The style string
     */
    auto style(const std::string_view custom = "") const -> std::string {
        std::stringstream ss;
        ss << "style=\""
           << "fill:" << Colors::to_string(this->fill_color) << ";"
           << "stroke:" << Colors::to_string(this->color) << ";"
           << "stroke-width:" << std::setprecision(2) << this->stroke_width <<";"
           << custom
           << "\"";
        return ss.str();
    }

 private:
    Colors::Color_t color{Colors::Black}; /** The stroke color of the object */
    Colors::Color_t fill_color{Colors::None}; /** The fill color of the object */
    double stroke_width{1.0}; /** The stroke width of the object */
};

/**
 * @brief A line SVG object.
 *
 */
class Line : public Object {
 public:
    /**
     * @brief Construct a new Line object in black.
     *
     * @param start The start coordinate of the line.
     * @param end The end coordinate of the line.
     */
    Line(const Coordinate &start, const Coordinate &end)
        : Object{Colors::Black}, start{start}, end{end} { }

    /** 
     * @brief Construct a new Line object with a color.
     * 
     * @param start The start coordinate of the line.
     * @param end The end coordinate of the line.
     * @param color The stroke color of the line.
     */
    Line(const Coordinate &start, const Coordinate &end, const Colors::Color_t &color)
        : Object{color}, start{start}, end{end} { }

    /**
     * @brief Output the line to the stream.
     *
     * @param stream The stream to write to.
     * @param scale How the line coordinates should be scaled.
     */
    void to_stream(std::ostream &stream, const double scale) const override {
        stream << "<line "
               << "x1=\"" << scale * this->start.x << "\" "
               << "y1=\"" << scale * this->start.y << "\" "
               << "x2=\"" << scale * this->end.x << "\" "
               << "y2=\"" << scale * this->end.y << "\" "
               << this->style()
               << "/>\n";
    }

 private:
    Coordinate start;      /** [m] The start coordinate */
    Coordinate end;        /** [m] The end coordinate */
};

/**
 * @brief A text SVG object
 *
 */
class Text : public Object {
 public:
    /**
     * @brief Construct a new Text object with all custom properties
     * @note The text is centered around the position.
     * 
     * @param position The position of the text.
     * @param text The text to display.
     * @param color The color of the text. Default is black.
     * @param font_size The font size of the text. Default is 12.
     * @param rotation The rotation of the text. Default is 0.
     */
    Text(
        const Coordinate &position,
        const std::string_view &text,
        const Colors::Color_t &color = Colors::Black,
        const int font_size = 12,
        const double rotation = 0.0)
        : Object{Colors::None, color}, position{position}, text{text}, font_size{font_size}, rotation{rotation} { }

    /**
     * @brief Output the text to the stream.
     *
     * @param stream The stream to write to.
     * @param scale How the text coordinates should be scaled.
     */
    void to_stream(std::ostream &stream, const double scale) const override {
        /* Set the additional text attributes */
        std::stringstream attributes;
        attributes << "font-size:" << this->font_size << "px;";

        /* Add the transform attribute to the text */
        std::stringstream transform;
        transform << std::setprecision(4)
            << "transform=\"translate(" << scale * this->position.x << "," << scale * this->position.y << ")";
        if (this->rotation != 0) {
            transform << " rotate(" << this->rotation << ")";
        }
        transform << "\" ";

        /* Write the text to the stream */
        stream << "<text "
               << transform.str()
               << "text-anchor=\"middle\" text-align=\"center\" "
               << this->style(attributes.str())
               << ">"
               << this->text
               << "</text>\n";
    }

 private:
    Coordinate position;   /** [m] The position of the text */
    std::string_view text; /** The text */
    int font_size;      /** The font size */
    double rotation;   /** The rotation of the text */
};

/**
 * @brief A ellipse SVG object.
 *
 */
class Ellipse : public Object {
 public:
    /**
     * @brief Construct a new Ellipse object:
     *
     * @param center The center of the ellipse.
     * @param size The width and height of the ellipse.
     * @param fill_color The fill color of the ellipse. Default is none.
     */
    Ellipse(const Coordinate &center, const Size &size, const Colors::Color_t &fill_color = Colors::None)
        : Object{Colors::Black, fill_color}, center{center}, size{size} { }

    /**
     * @brief Output the ellipse to the stream.
     *
     * @param stream The stream to write to.
     * @param scale How the ellipse coordinates should be scaled.
     */
    void to_stream(std::ostream &stream, const double scale) const override {
        stream << "<ellipse "
               << "cx=\"" << scale * this->center.x << "\" "
               << "cy=\"" << scale * this->center.y << "\" "
               << "rx=\"" << scale * this->size.height / 2.0 << "\" "
               << "ry=\"" << scale * this->size.width / 2.0 << "\" "
               << this->style()
               << "/>\n";
    }

 private:
    Coordinate center;          /** [m] The center of the ellipse */
    Size size;                  /** [m] The width and height of the ellipse */
};

/**
 * @brief A polygon SVG object.
 * @note The PointRange can be any container whose elements have
 * the x() and y() member functions.
 *
 * @tparam PointRange The range of points that define the polygon.
 */
template <std::ranges::input_range PointRange>
class Polygon : public Object {
 public:
    /**
     * @brief Construct a new Polygon object.
     *
     * @param points The point range that defines the polygon.
     * @param fill_color The fill color of the polygon. Default is none.
     */
    Polygon(const PointRange &points, const Colors::Color_t &fill_color = Colors::None) // NOLINT
        : Object{Colors::Black, fill_color}, points{points} { }

    /**
     * @brief Output the polygon to the stream.
     *
     * @param stream The stream to write to.
     * @param scale How the polygon coordinates should be scaled.
     */
    void to_stream(std::ostream &stream, const double scale) const override {
        stream << "<polygon "
               << "points=\"";
        for (const auto &point : this->points) {
            stream << scale * point.x() << "," << scale * point.y() << " ";
        }
        stream << "\" "
               << this->style()
               << "/>\n";
    }

 private:
    PointRange points;          /** The points that define the polygon */
};

/**
 * @brief A horizontal dimension SVG object
 * @note This dimension is always horizontal and the
 * dimension value is derived from `size.width` of the size property.
 * The `size.height` (positive or negative ) defines the vertical offset
 * of the dimension line from the given start coordinate.
 * The start coordinate is the left end of the dimension line.
 * 
 */
class HorizontalDimension : public Object {
 public:
    /**
     * @brief Construct a new Horizontal Dimension object.
     * 
     * @param start_left The start coordinate on the left of the dimension line.
     * @param size The size (width, height) of the dimension line.
     */
    HorizontalDimension(const Coordinate &start_left, const Size &size) {
        /* Get the dimension coordinates */
        this->start = start_left;
        this->end = {start_left.x + size.width, start_left.y};
        this->offset = size.height;

        /* Format the dimension text */
        std::stringstream ss;
        ss << std::setprecision(4) << size.width << " m";
        this->text = ss.str();
    }

    /**
     * @brief Output the dimension to the stream.
     *
     * @param stream The stream to write to.
     * @param scale How the dimension coordinates should be scaled.
     */
    void to_stream(std::ostream &stream, const double scale) const override {
        /* Get the dimension lines */
        const double aux_extension = this->offset > 0 ? 0.2 : -0.2;
        Line auxiliary_right{this->start, {this->start.x, this->start.y + this->offset + aux_extension}};
        Line auxiliary_left{this->end, {this->end.x, this->end.y + this->offset + aux_extension}};
        Line dimension_line{{this->start.x, this->start.y + this->offset}, {this->end.x, this->end.y + this->offset}};

        /* Get the point for the text center */
        const double text_x = (this->start.x + this->end.x) / 2.0;
        /* The text center refers to the bottom of the text,
         * therefore the top and bottom offsets differ.
         */
        const double text_y = this->offset > 0 ?
            this->start.y + this->offset + 0.4 :
            this->start.y + this->offset - 0.1;

        /* Draw everything */
        auxiliary_right.to_stream(stream, scale);
        auxiliary_left.to_stream(stream, scale);
        dimension_line.to_stream(stream, scale);
        Text({text_x, text_y}, this->text, Colors::Black, 8).to_stream(stream, scale);
    }

 private:
    Coordinate start; /** The start coordinate of the dimension */
    Coordinate end; /** The end coordinate of the dimension */
    std::string text; /** Dimension value */
    double offset; /** The vertical offset of the dimension line */
};

/**
 * @brief A vertical dimension SVG object
 * @note This dimension is always vertical and the
 * dimension value is derived from `size.height` of the size property.
 * The `size.width` (positive or negative ) defines the horizontal offset
 * of the dimension line from the given start coordinate.
 * The start coordinate is the top end of the dimension line.
 * 
 */
class VerticalDimension : public Object {
 public:
    /**
     * @brief Construct a new Vertical Dimension object.
     * 
     * @param start_top The start coordinate of the topside of the dimension line.
     * @param size The size (width, height) of the dimension line.
     */
    VerticalDimension(const Coordinate &start_top, const Size &size) {
        /* Get the dimension coordinates */
        this->start = start_top;
        this->end = {start_top.x, start_top.y + size.height};
        this->offset = size.width;

        /* Format the dimension text */
        std::stringstream ss;
        ss << std::setprecision(4) << size.height << " m";
        this->text = ss.str();
    }

    /**
     * @brief Output the dimension to the stream.
     *
     * @param stream The stream to write to.
     * @param scale How the dimension coordinates should be scaled.
     */
    void to_stream(std::ostream &stream, const double scale) const override {
        /* Get the dimension lines */
        const double aux_extension = this->offset > 0 ? 0.2 : -0.2;
        Line auxiliary_top{this->start, {this->start.x + this->offset + aux_extension, this->start.y}};
        Line auxiliary_bottom{this->end, {this->end.x + this->offset + aux_extension, this->end.y}};
        Line dimension_line{{this->start.x + this->offset, this->start.y}, {this->end.x+ this->offset, this->end.y}};

        /* Get the point for the text center */
        const double text_y = (this->start.y + this->end.y) / 2.0;
        const double text_x = this->start.x + this->offset - 0.1; // Dimension text always left

        /* Draw everything */
        auxiliary_top.to_stream(stream, scale);
        auxiliary_bottom.to_stream(stream, scale);
        dimension_line.to_stream(stream, scale);
        Text({text_x, text_y}, this->text, Colors::Black, 8, -90.0).to_stream(stream, scale);
    }

 private:
    Coordinate start; /** The start coordinate of the dimension */
    Coordinate end; /** The end coordinate of the dimension */
    std::string text; /** Dimension value */
    double offset; /** The horizontal offset of the dimension line */
};

/**
 * @brief A SVG output file stream
 * This automatically starts the SVG header and ends it when closed.
 * Otherwise it behaves like a normal output stream.
 * @details All the SVG objects assume [m] as their units. This class
 * automatically scales the objects to the desired pixel/m ratio.
 */
class ofstream : public std::ofstream {
 public:
    /* No default constructor */
    ofstream() = delete;

    /**
     * @brief Construct a new SVG ofstream object-
     *
     * @param path The path to the file to write to.
     * @param size The size of the SVG in [m].
     * @param pixel_per_m How the SVG should be scaled [pixel/m]. Default is 10.
     */
    ofstream(const std::filesystem::path &path, const Size &size, const int pixel_per_m = 10)
        : std::ofstream{path}, size_{size}, dots_per_m_{pixel_per_m}
    {
        /* Start the header */
        *this << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
              << "<svg\n"
              << "xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
              << "xmlns=\"http://www.w3.org/2000/svg\"\n"
              << "version=\"1.0\"\n"
              << "width=\"" << size.width * this->dots_per_m_ << "\"\n"
              << "height=\"" << size.height * this->dots_per_m_ << "\"\n"
              << "id=\"svg3234\">\n";
    }

    /* Default constructors */
    ofstream(const ofstream &) = default;
    ofstream(ofstream &&) = default;
    auto operator=(const ofstream &) -> ofstream & = default;
    auto operator=(ofstream &&) -> ofstream & = default;

    /**
     * @brief Destroy the SVG ofstream object.
     */
    ~ofstream() override = default;

    /**
     * @brief Add an object to the stream.
     *
     * @param object The object to add.
     * @return ostream& The stream itself.
     */
    auto operator<<(const Object &object) -> ofstream & {
        object.to_stream(*this, this->dots_per_m_ / 1.0);
        return *this;
    }

    /**
     * @brief Close the SVG stream.
     * @details This adds the closing tag to the stream and closes the file.
     */
    void close() {
        *this << "</svg>\n";
        std::ofstream::close();
    }

    /**
     * @brief Get the center coordinate of the SVG.
     * 
     * @return Coordinate [m] The center coordinate of the SVG.
     */
    auto center() const -> Coordinate {
        return {this->size_.width / 2.0, this->size_.height / 2.0};
    }

 private:
    Size size_; /** [m] The size of the SVG */
    int dots_per_m_; // The dots per meter, which defines the scaling of the SVG
};

}; // namespace svg

#endif // MODULEBASICS_INCLUDE_MODULEBASICS_SVG_H_
