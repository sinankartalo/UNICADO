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

#include <moduleBasics/report.h>
#include <runtimeInfo/runtimeInfo.h>

#include <format>

Report::Report(const std::shared_ptr<RuntimeIO>& rtIO) : rtIO_(rtIO) { aircraftName_ = rtIO_->aircraft_model(); }

void Report::generateReports(const std::string& name) {
  if (rtIO_->reportOn) {
    generateHtmlReport(name);
    if (rtIO_->texOn) {
      generateTexReport(name);
    }
  }
}

/*** HTML REPORT SECTION ***/
void Report::generateHtmlReport(const std::string& name) {
  std::ofstream report;
  if (name.empty()) {
    report.open(fs::path(rtIO_->getHtmlDir() + "/" + rtIO_->programname + "_report.html").string());
  } else {
    report.open(fs::path(rtIO_->getHtmlDir() + "/" + name + "_report.html").string());
  }
  generateStyleSheet_();
  generateHtmlReportHeader_();
  generateHtmlReportFooter_();
  if (report.is_open()) {
    report << htmlHeader_.str() << htmlBody_.str() << htmlFooter_.str();
    report.close();
  } else {
    throw("[ERROR] - Opening HTML-Reportfile failed. Abort program!");
  }
}

void Report::generateHtmlReportHeader_() {
  /* Header of html report. Do not change! */
  htmlHeader_
      << "<html lang=\"en\">\n"
      << "<head>\n"
      << "<meta content=\"width=device-width, initial-scale=1.0\" name=\"viewport\"/>\n"
      << "<meta charset=\"utf-8\"/>\n"
      << "<title>" << reportname() << "</title>\n"
      << "<link href=\"style.css\" rel=\"stylesheet\"/>\n"
      << "</head>\n"
      /* End of header */
      /* Begin of body */
      << "<body>\n"
      << "<div class=\"logo\"></div>\n"
      << "<div class=\"content\">\n"
      << "<h1>Report - " << reportname()  << "<br/>" << "\n"
      << "<span>" << "<font size=\"2\">All outputs of the program '" << rtIO_->programname << "' were created with version "
      << rtIO_->toolVersion << "</span></h1>\n"
      << "<!-- Begin CeRAS Report -->\n"
      << "<!-- ################################################################################################### "
         "-->\n"
      << "<div class=\"container\">\n";
}

void Report::generateHtmlReportFooter_() {
  /* End of body */
  htmlFooter_ << "</div>"
              << "</div>"
              << "</div>"
              << "</body>"
              << std::endl
              /* Footer of html report. Do not change! */
              << "</html>\r";
}

/*** TEX REPORT SECTION ***/
void Report::generateTexReport(const std::string& name) {
  std::ofstream report;
  if (name.empty()) {
    report.open(fs::path(rtIO_->getTexDir() + "/" + rtIO_->programname + "_report.tex").string());
  } else {
    report.open(fs::path(rtIO_->getTexDir() + "/" + name + "_report.tex").string());
  }
  generateTexReportHeader_();
  generateTexReportFooter_();
  if (report.is_open()) {
    report << texHeader_.str() << texBody_.str() << texFooter_.str();
    report.close();
  } else {
    throw("[ERROR] - Opening TeX-Reportfile failed. Abort program!");
  }
}

void Report::generateTexReportHeader_() { texHeader_ << "" << std::endl; }

void Report::generateTexReportFooter_() { texFooter_ << "" << std::endl; }

/*** PLOT SECTION ***/

void Report::add_plot(const std::shared_ptr<Plot>& plot) { plots_.insert(std::make_pair(plot->getPlotName(), plot)); }

/**
 * \brief Report name
 *
 * \return std::string Name of the report from programname (modulename + aircraft model)
 */
const std::shared_ptr<Plot>& Report::get_plot(const std::string& key) {
  if (plots_.find(key) != plots_.end()) {
    return plots_.at(key);
  } else {
    throw std::invalid_argument(std::format("Key {} not found", key));
  }
}

std::vector<std::string> Report::get_available_plot_keys() const {
  std::vector<std::string> keys;
  for (auto& item : plots_) {
    keys.push_back(item.first);
  }
  return keys;
}

std::string Report::reportname() const {
  std::string name;
  bool new_word = true;

  for (char ch : rtIO_->programname) {
    if (ch == '_') {
      name += ' ';
      new_word = true;
    } else if (std::isupper(ch)) {
      if (!name.empty() && !new_word) {
        name += ' ';
      }
      name += ch;
      new_word = false;
    } else {
      if (new_word) {
        name += std::toupper(ch);
        new_word = false;
      } else {
        name += ch;
      }
    }
  }
  name += " of " + rtIO_->aircraft_model();
  return name;
}

void Report::generateStyleSheet_() {
  std::ofstream css_file(fs::path(rtIO_->getHtmlDir() + "/style.css").string());

  if (css_file.is_open()) {
    css_file << R"(
*, *:before, *:after {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}
html, body {
  background-color: #2a2d33;
  font-family: system-ui, sans-serif;
  font-size: 1.3rem;
  color: #2c3e50;
  line-height: 1.3;
  background-size: cover;
}

table {
  margin-left: 0;
  margin-right: auto;
}

.logo {
  background-image: url('logoUNICADO_icon.svg');
  height: 75vh;
  width: 100%;
  opacity: 0.2;
  filter: grayscale(100%);
  background-size: contain;
  background-repeat: no-repeat;
  background-position: center center;
  position: relative center;

}

.content {
  position: absolute;
  width: 98%;
  height: 85%;
  top: 0rem;
  left: 1rem;
  padding-left: 0rem;
  box-sizing: border-box;
  min-width: fit-content;

}

.content h1 {
      color: #ecf0f1;
      margin: 1rem 0.1rem;
      font-size: 2.7rem;
      font-weight: lighter;
      font-variant: small-caps;
}

.container {
  position: absolute;
  display: flex;
  width: 100%;
  height: 100%;
}


.box {
  width: 50%;
  overflow-y: auto;
  overflow-x: auto;
  scrollbar-width: none;
  scrollbar-color: #f0f0f0 #f0f0f0;
  padding: 0;
  margin-right: 0.2rem;
  background-color: #ecf0f1;
  box-shadow: 5px 5px 5px black;
  border-radius: 0.5rem;
  min-width: fit-content;
  text-align: center;
}


.data {
  background-color: #ecf0f1;
  padding: auto;
  min-width: fit-content;
  max-width: 50%;
}

.plot {
  background-color: #ecf0f1;
  text-align: center;
  max-width: 50%;
}

.box h2 {
  margin-left: 0.5rem;
  font-weight: normal;
  font-variant: small-caps;
  font-size: 1.5rem;
  text-align: left;
}

.box h3 {
  margin-left: 0.5rem;
  font-weight: normal;
  font-variant: small-caps;
  font-size: 1.2rem;
  text-align: left;
}

.box h4 {
  margin-left: 0.5rem;
  font-weight: normal;
  font-variant: small-caps;
  font-size: 1.0rem;
  text-align: left;
}

.box ul {
  list-style-type: disc;
  margin-left: 0rem;
  padding-left: 2rem;
  text-align: left;
}

.box ul li {
  margin: 0 0 0 0;
  padding-left: 0;
  font-size: 0.9rem;
  font-weight: lighter;
}

.content-table {
  margin-left: 2rem;
  padding: 0.50rem;
  font-size:0.9rem;
  flex-basis: 100rem;
  font-weight: lighter;
  padding-bottom: 2rem;
}

.content-table caption {
  text-align: left;
  font-weight:600;
  font-size: larger;
  font-variant: small-caps;
}

.content-table th {
  text-align: left;
  padding-right: 1rem;
  white-space: nowrap;
  min-width: 3rem;
  font-weight: normal;
}

.content-table td {
  padding-right: 1rem;
  text-align: left;
}

.content-table td.row-header {
  font-weight: normal;
  text-align: left;
  padding-right: 0.5rem;
}

.content-table td.content-data-number {
  text-align: right;
}

.content-table tbody tr:hover {
  color: #ee5253;
  cursor: pointer;
}

.plot img.image-plot {
  display: block;
  margin: 0px auto;
  min-width: none;
  width: 90%;
  color: #ecf0f1;
  width: 90%;
  max-width: 800px;
  height: auto;
  text-align: center;
}
)";
    css_file.close();
    myRuntimeInfo->out << "CSS code written to style.css successfully." << std::endl;
  } else {
    myRuntimeInfo->err << "Unable to open style.css." << std::endl;
  }
}
