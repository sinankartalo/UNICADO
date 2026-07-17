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

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_PLOT_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_PLOT_H_

#include <moduleBasics/runtimeIO.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <memory>
#include <map>
#include <string>

#ifdef BUILD_MODULEBASICS_SHARED
    #ifdef _WIN32
        #define MODULEBASICSDLLEXPORT __declspec(dllexport)
    #else
        #define MODULEBASICSDLLEXPORT __attribute__ ((visibility ("default")))
    #endif
#elif defined(IMPORT_MODULEBASICS_SHARED)
    #ifdef _WIN32
        #define MODULEBASICSDLLEXPORT __declspec(dllimport)
    #endif
#else
    #define MODULEBASICSDLLEXPORT
#endif

namespace fs = std::filesystem;


/**
 * \brief Plot class for single plot
 * 
 */
class MODULEBASICSDLLEXPORT Plot {
 public:
    /**
     * \brief Construct a new Plot object
     * 
     * \param rtIO RuntimeIO object
     * \param plotName name of string for plot
     */
    Plot(const std::shared_ptr<RuntimeIO>& rtIO, std::string plotName);

    /**
     * \brief Get the Plot Name object
     * 
     * \return std::string plotname
     */
    std::string getPlotName() { return plotName_; }

    /**
     * \brief Get the Svg Destination object
     * 
     * \return fs::path path object
     */
    fs::path getSvgDestination() { return svg_; }

    /**
     * \brief Get the Data Destination object
     * 
     * \return fs::path object
     */
    fs::path getDataDestination() { return data_; }

    /**
     * \brief Get the Script Destination object
     * 
     * \return fs::path object
     */
    fs::path getScriptDestination() { return script_; }

    /**
     * \brief Get the Data Stream object
     * 
     * \return std::stringstream& reference object to dataStream
     */
    std::stringstream& getDataStream() { return dataBody_; }

    /**
     * \brief Generate Plotdata at path
     * 
     * \param data fs::path object where to generate plotdata
     */
    void generatePlotData(fs::path data);

    /**
     * \brief Get the Script Stream object
     * 
     * \return std::stringstream& reference object to scriptStream
     */
    std::stringstream& getScriptStream() { return scriptBody_; }

    /**
     * \brief Generate ScriptStream
     * 
     * \param script fs::path object where to generate script
     * \param format format information
     * \param delimiter delimiter information
     * \param xtraLinStyles xtraLinStyles information
     * \param doubleSize doubleSize information
     * \param columnWidth columnWidth information
     */
    void generatePlotScript(fs::path script, const std::string format, std::string delimiter, const bool xtraLinStyles, const bool doubleSize, double columnWidth);

    /**
     * \brief call gnuplot on script
     * 
     * \param script path to plotscript 
     * \param additionalArguments additional arguments for gnuplot execution
     */
    void call_gnuplot(fs::path script, std::string additionalArguments);

 private:
    std::shared_ptr<RuntimeIO> rtIO_;
    const std::string plotName_;

    /**
     * \brief Generate plot script header
     * 
     * \param format format information
     * \param delimiter delimiter information
     * \param xtraLinStyles xtraLinStyles information
     * \param doubleSize doubleSize information
     * \param columnWidth columnWidth information
     */
    void generatePlotScriptHeader_(const std::string format, std::string delimiter, const bool xtraLinStyles, const bool doubleSize, double columnWidth);

    /**
     * \brief Converting plot from svg to pdf
     * 
     */
    void convert_plot_();

    fs::path data_;
    fs::path svg_;
    fs::path script_;

    std::stringstream dataBody_;
    std::stringstream scriptHeader_;
    std::stringstream scriptBody_;
    std::stringstream scriptFooter_;
};

#endif // MODULEBASICS_INCLUDE_MODULEBASICS_PLOT_H_
