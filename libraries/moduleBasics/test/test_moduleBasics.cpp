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

#include <aixml/endnode.h>
#include <aixml/node.h>
#include <gtest/gtest.h>
#include <moduleBasics/colors.h>
#include <moduleBasics/modeSelector.h>
#include <moduleBasics/module.h>

#include <fstream>
#include <memory>

class TestModule : public Module {
 public:
  explicit TestModule(const fs::path& rtConfigXML) : Module("test_tool", "1.0", rtConfigXML) {
  }
  void initialize() {
    myRuntimeInfo->info << "Initialize called" << std::endl;
  }
  void run() {
    myRuntimeInfo->info << "Run called" << std::endl;
  }
  void update() {
    myRuntimeInfo->info << "Update called" << std::endl;
  }
  void report() {
    myRuntimeInfo->info << "Report called" << std::endl;
  }
  void save() {
    myRuntimeInfo->info << "Save called" << std::endl;
  }
  template<typename ExceptionType = std::string>
  void throwing() {
    throwError<ExceptionType>(__FILE__,__func__,__LINE__, "Error message!");
  }
 private:
};

/* Setup test fixture */
class RuntimeIOTestFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    acxml = aixml::openDocument(acxmlAccess.string());
    config = aixml::openDocument(configAccess.string());
#ifdef _WIN32
    /* Create gnuplot and inkscape file */
    std::ofstream gnuplot(gnuplot_tmp);
    gnuplot.close();
    std::ofstream inkscape(inkscape_tmp);
    inkscape.close();
#endif
  }

#ifdef _WIN32
  void TearDown() override {
    fs::remove(gnuplot_tmp);
    fs::remove(inkscape_tmp);
  }
#endif

  /* Fixture variables */
  node acxml;
  node config;
  std::string programname = "test_moduleBasics";
  std::string toolversion = "1.0";
  modi consoleOn = 0;
  modi logOn = 0;
  bool plotOn = true;
  bool plotCopyOn = false;
  bool plotDeleteOn = false;
  bool reportOn = true;
  bool texOn = false;
  bool infoOn = false;
  int ownToolLevel = false;
  fs::path gnuAccess = "DEFAULT";
  fs::path inkAccess = "DEFAULT";
  fs::path logAccess = "default.log";
  fs::path acxmlAccess = "../../../moduleBasics/test/stubs/requirements_exchange_file.xml";
  fs::path configAccess = "../../../moduleBasics/test/stubs/control_settings_conf.xml";
#ifdef _WIN32
#include <string>
  fs::path gnuplot_tmp = fs::temp_directory_path().string() + "gnuplot";
  fs::path inkscape_tmp = fs::temp_directory_path().string() + "inkscape";
#endif
};

class ModuleTestFixture : public ::testing::Test {
 protected:
  void SetUp() override {
  }
  fs::path configAccess = "../../../moduleBasics/test/stubs/control_settings_conf.xml";
};

/* Testcases */

#ifdef _WIN32
TEST_F(RuntimeIOTestFixture, inkscape_and_gnuplot_available_WINDOWS) {
  gnuAccess = gnuplot_tmp.parent_path();
  inkAccess = inkscape_tmp.parent_path();

  std::shared_ptr<RuntimeIO> rtIO = std::make_shared<RuntimeIO>(programname, toolversion, consoleOn, logOn, plotOn, plotCopyOn, plotDeleteOn, reportOn, texOn, infoOn, ownToolLevel,
                                                                gnuAccess, inkAccess, "test.log", acxmlAccess, configAccess, acxml, config);
  EXPECT_NO_THROW(rtIO->make_executable_available(gnuAccess, "gnuplot"));
  EXPECT_NO_THROW(rtIO->make_executable_available(inkAccess, "inkscape"));
  EXPECT_TRUE(rtIO->is_executable_available("gnuplot"));
  EXPECT_TRUE(rtIO->is_executable_available("inkscape"));
}

#else
TEST_F(RuntimeIOTestFixture, inkscape_and_gnuplot_available_UNIX) {
  std::shared_ptr<RuntimeIO> rtIO = std::make_shared<RuntimeIO>(programname, toolversion, consoleOn, logOn, plotOn, plotCopyOn, plotDeleteOn, reportOn, texOn, infoOn, ownToolLevel,
                                                                gnuAccess, inkAccess, "test.log", acxmlAccess, configAccess, acxml, config);
  rtIO->make_executable_available("", "gnuplot");
  rtIO->make_executable_available("", "inkscape");
  EXPECT_TRUE(rtIO->is_executable_available("gnuplot"));
  EXPECT_TRUE(rtIO->is_executable_available("inkscape"));
}
#endif
TEST_F(RuntimeIOTestFixture, logfile_destination_and_name) {
  logAccess = fs::path("../../non-default.log");
  // Need to destruct rtIO element to call destructor which writes logfile and saves content
  {
    std::shared_ptr<RuntimeIO> rtIO = std::make_shared<RuntimeIO>(programname, toolversion, consoleOn, logOn, plotOn, plotCopyOn, plotDeleteOn, reportOn, texOn, infoOn,
                                                                  ownToolLevel, gnuAccess, inkAccess, logAccess, acxmlAccess, configAccess, acxml, config);
    myRuntimeInfo->out << rtIO->logAccess.string() << std::endl;
  }
  EXPECT_TRUE(fs::exists(logAccess));
}

/* module.h testcases*/

TEST_F(ModuleTestFixture, basicExecutionCorrect) {
  TestModule test_module(configAccess);

  EXPECT_EQ(test_module.execute(), 0);
}

TEST_F(ModuleTestFixture, basicExecutionNoThrow) {
  EXPECT_NO_THROW({ TestModule instance(configAccess); }); // cppcheck-suppress unreadVariable
}

TEST_F(ModuleTestFixture, basicExecutionThrowOnPurposeStdString) {
  EXPECT_THROW({
    TestModule instance(configAccess);
    instance.throwing<>();}, std::string);
}

TEST_F(ModuleTestFixture, basicExecutionThrowOnPurposeStdRuntimeError) {
  EXPECT_THROW({
    TestModule instance(configAccess);
    instance.throwing<std::runtime_error>();}, std::runtime_error);
}

TEST_F(ModuleTestFixture, basicExecutionThrow) {
  EXPECT_EXIT(
      { TestModule instance("./myfile.xml"); }, // cppcheck-suppress unreadVariable
      ::testing::ExitedWithCode(EXIT_FAILURE),
      ""); // empty string - since mock doesn't work properly currently (2024/03/26)
}


TEST(ModuleTestColors, predefinedColorTest) {
  /* Arange test */

  auto [r, g, b] = colors::rwth_blue;

  EXPECT_EQ(r, 1);
  EXPECT_EQ(g, 81);
  EXPECT_EQ(b, 157);
}

TEST(ModuleTestColors, nextColorTest) {
  EXPECT_TRUE(colors::next_color() == colors::ustutt_black);
  EXPECT_TRUE(colors::next_color() == colors::tum_blue);
  EXPECT_TRUE(colors::next_color() == colors::tub_red);
  EXPECT_TRUE(colors::next_color() == colors::tuhh_turquoise);
  EXPECT_TRUE(colors::next_color() == colors::tubs_red);
  EXPECT_TRUE(colors::next_color() == colors::rwth_blue);
  EXPECT_TRUE(colors::next_color() == colors::ustutt_black);
  colors::reset_next_color();
  EXPECT_TRUE(colors::next_color() == colors::ustutt_black);
}

TEST(ModuleTestColors, matplotColor) {
  auto [r, g, b] = colors::tuhh_turquoise;
  auto matplot_tuhh_turquoise = colors::to_matplot_color(colors::tuhh_turquoise);
  EXPECT_NEAR(0.0f, matplot_tuhh_turquoise[0], 1E-5);
  EXPECT_NEAR(r / 255., matplot_tuhh_turquoise[1], 1E-5);
  EXPECT_NEAR(g / 255., matplot_tuhh_turquoise[2], 1E-5);
  EXPECT_NEAR(b / 255., matplot_tuhh_turquoise[3], 1E-5);
}
