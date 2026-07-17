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

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_REPORT_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_REPORT_H_

#include <moduleBasics/plot.h>
#include <moduleBasics/runtimeIO.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef BUILD_MODULEBASICS_SHARED
#ifdef _WIN32
#define MODULEBASICSDLLEXPORT __declspec(dllexport)
#else
#define MODULEBASICSDLLEXPORT __attribute__((visibility("default")))
#endif
#elif defined(IMPORT_MODULEBASICS_SHARED)
#ifdef _WIN32
#define MODULEBASICSDLLEXPORT __declspec(dllimport)
#endif
#else
#define MODULEBASICSDLLEXPORT
#endif

/**
 * \brief Report class for reporting
 *
 */
class MODULEBASICSDLLEXPORT Report {
 public:
  /**
   * \brief Construct a new Report object
   *
   * \param rtIO RuntimeIO object
   */
  explicit Report(const std::shared_ptr<RuntimeIO>& rtIO);

  /**
   * \brief Destroy the Report object
   *
   */
  ~Report() = default;

  /**
   * \brief Generate reports based on selections in rtIO
   *
   * \param name Name of the generated reports
   */
  void generateReports(const std::string& name = "");

  /**
   * \brief Get html reportstream
   *
   * \return std::stringstream& of html report
   */
  std::stringstream& htmlReportStream() { return htmlBody_; }

  /**
   * \brief Generate html report
   *
   * \param name Name of the generated reports
   */
  void generateHtmlReport(const std::string& name);

  /**
   * \brief Get tex reportstream
   *
   * \return std::stringstream& of tex report
   */
  std::stringstream& texReportStream() { return texBody_; }

  /**
   * \brief Generate tex report
   *
   * \param name Name of the generated reports
   */
  void generateTexReport(const std::string& name);

  /**
   * \brief Add Plots to Report (Legacy method)
   *
   * \param plot Shared pointer to plot object
   */
  void add_plot(const std::shared_ptr<Plot>& plot);

  /**
   * \brief Get the plot object by key
   *
   * \param key string for plot object
   * \return const std::shared_ptr<Plot>&
   */
  const std::shared_ptr<Plot>& get_plot(const std::string& key);

  /**
   * \brief Get the available plot keys object
   *
   * \return std::vector<const std::string&> vector
   */
  std::vector<std::string> get_available_plot_keys() const;

  /**
   * \brief Reportname constructed with modulename + aircraft model
   *
   * \return std::string returns the name of the report
   */
  std::string reportname() const;

 protected:
  std::shared_ptr<RuntimeIO> rtIO_;

 private:
  /**
   * \brief Generates html report header (fixed)
   *
   */
  void generateHtmlReportHeader_();

  /**
   * \brief Generates html report footer (fixed)
   *
   */
  void generateHtmlReportFooter_();

  /**
   * \brief Generates tex report header (fixed)
   *
   */
  void generateTexReportHeader_();

  /**
   * \brief Generates tex report footer (fixed)
   *
   */
  void generateTexReportFooter_();

  /**
   * \brief Generates stylesheet for html report (fixed)
   *
   */
  void generateStyleSheet_();

  std::stringstream htmlHeader_;
  std::stringstream htmlBody_;
  std::stringstream htmlFooter_;
  std::stringstream texHeader_;
  std::stringstream texBody_;
  std::stringstream texFooter_;
  std::string aircraftName_;

  std::map<std::string, const std::shared_ptr<Plot>&> plots_;
};

#endif  // MODULEBASICS_INCLUDE_MODULEBASICS_REPORT_H_
