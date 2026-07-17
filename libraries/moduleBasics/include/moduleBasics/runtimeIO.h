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

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_RUNTIMEIO_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_RUNTIMEIO_H_

#include <aixml/endnode.h>
#include <aixml/node.h>
#include <moduleBasics/modeSelector.h>
#include <runtimeInfo/runtimeInfo.h>
#include <standardFiles/functions.h>

#include <format>
#include <algorithm>
#include <filesystem> // NOLINT
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <cstdlib>


#ifdef _WIN32
#include <windows.h>
#endif

/* Namespace definition */
namespace fs = std::filesystem;

/**
 * \brief   RuntimeIO - Class for handling IO elements
 *          stores access points (paths)
 *          stores nodes for acxml and moduleconfig
 *          checks and creates basic directories
 *          creates directories
 */
class RuntimeIO {
 public:
  /**
   * \brief Construct a new Runtime I O object
   *
   * \param _programname
   * \param _toolVersion
   * \param _consoleOn true/false
   * \param _logOn true/false
   * \param _plotOn true/false
   * \param _plotCopyOn true/false
   * \param _plotDeleteOn true/false
   * \param _reportOn true/false
   * \param _texOn true/false
   * \param _infoOn true/false
   * \param _ownToolLevel
   * \param _gnuAccess path
   * \param _inkAccess path
   * \param _logAccess path
   * \param _acxmlAccess path
   * \param _moduleConfAccess path
   * \param _acxml node
   * \param _moduleConfig node
   */
  RuntimeIO(const std::string& _programname, const std::string& _toolVersion, modi _consoleOn, modi _logOn,
            bool _plotOn, bool _plotCopyOn, bool _plotDeleteOn, bool _reportOn, bool _texOn, bool _infoOn,
            int _ownToolLevel, fs::path _gnuAccess, fs::path _inkAccess, fs::path _logAccess, fs::path _acxmlAccess,
            fs::path _moduleConfAccess, node& _acxml, const node& _moduleConfig)  // NOLINT
      : programname(_programname),
        toolVersion(_toolVersion),
        consoleOn(_consoleOn),
        logOn(_logOn),
        plotOn(_plotOn),
        plotCopyOn(_plotCopyOn),
        plotDeleteOn(_plotDeleteOn),
        reportOn(_reportOn),
        texOn(_texOn),
        infoOn(_infoOn),
        ownToolLevel(_ownToolLevel),
        gnuAccess(_gnuAccess),
        inkAccess(_inkAccess),
        logAccess(_logAccess),
        acxmlAccess(_acxmlAccess),
        moduleConfAccess(_moduleConfAccess),
        acxml(_acxml),
        moduleConfig(_moduleConfig) {
    myRuntimeInfo = new runtimeInfo(consoleOn, logOn, logAccess.string(), programname);
    set_directory_names();
    read_aircraft_information();
    if (plotOn) {
      make_executable_available(gnuAccess, "gnuplot");
      if (texOn) {
        make_executable_available(inkAccess, "inkscape");
      }
    }
  }

  /**
   * \brief Destroy the Runtime I O object
   *
   */
  ~RuntimeIO() {
    /* Generating info files for in and output */
    if (infoOn) {
      generate_io_info();
      generate_io_gui_info();
      generate_input_xml();
      generate_output_xml();
    }

    /* Copy plot files from tool directories to io plot directories */
    if (plotCopyOn && plotOn) {
      myRuntimeInfo->debug << "Copy plotting files from " << getCsvFilesDirTool() << " to " << getCsvFilesDir()
                           << std::endl;
      copyFiles(getCsvFilesDirTool(), getCsvFilesDir());
      myRuntimeInfo->debug << "Copy plotting files from " << getPlotFilesDirTool() << " to " << getPlotFilesDir()
                           << std::endl;
      copyFiles(getPlotFilesDirTool(), getPlotFilesDir());
    }

    /* Delete plotting tool directories */
    if (plotDeleteOn && plotOn) {
      myRuntimeInfo->debug << "Delete directory " << getCsvFilesDirTool() << std::endl;
      deleteDirectory(getCsvFilesDirTool());
      myRuntimeInfo->debug << "Delete directory " << getPlotFilesDirTool() << std::endl;
      deleteDirectory(getPlotFilesDirTool());
    }
    delete myRuntimeInfo;
  }

  /**
   * \brief Show runtimeinfo informations from <...>_conf.xml
   *
   */
  void show_runtime() {
    myRuntimeInfo->out << "[MODULE RUNTIMEINFO] - " << programname << "" << std::endl;
    myRuntimeInfo->out << "   [CONSOLE  ] - " << (consoleOn > 0 ? "[ON]" : "[OFF]") << std::endl;
    myRuntimeInfo->out << "   [LOG      ] - " << (logOn > 0 ? "[ON]" : "[OFF]") << std::endl;
    myRuntimeInfo->out << "   [PLOT     ] - " << (plotOn > 0 ? "[ON]" : "[OFF]") << std::endl;
    myRuntimeInfo->out << "      [COPY  ] - " << (plotCopyOn > 0 ? "[ON]" : "[OFF]") << std::endl;
    myRuntimeInfo->out << "      [DELETE] - " << (plotCopyOn > 0 ? "[ON]" : "[OFF]") << std::endl;
    myRuntimeInfo->out << "   [REPORT   ] - " << (reportOn > 0 ? "[ON]" : "[OFF]") << std::endl;
    myRuntimeInfo->out << "   [TEX      ] - " << (texOn > 0 ? "[ON]" : "[OFF]") << std::endl;
    myRuntimeInfo->out << "   [INFOFILES] - " << (infoOn > 0 ? "[ON]" : "[OFF]") << std::endl;
    myRuntimeInfo->out << "   [GNUPLOT]" << std::endl;
    myRuntimeInfo->out << "      [PATH    ] - " << (gnuAccess.parent_path().string()) << std::endl;
    myRuntimeInfo->out << "      [FILENAME] - " << (gnuAccess.filename().string()) << std::endl;
    myRuntimeInfo->out << "   [INKSCAPE]" << std::endl;
    myRuntimeInfo->out << "      [PATH    ] - " << (inkAccess.parent_path().string()) << std::endl;
    myRuntimeInfo->out << "      [FILENAME] - " << (inkAccess.filename().string()) << std::endl;
    myRuntimeInfo->out << "   [LOGFILE]" << std::endl;
    myRuntimeInfo->out << "      [PATH    ] - " << (logAccess.parent_path().string()) << std::endl;
    myRuntimeInfo->out << "      [FILENAME] - " << (logAccess.filename().string()) << std::endl;
    myRuntimeInfo->out << "   [IO/ACXML]" << std::endl;
    myRuntimeInfo->out << "      [PATH    ] - " << (acxmlAccess.parent_path().string()) << std::endl;
    myRuntimeInfo->out << "      [FILENAME] - " << (acxmlAccess.filename().string()) << std::endl;
    myRuntimeInfo->out << "   [MODCONFIG]" << std::endl;
    myRuntimeInfo->out << "      [PATH    ] - " << (moduleConfAccess.parent_path().string()) << std::endl;
    myRuntimeInfo->out << "      [FILENAME] - " << (moduleConfAccess.filename().string()) << std::endl;
  }

  /**
   * \brief Plots directories with by key values
   *
   */
  void show_directories() {
    for (auto const& directory : directories) {
      myRuntimeInfo->out << "[" << directory.first << "] - " << directory.second << std::endl;
    }
  }

  /* Get strings from directories */
  std::string getIODir() { return get_directory(std::string("ACXML")).parent_path().string(); }
  std::string getGeometryDir() { return get_directory(std::string("GEOMETRY")).parent_path().string(); }
  std::string getAirfoilDataDir() { return get_directory(std::string("AIRFOILDATA")).parent_path().string(); }
  std::string getAeroDataDir() { return get_directory(std::string("AERODATA")).parent_path().string(); }
  std::string getEngineDataDir() { return get_directory(std::string("ENGINEDATA")).parent_path().string(); }
  std::string getMissionDataDir() { return get_directory(std::string("MISSIONDATA")).parent_path().string(); }
  std::string getReportDir() { return get_directory(std::string("REPORT")).parent_path().string(); }
  std::string getTexDir() { return get_directory(std::string("TEX")).parent_path().string(); }
  std::string getHtmlDir() { return get_directory(std::string("HTML")).parent_path().string(); }
  std::string getXmlDir() { return get_directory(std::string("XML")).parent_path().string(); }
  std::string getPlotDir() { return get_directory(std::string("PLOT")).parent_path().string(); }
  std::string getCsvFilesDir() { return get_directory(std::string("CSVFILES")).parent_path().string(); }
  std::string getCsvFilesDirTool() { return get_directory(std::string("CSVFILESTOOL")).parent_path().string(); }
  std::string getPlotFilesDir() { return get_directory(std::string("PLOTFILES")).parent_path().string(); }
  std::string getPlotFilesDirTool() { return get_directory(std::string("PLOTFILESTOOL")).parent_path().string(); }
  std::string getLogFilesDir() { return get_directory(std::string("LOGFILES")).parent_path().string(); }

  /**
   * \brief Create a Geometry dir if not existent
   *
   */
  void createGeometryDir() { create_directory("GEOMETRY"); }

  /**
   * \brief Create a AirfoilData dir if not existent
   *
   */
  void createAirfoilDataDir() { create_directory("AIRFOILDATA"); }

  /**
   * \brief Create a Aero Data dir if not existent
   *
   */
  void createAeroDataDir() { create_directory("AERODATA"); }

  /**
   * \brief Create a Engine Data dir if not existent
   *
   */
  void createEngineDataDir() { create_directory("ENGINEDATA"); }

  /**
   * \brief Create a Mission Data dir if not existent
   *
   */
  void createMissionDataDir() { create_directory("MISSIONDATA"); }

  /**
   * \brief Check if file exists by directory string and file string
   *
   * \param dir directory where file should be
   * \param file file to check for
   * \return true
   * \return false
   */
  bool checkFileExistence(const std::string& dir, const std::string& file) {
    return fs::exists(fs::path(dir).append(file));
  }

  /**
   * \brief Check if file exists by path
   *
   * \param path path with file included
   * \return true
   * \return false
   */
  bool checkFileExistence(fs::path path) { return fs::exists(path); }

  /**
   * \brief Check whether gnuplot is found in the system
   *
   * \return true successful gnuplot call
   * \return false failed gnuplot call
   */
  bool is_executable_available(const std::string& executable) {
    bool is_available = false;
#ifdef _WIN32
    is_available = std::system(std::format("where \"{}\" >NUL 2>&1", executable).c_str()) == 0;
#else
    is_available = std::system(std::format("which \"{}\" >/dev/null 2>&1", executable).c_str()) == 0;
#endif
    myRuntimeInfo->debug << std::format("{} ... {}", executable, is_available ? "is available" : "is not available")
                         << std::endl;
    return is_available;
  }

  void make_executable_available(const fs::path& executable_directory_path, const std::string& executable) {
    // Check if executable is already available?
    if (is_executable_available(executable)) {
      return;
    }

#ifdef _WIN32
    // Get a mutable copy of the path to make preferred
    auto preferred_executable{executable_directory_path};
    std::string path = preferred_executable.string();

    // Retrieve current environment variable
    constexpr DWORD buffer_size = 32767;
    WCHAR path_buffer[buffer_size]; // NOLINT

    // Read current environment variable
    DWORD path_size = GetEnvironmentVariableW(L"Path", path_buffer, buffer_size);
    if (path_size == 0) {
      throwError(__FILE__, __func__, __LINE__, "Could not retrieve Environmentvariable!\n");
    }
    std::wstring current_path(path_buffer, path_size);

    // Append new path to current path
    std::wstring wide_path = current_path + L";" + std::wstring(path.begin(), path.end());
    // Add executable path to local path variable - only current execution scope
    if (!SetEnvironmentVariableW(L"Path", wide_path.c_str())) {
      myRuntimeInfo->err << std::format("Failed to set environment variable for {}", executable) << std::endl;
    }
    // Check again if executable can be found - if not -> wrong
    if (!is_executable_available(executable)) {
      throwError(__FILE__, __func__, __LINE__, std::format("Could not find path to {} at path {}!", executable, path));
    }
    return;
#else
    /* Reaches throw on non windows machines - installatino of gnuplot or inkscape via package manager mandatory on unix systems */
    throwError(__FILE__, __func__, __LINE__, std::format("Unix users are supposed to have {} installed via package manager!", executable));
#endif
  }

  /**
   * \brief Create a common directories
   *
   */
  void create_common_directories() {
    check_and_create_directory("REPORT", reportOn || texOn || plotOn || logOn);
    check_and_create_directory("TEX", texOn || plotOn || logOn);
    check_and_create_directory("HTML", reportOn);
    check_and_create_directory("PLOT", plotOn);
    check_and_create_directory("CSVFILES", plotOn);
    check_and_create_directory("CSVFILESTOOL", plotOn);
    check_and_create_directory("PLOTFILES", plotOn);
    check_and_create_directory("PLOTFILESTOOL", plotOn);
    check_and_create_directory("LOGFILES", logOn);
  }

  /**
   * \brief Adding directory to directories
   *
   * \param key Key for directory - will be transformed to uppercase letters
   * \param path std::string path
   */
  void add_directory(const std::string& key, const std::string& path) {
    fs::path tmp(path + "/");
    add_directory(key, tmp);
  }

  /**
   * \brief Adding directory to directories
   *
   * \param key Key for directory - will be transformed to uppercase letters
   * \param newDir fs::path path
   */
  void add_directory(std::string key, fs::path newDir) {
    std::transform(key.cbegin(), key.cend(), key.begin(), [](unsigned char c) { return std::toupper(c); });
    if (directories.find(key) != directories.end()) {
      myRuntimeInfo->out << "Key already exist! No further operations" << std::endl;
      return;
    }
    myRuntimeInfo->out << "Add directory with key [" << key << "]" << newDir.parent_path().string() << std::endl;
    directories.insert(std::make_pair(key, newDir));
    create_directory(key);
  }

  /**
   * \brief Reopen the Aircraft XML File assigned to acxml node
   *
   */
  void reopenAcXML() { acxml = aixml::openDocument(acxmlAccess.string()); }

  /**
   * \brief Saves Aircraft XML File with ownToolLevel (see rtIO variable)
   *
   */
  void saveAcXML() { aixml::saveDocument(acxml, ownToolLevel); }

  /**
   * \brief Closes Aircraft XML File with ownToolLevel (see rtIO variable)
   *
   */
  void closeAcXML() { aixml::closeDocument(acxml); }

  /**
   * \brief Saves and closes Aircraft XML File with ownToolLevel (see rtIO variable)
   *
   */
  void saveAndCloseAcXML() {
    saveAcXML();
    closeAcXML();
  }

  /**
   * \brief Saves ModuleConfig XML File
   *
   */
  void saveModuleConfig() { aixml::saveDocument(moduleConfig); }

  /**
   * \brief Closes ModuleConfig XML File
   *
   */
  void closeModuleConfig() { aixml::closeDocument(moduleConfig); }

  /**
   * \brief Saves and closes ModuleConfig XML File
   *
   */
  void saveAndCloseModuleConfig() {
    saveModuleConfig();
    closeModuleConfig();
  }

  /**
   * \brief Returns aircraft type
   *
   * \return std::string&
   */
  [[nodiscard]] const std::string& aircraft_type() const { return aircraft_type_; }

  /**
   * \brief Returns aircraft model
   *
   * \return std::string&
   */
  [[nodiscard]] const std::string& aircraft_model() const { return aircraft_model_; }

  /**
   * \brief Returns aircraft configuration type
   *
   * \return std::string&
   */
  [[nodiscard]] const std::string& aircraft_configuration_type() const { return aircraft_configuration_type_; }

  /**
   *  \brief Returns wether aerodynamic technologies are integrated
   *
   *  \return std::string&
   */
  [[nodiscard]] const std::string& aerodynamic_technologies() const {return aerodynamic_technologies_; }

  /**
   * Returns aircraft energy carrier type
   *
   * \return  std::string&     possible energy carriers - if multiple carriers in use -> hybrid
   */
  [[nodiscard]] const std::string& aircraft_energy_carrier_type() const { return aircraft_energy_carrier_type_; }

  /**
   * \brief Return map of used energy carriers and occurence
   * \return std::map<int, std::tuple<std::string, double, double, double>>& energy carrier ID, tuple(fuel type, density, volumetric energy density, gravimetric energy density)
   */
  [[nodiscard]] const std::map<int, std::tuple<std::string, double>>& aircraft_energy_carriers() const { return aircraft_energy_carriers_; }

  /**
   * Returns map of propulsor parents and number of propulsors mounted to this parent
   *
   * \return  std::map<std::string, int>& map of propulsor parents including number of mounts per parent
   */
  [[nodiscard]] const std::map<std::string, int>& aircraft_propulsor_parents() const { return aircraft_propulsor_parents_; }


  /**
   * \brief Returns the fuel name of a given energy carrier ID from the aircraft XML
   * \param energy_carrier_id: energy carrier ID of the fuel
   * \return std::string for the density name
   */
  [[nodiscard]] const std::string& get_fuel_type(int energy_carrier_id) const {
    check_energy_carrier_id_(energy_carrier_id);
    return std::get<0>(aircraft_energy_carriers_.at(energy_carrier_id));
  }

  /**
   * \brief Returns the fuel density of a given energy carrier ID from the aircraft XML
   * \param energy_carrier_id: energy carrier ID of the fuel
   * \return double value of the density [kg/m^3]
   */
  [[nodiscard]] double get_fuel_density(int energy_carrier_id) const {
    check_energy_carrier_id_(energy_carrier_id);
    return std::get<1>(aircraft_energy_carriers_.at(energy_carrier_id));
  }

 public:
  const std::string programname;
  const std::string toolVersion;
  const modi consoleOn;
  const modi logOn;
  mutable bool plotOn;
  bool plotCopyOn;
  bool plotDeleteOn;
  const bool reportOn;
  const bool texOn;
  const bool infoOn;
  const int ownToolLevel;
  const fs::path gnuAccess;
  const fs::path inkAccess;
  const fs::path logAccess;
  const fs::path acxmlAccess;
  const fs::path moduleConfAccess;
  node& acxml;
  const node& moduleConfig;

 private:
  std::map<std::string, fs::path> directories;
  std::string aircraft_type_;
  std::string aircraft_model_;
  std::string aircraft_configuration_type_;
  std::string aerodynamic_technologies_;
  std::string aircraft_energy_carrier_type_;
  std::map<int, std::tuple<std::string, double>> aircraft_energy_carriers_;
  std::map<std::string, int> aircraft_propulsor_parents_;

  /**
   * \brief Set common Directory Names
   *
   */
  void set_directory_names() {
    directories.insert(std::make_pair("ACXML", acxmlAccess));
    directories.insert(std::make_pair("GEOMETRY", fs::path(getIODir() + "/geometry_data/")));
    directories.insert(std::make_pair("AIRFOILDATA", fs::path(getGeometryDir() + "/airfoil_data/")));
    directories.insert(std::make_pair("AERODATA", fs::path(getIODir() + "/aero_data/")));
    directories.insert(std::make_pair("ENGINEDATA", fs::path(getIODir() + "/engine_data/")));
    directories.insert(std::make_pair("MISSIONDATA", fs::path(getIODir() + "/mission_data/")));
    directories.insert(std::make_pair("REPORT", fs::path(getIODir() + "/reporting/")));
    directories.insert(std::make_pair("TEX", fs::path(getReportDir() + "/report_tex/")));
    directories.insert(std::make_pair("HTML", fs::path(getReportDir() + "/report_html/")));
    directories.insert(std::make_pair("XML", fs::path(getReportDir() + "/report_xml/")));
    directories.insert(std::make_pair("PLOT", fs::path(getReportDir() + "/plots/")));
    directories.insert(std::make_pair("CSVFILES", fs::path(getPlotDir() + "/csv_files/")));
    directories.insert(std::make_pair("CSVFILESTOOL", fs::path("./csv_files/")));
    directories.insert(std::make_pair("PLOTFILES", fs::path(getPlotDir() + "/plt_files/")));
    directories.insert(std::make_pair("PLOTFILESTOOL", fs::path("./plt_files/")));
    directories.insert(std::make_pair("LOGFILES", fs::path(getReportDir() + "/log_files/")));
  }

  /**
   * \brief Creates a directory if not existent
   *
   * \param key directories map key value
   * \return true
   * \return false
   */
  bool create_directory(const std::string& key) { return check_and_create_directory(key, true); }

  /**
   * \brief Checks if directory exist
   *
   * \param key directories map key value
   * \return true
   * \return false
   */
  bool check_directory(const std::string& key) {
    myRuntimeInfo->out << "Checking directory... [" << key << "]" << std::endl;
    return !fs::exists(get_directory(key));
  }

  /**
   * \brief Checks and creates a directory if not existent. Do only if flag is true otherwise do nothing and return
   * false
   *
   * \param key directories map key value
   * \param flags Do check and create
   * \return true
   * \return false
   */
  bool check_and_create_directory(const std::string& key, bool flags) {
    if (!flags) {
      return false;
    }
    if (check_directory(key)) {
      myRuntimeInfo->out << "Creating directory... [" << key << "]" << std::endl;
      return fs::create_directories(get_directory(key));
    }
    return true;
  }

  /**
   * \brief Get the directory fs::path value from selected key
   *
   * \param key directories map key value
   * \return fs::path directory path
   */
  fs::path get_directory(const std::string& key) { return directories[key]; }

  /**
   * \brief Generates IO XML Files based on global node element in node class at each read/write operation
   *
   * \param variablesVector Vector with list of read or written elements using the node class
   * \param direction "input" or "output"
   */
  void generate_io_xml_file(std::vector<std::string> variablesVector, const std::string& direction) {
    std::ofstream plotStream;
    std::string plotDataFile;
    std::string filePostfix;
    if (direction == "input") {
      filePostfix = "_inputs.xml";
    } else if (direction == "output") {
      filePostfix = "_outputs.xml";
    }
    plotDataFile = programname + filePostfix;
    plotStream.open(plotDataFile.c_str());
    std::string programInputTag("ExchangeData");
    plotStream << "<" << programInputTag << "></" << programInputTag << ">";
    plotStream.close();
    const node& inXML = aixml::openDocument(programname + filePostfix);
    std::string xmlFile = variablesVector.at(0).substr(0, variablesVector.at(0).rfind(".xml/")) + ".xml";
    node currentInputNode = aixml::openDocument(xmlFile);
    for (uint32_t i(0); i < variablesVector.size(); i++) {
      std::string polarFileString = "_polar.xml";  // Special case: skip _polar xml file
      std::size_t found =
          (variablesVector.at(i).substr(0, variablesVector.at(i).rfind(".xml/")) + ".xml").find(polarFileString);
      if (found != std::string::npos) {
        continue;
      }
      // If the read file is the same as from the previous iteration, use old node. Otherwise open new node.
      if (xmlFile != variablesVector.at(i).substr(0, variablesVector.at(i).rfind(".xml/")) + ".xml") {
        xmlFile = variablesVector.at(i).substr(0, variablesVector.at(i).rfind(".xml/")) + ".xml";
        myRuntimeInfo->debug << ".. read " << xmlFile << std::endl;
        currentInputNode = aixml::openDocument(xmlFile);
      }
      std::string ithXmlFile = (variablesVector.at(i).substr(0, variablesVector.at(i).find(".xml/")));
      ithXmlFile = ithXmlFile + ".xml";
      std::string resPath = variablesVector.at(i).substr(variablesVector.at(i).find(ithXmlFile) + ithXmlFile.size(),
                                                         variablesVector.at(i).size());
      std::vector<std::string> tmpElements;
      std::vector<std::string> tmpPathElements = tokenize(resPath, "/");
      for (uint16_t k(0); k < tmpPathElements.size(); k++) {
        tmpElements.push_back(tmpPathElements.at(k));
      }
      std::vector<node> elementNodes;
      std::string accumulatedPath = "";
      std::vector<std::string> accumulatedPaths;
      std::string currentElement;
      std::string parentElement = programInputTag;
      for (uint16_t j = 0; j < tmpElements.size(); j++) {
        std::string accumulatedPath_old = accumulatedPath;
        accumulatedPath = accumulatedPath + "/" + tmpElements.at(j);
        /*TODO(aigner#1#05/10/21): CAUTION! The following lines are only a quick fix.
        Tools that change the aircraft XML file before the toolOutput, could cause problems, because initially read xml
        elements could have been deleted.*/
        if (!currentInputNode.find(accumulatedPath, 1)) {
          std::vector<std::string> tmpElement_tokenized = tokenize(tmpElements.at(j), "@");
          std::string elementName = tmpElement_tokenized.at(0);
          node& childNode = currentInputNode.at(accumulatedPath_old).appendChild(elementName, true);
          if (tmpElement_tokenized.size() > 1) {
            std::vector<std::string> attribute_tokenized = tokenize(tmpElement_tokenized.at(1), "=");
            std::string attributeName = attribute_tokenized.at(0);
            std::string attributeValue = attribute_tokenized.at(1);
            childNode.addAttrib(attributeName, attributeValue);
          }
        }
        elementNodes.push_back(currentInputNode.at(accumulatedPath));
        elementNodes.back().deleteChildren();
        std::map<std::string, std::string> currentAttribMap = elementNodes.back().getAttribMap();
        std::map<std::string, std::string>::iterator it;
        for (it = currentAttribMap.begin(); it != currentAttribMap.end(); ++it) {
          if (!compareStrings(it->first, "ID", true) && !compareStrings(it->first, "uID", true) &&
              !compareStrings(it->first, "Name", true) && !compareStrings(it->first, "name", true)) {
            elementNodes.back().deleteAttrib(it->first);
          } else {
            myRuntimeInfo->debug << "... Keeping " << it->first << ": " << it->second << std::endl;
          }
        }
        if (currentAttribMap.count("Name")) {
          accumulatedPath = accumulatedPath + "@Name=" + currentAttribMap.at("Name");
          myRuntimeInfo->debug << "... Found Name tag at: " << accumulatedPath << std::endl;
        }
        accumulatedPaths.push_back(accumulatedPath);
        currentElement = programInputTag + accumulatedPaths.at(j);
        if (!inXML.find(currentElement, 1) && inXML.find(parentElement, 1)) {
          if (j == (tmpElements.size() - 1)) {
            inXML.at(parentElement).appendChild(elementNodes.at(j));
            parentElement = programInputTag;
          } else {
            inXML.at(parentElement).appendChild(elementNodes.at(j));
            parentElement = currentElement;
          }
        } else {
          parentElement = currentElement;
        }
      }
    }
    aixml::saveDocument(inXML);
    aixml::closeDocument(inXML);
  }

  /**
   * \brief Generates info file <programname>_IOinfo.txt with information on read/written elements from node class
   *
   */
  void generate_io_info() {
    myRuntimeInfo->out << "Generating IO XML access file for module: " << programname << " in "
                       << programname + "_IOinfo.txt ..." << std::endl;
    std::ofstream ioInfo(programname + "_IOinfo.txt", std::ofstream::out);
    if (ioInfo.is_open()) {
      ioInfo << "Input:" << std::endl;
      ioInfo << "==================================================================================" << std::endl;
      for (size_t i = 0; i < node::input.size(); i++) {
        ioInfo << node::input.at(i) << std::endl;
      }
      ioInfo << std::endl;
      ioInfo << "Output:" << std::endl;
      ioInfo << "==================================================================================" << std::endl;
      for (size_t i = 0; i < node::output.size(); i++) {
        ioInfo << node::output.at(i) << std::endl;
      }
      ioInfo.close();
    } else {
      myRuntimeInfo->err << "Could not open: " << programname << "_IOinfo.txt! File not written." << std::endl;
    }
  }

  /**
   * \brief Generate IO GUI Info in xml readable format for input and output from node class
   *
   */
  void generate_io_gui_info() {
    myRuntimeInfo->out << "Generating IO XML access file for module: " << programname << " in "
                       << programname + "_guiSettings.xml ..." << std::endl;
    std::ofstream ioGUIInfo(programname + "_guiSettings.xml", std::ofstream::out);
    if (ioGUIInfo.is_open()) {
      ioGUIInfo << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>" << std::endl
                << "<!DOCTYPE ModulSettings []>" << std::endl
                << "<ModulSettings>" << std::endl
                << "    <TapSetup>" << std::endl
                << "        <FrontPage TapName=\"Beschreibung\"><![CDATA[" << programname << "]]></FrontPage>"
                << std::endl
                << "        <Plot TapName=\"Report\"><![CDATA["
                << "reportHTML" << FILESEPERATOR << programname << "_report.html]]></Plot>" << std::endl
                << "        <Values TapName=\"Werte\">" << std::endl;
      for (size_t i(0); i < node::input.size(); i++) {
        ioGUIInfo << "            <Input>" << node::input.at(i) << "</Input>" << std::endl;
      }
      for (size_t i(0); i < node::output.size(); i++) {
        ioGUIInfo << "            <Output>" << node::output.at(i) << "</Output>" << std::endl;
      }
      ioGUIInfo << "        </Values>" << std::endl << "    </TapSetup>" << std::endl << "</ModulSettings>";
      ioGUIInfo.close();
    } else {
      myRuntimeInfo->err << "Could not open: " << programname << "_guiSettings.xml! File not written." << std::endl;
    }
  }

  /**
   * \brief Generates IO XML Files based on global node element in node class at each read/write operation
   *
   * \param direction "input" or "output"
   */
  void generate_io_xml(const std::string& direction) {
    myRuntimeInfo->out << "Writing " << direction << "-Files ..." << std::endl;
    try {
      if (direction.compare("input") == 0 && node::input.size() > 0) {
        generate_io_xml_file(node::input, direction);
      } else if (direction.compare("output") == 0 && node::output.size() > 0) {
        generate_io_xml_file(node::output, direction);
      }
    } catch (std::exception& ex) {
      std::exception_ptr p = std::current_exception();
      myRuntimeInfo->err << ex.what() << std::endl;  // (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
      myRuntimeInfo->warn << "No _" << direction << ".xml file will be written." << std::endl;
    }
  }

  /**
   * \brief Generates Input XML file based on node::input in node class
   *
   */
  void generate_input_xml() { generate_io_xml("input"); }

  /**
   * \brief Generates Input XML file based on node::output in node class
   *
   */
  void generate_output_xml() { generate_io_xml("output"); }

  /**
   * \brief Check if the given energy carrier ID can be found within the aircraft xml
   * \param energy_carrier_id: energy carrier ID of the fuel
   */
  void check_energy_carrier_id_(int energy_carrier_id) const {
    if (aircraft_energy_carriers_.find(energy_carrier_id) != aircraft_energy_carriers_.end()) {
        return;
    } else {
        throwError(__FILE__, __func__, __LINE__, std::format("Given energy carrier ID {} cannot be found in the aircraft xml!", energy_carrier_id));
    }
  }

  /**
   * \brief Read aicraft information from aircraft_exchange_file
   *
   */
  void read_aircraft_information() {
    aircraft_type_ =
        static_cast<std::string>(acxml.at("aircraft_exchange_file/requirements_and_specifications/general/type/value"));
    aircraft_model_ = static_cast<std::string>(
        acxml.at("aircraft_exchange_file/requirements_and_specifications/general/model/value"));
    aircraft_configuration_type_ = static_cast<std::string>(acxml.at(
        "aircraft_exchange_file/requirements_and_specifications/design_specification/configuration/configuration_type/value"));

    // Read aerodynamic_technologies
    aerodynamic_technologies_ = static_cast<std::string>(acxml.at(
        "aircraft_exchange_file/requirements_and_specifications/design_specification/configuration/aerodynamic_technologies/"
        "value"));

    // Read energy carrier information
    std::vector<node*> energy_carriers = acxml.getVector(
        "aircraft_exchange_file/requirements_and_specifications/design_specification/energy_carriers/energy_carrier", 1);

    for (auto ec : energy_carriers) {
      // Add energy carriers to aircraft energy carrier list
      aircraft_energy_carriers_[ec->getIntAttrib("ID")] = std::tuple(static_cast<std::string>(ec->at("type/value")),
                                                                     static_cast<double>(ec->at("density/value")));
    }

    // Read propulsion information from acxml
    std::vector<node*> propulsors = acxml.getVector(
        "aircraft_exchange_file/requirements_and_specifications/design_specification/propulsion/propulsor", 1);

    for (auto propulsor : propulsors) {
      // List propulsor parents and count
      aircraft_propulsor_parents_[static_cast<std::string>(propulsor->at("position/parent_component/value"))]++;
    }

    // If aircraft energy carriers have more than one carrier -> set energy carrier type to hybrid - else set to first
    // item
    if (aircraft_energy_carriers_.size() > 1) {
      aircraft_energy_carrier_type_ = "hybrid";
    } else {
      aircraft_energy_carrier_type_ = std::get<0>(aircraft_energy_carriers_[0]);
    }
  }
};

#endif  // MODULEBASICS_INCLUDE_MODULEBASICS_RUNTIMEIO_H_
