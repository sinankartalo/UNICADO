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

#ifndef MODULEBASICS_INCLUDE_MODULEBASICS_MODULE_H_
#define MODULEBASICS_INCLUDE_MODULEBASICS_MODULE_H_

#include <moduleBasics/runtimeIO.h>  // import runtime io class

#include <algorithm>   // import c++ standard library for using transform
#include <cstdlib>     // import c++ standard library for using cstdlib
#include <exception>   // import c++ standard library for using exception
#include <filesystem>  // import c++ standard library for using path
#include <iostream>    // import c++ standard library for using iostream
#include <map>         // import c++ standard library for using map
#include <memory>      // import c++ standard library for using memory-ops
#include <string>      // import c++ standard library for using string

/**
 * \brief Template class for modules at top level - must be inherited
 *
 */
class Module {
 public:
  /**
   * \brief Construct a new Module object
   *
   * \param toolName Tool Name string
   * \param toolVersion Tool Version string
   */
  Module(int argc, char* argv[], const std::string& toolName, const std::string& toolVersion)
      : Module(toolName, toolVersion, commandline(argc, argv, toolName)) {}

  /**
   * \brief Construct a new Module object
   *
   * \param toolName Tool Name string
   * \param toolVersion Tool Version string
   * \param rtConfigXML path to <toolName>_conf.xml
   */
  Module(const std::string& toolName, const std::string& toolVersion, const fs::path& rtConfigXML)
      : toolName_(toolName), toolVersion_(toolVersion) {
    try {
      return_code = init_runtime(rtConfigXML);
    } catch (const char* text) {
      std::cerr << text << std::endl;
      exit(EXIT_FAILURE);
    } catch (const std::string& text) {
      std::cerr << text << std::endl;
      exit(EXIT_FAILURE);
    } catch (const std::exception& e) {
      std::cerr << "The following C++ standard error (" <<  typeid(e).name() << ") has occured: " << e.what() << std::endl;
      exit(EXIT_FAILURE);
    } catch (...) {
      std::cerr << "Unknown error has occured. Exit" << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  /**
   * \brief Execute module
   *
   * \return int returncode (EXIT_SUCCESS or EXIT_FAILURE)
   */
  int execute() {
    if (return_code == EXIT_SUCCESS) {
      try {
        rtIO_->show_runtime();
        rtIO_->create_common_directories();

        initialize();
        run();
        update();
        report();
        save();
      } catch (const char* text) {
        myRuntimeInfo->err << text << std::endl;
        return_code = EXIT_FAILURE;
      } catch (const std::string& text) {
        myRuntimeInfo->err << text << std::endl;
        return_code = EXIT_FAILURE;
      } catch (const std::exception& e) {
        myRuntimeInfo->err << "The following C++ standard error (" << typeid(e).name() << ") has occured: " << e.what() << std::endl;
        return_code = EXIT_FAILURE;
      } catch (...) {
        myRuntimeInfo->err << "Unknown error has occured. Exit" << std::endl;
        return_code = EXIT_FAILURE;
      }
    }
    return return_code;
  }

  /* Pure virtual methods to be replaced by each module */
  /**
   * \brief Initialize operations (pure virtual) - on top level, set your next strategy here
   *
   */
  virtual void initialize() = 0;

  /**
   * \brief Run operations (pure virtual)
   *
   */
  virtual void run() = 0;

  /**
   * \brief Update operations (pure virtual)
   *
   */
  virtual void update() = 0;

  /**
   * \brief Report operations (pure virtual)
   *
   */
  virtual void report() = 0;

  /**
   * \brief Save operations (pure virtual) - not on acxml or moduleConfig node
   *
   */
  virtual void save() = 0;

  /**
   * \brief Get the Runtime I O object
   *
   * \return std::shared_ptr<RuntimeIO>
   */
  std::shared_ptr<RuntimeIO> get_RuntimeIO() { return rtIO_; }

 protected:
  std::shared_ptr<RuntimeIO> rtIO_;

 private:
  std::string toolName_;
  std::string toolVersion_;
  int return_code{EXIT_SUCCESS};

  /**
   * \brief Initialize RuntimeIO object + myRuntimeInfo
   *
   * \param rtConfigXML node to <toolName>_conf.xml
   * \return int returncode (EXIT_SUCCESS or EXIT_FAILURE)
   */
  int init_runtime(const fs::path& rtConfigXML) {
    std::string programname = toolName_;
    std::string toolVersion = toolVersion_;
    fs::path moduleConfAccess = rtConfigXML;

    /* Open config xml */
    const node& rtConfig = aixml::openDocument(rtConfigXML.string());

    modi consoleOutput =
        Modeselector(EndnodeReadOnly<std::string>("control_settings/console_output").read(rtConfig).value())
            .get_mode_numeric();
    modi logOutput = Modeselector(EndnodeReadOnly<std::string>("control_settings/log_file_output").read(rtConfig).value()).get_mode_numeric();
    bool plotOutput = EndnodeReadOnly<bool>("control_settings/plot_output/enable").read(rtConfig).value();
    bool plotCopy = EndnodeReadOnly<bool>("control_settings/plot_output/copy_plotting_files").read(rtConfig).value();
    bool plotDelete = EndnodeReadOnly<bool>("control_settings/plot_output/delete_plotting_files_from_tool_folder")
                          .read(rtConfig)
                          .value();
    bool reportOutput = EndnodeReadOnly<bool>("control_settings/report_output").read(rtConfig).value();
    bool texOutput = EndnodeReadOnly<bool>("control_settings/tex_report").read(rtConfig).value();
    bool infoOutput = EndnodeReadOnly<bool>("control_settings/write_info_files").read(rtConfig).value();
    int ownToolLevel = std::stoi(EndnodeReadOnly<std::string>("control_settings/own_tool_level").read(rtConfig).value());

    fs::path gnuAccess = EndnodeReadOnly<std::string>("control_settings/gnuplot_path").read(rtConfig).value();
    fs::path inkAccess = EndnodeReadOnly<std::string>("control_settings/inkscape_path").read(rtConfig).value();

    fs::path logAccess = EndnodeReadOnly<std::string>("control_settings/log_file").read(rtConfig).value();

    std::string ioDir = EndnodeReadOnly<std::string>("control_settings/aircraft_exchange_file_directory").read(rtConfig).value();
    std::string ioFile = EndnodeReadOnly<std::string>("control_settings/aircraft_exchange_file_name").read(rtConfig).value();
    fs::path acxmlAccess(ioDir + ioFile);

    if (!fs::exists(acxmlAccess)) {
      std::cerr << "Could not find aircraft exchange file - " << acxmlAccess.string() << " - Abort program!"
                << std::endl;
      exit(EXIT_FAILURE);
    }

    /* close config xml */
    aixml::closeDocument(rtConfig);

    rtIO_ = std::make_shared<RuntimeIO>(
        programname, toolVersion, consoleOutput, logOutput, plotOutput, plotCopy, plotDelete, reportOutput, texOutput,
        infoOutput, ownToolLevel, gnuAccess, inkAccess, logAccess, acxmlAccess, moduleConfAccess,
        aixml::openDocument(acxmlAccess.string()), aixml::openDocument(moduleConfAccess.string()));

    if (myRuntimeInfo == nullptr) {
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  /**
   * \brief Method to read commandlinearguments for constructor - if option "-c" is used,
   *        an additional path + filename must be given, otherwise the default path will be used
   *
   * \param argc argumentcount from main function
   * \param argv argumentvalues from main function
   * \param toolName Tool name given by module
   * \return fs::path for opening basic configuration file <tool name>_conf.xml
   */
  fs::path commandline(int argc, char* argv[], const std::string& toolName) {
    // Initialize path variable with default path
    fs::path path("./" + toolName + "_conf.xml");
    std::string tmp;
    int count = 0;
    while (count < argc) {
      tmp = argv[count++];
      if (tmp.compare("-c") == 0 && count < argc) {
        tmp = argv[count];
        path = fs::path(tmp);
      }
    }
    /* Check if path exists */
    if (!fs::exists(path)) {
      std::cerr << "Error: " << path.string() << " not found. Abort program!" << std::endl;
      exit(1);
    }
    return path;
  }
};

#endif  // MODULEBASICS_INCLUDE_MODULEBASICS_MODULE_H_
