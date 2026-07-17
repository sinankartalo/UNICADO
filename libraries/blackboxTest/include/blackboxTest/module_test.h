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


/**
 * @brief The blackbox test structure for the module.
 * @details
 * This is a header only file that provides the blackbox test structure for the module.
 * You just have to provide a main function which call the run_module_tests function
 * with your module type as template parameter. Since all modules derive from
 * the moduleBasics Module class, this framework know how to execute them.
 *
 * Since this module class does not allow to directly inject XML files, this framework
 * creates a temporary working directory and copies the original XML files there.
 * Those files are modified for each test case and the module is executed with the
 * modified files. The expectations are then checked against the modified files.
 * After all tests are run, the temporary working directory is deleted.
 *
 * A possible implementation for your module could look like this:
 * @code {.cpp}
 * // Includes
 * #include "path-to/toolinfo.h"
 * #include "path-to/your-module.h"
 * #include <blackboxTest/module_test.h>
 *
 * // Main to execute the blackbox tests
 * int main(int argc, char *argv[]) {
 *    return run_module_tests<YourModule>(argc, argv);
 * }
 *
 * @endcode
 *
 * The test cases are defined in a XML file, which is expected to be called *test_cases.xml* and
 * to be located in the *test* folder of your module.
 * The structure of the XML file is as follows:
 * @code {.xml}
 * <?xml version="1.0" encoding="UTF-8"?>
 * <test_suites>
 *   <test_suite name="aircraft-name">
 *       <test name="test-name">
 *           <parameter>
 *               <path>can be a path starting with 'aircraft_exchange_file' or 'module_configuration_file'</path>
 *               <value>new-value</value>
 *           </parameter>
 *           <expect type="EQ">
 *               <path>aircraft_exchange_file/path/to/check</path>
 *               <value>expected-value</value>
 *           </expect>
 *       </test>
 *    </test_suite>
 * </test_suites>
 * @endcode
 *
 * You can have as many test suites, tests and expectations as you like, BUT there can only be one
 * parameter per test case! After all, the tests are meant to check the expected behavior of the module
 * when this input parameter is varied. The expectations are checked against the modified aircraft file.
 * You have to provide multiple test cases if you want to check multiple parameters.
 * The general convention is to use one test suite per aircraft and one test per parameter. At least
 * this framework assumes, that the base aircraft is the same for each test within one test suite.
 *
 * @attention Make sure that "toolinfo.h" is included BEFORE this file!
 */
#ifndef BLACKBOXTEST_INCLUDE_BLACKBOXTEST_MODULE_TEST_H_
#define BLACKBOXTEST_INCLUDE_BLACKBOXTEST_MODULE_TEST_H_

/* === Includes === */
#include <aixml/node.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <moduleBasics/module.h>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

/* Define the expected folder structure and filenames */
constexpr std::string_view test_case_file = "test/test_cases.xml";         /**< Where to find the test case file. */
constexpr std::string_view temp_folder_name = "unicado_module_test_";      /**< The name of the temporary working directory. */
constexpr std::string_view aircraft_original = "aircraft_original.xml";    /**< The original aircraft file. */
constexpr std::string_view aircraft_modified = "aircraft_modified.xml";    /**< The modified aircraft file. */
constexpr std::string_view config_original = "configuration_original.xml"; /**< The original configuration file. */
constexpr std::string_view config_modified = "configuration_modified.xml"; /**< The modified configuration file. */

/* Define the some behavioral options */
constexpr std::string_view console_mode = "mode_0"; /**< Override the console output mode with this mode. */

/** Define the folder structure needed for a aircraft */
constexpr std::array folder_structure = {
    "aero_data",
    "airfoil_data",
    "engine_data",
    "geometry_data"};

/* === Functions === */
/**
 * @brief Template function to apply an expectation to a value.
 *
 * @tparam ValueType The type of the value to compare.
 * @tparam CompareFunc The type of the comparison function.
 * @param path The node path of the current value which is tested.
 * @param expected The expected value.
 * @param actual The actual value.
 * @param compare The comparison function to use for the expectation.
 * @param compare_name The name of the comparison function.
 */
template <typename ValueType, typename CompareFunc>
void apply_expectation(
    const std::string_view &path,
    const ValueType &expected,
    const ValueType &actual,
    const CompareFunc &compare,
    const std::string_view &compare_name)
{
    /* Check whether the expectation is met */
    if (!compare(expected, actual))
    {
        /* Print the error message when the expectation is not met */
        GTEST_NONFATAL_FAILURE_("**********************************")
            << "Expected:\n"
            << "Path: " << path << "\n"
            << "to be " << compare_name << ": " << expected << "\n"
            << "but it was: " << actual << "\n"
            << "**********************************";
    }
}

/**
 * @brief Check whether the data in the aircraft XML meets the expectation.
 *
 * @param aircraft The aircraft XML data.
 * @param expectation The expectation to check.
 */
void check_expectation(const std::shared_ptr<node> aircraft, const node *expectation)
{
    /* Get the node path to check */
    const std::string path(expectation->at("path"));

    /* Get the expectation to apply => as lower case to be more robust */
    const std::string type(expectation->getStringAttrib("type"));
    std::string type_lower_case{type};
    std::transform(type.begin(), type.end(), type_lower_case.begin(), ::tolower);

    /* Get the comparison function and name of the expectation */
    const double expected = expectation->at("value");
    const double actual = aircraft->at(path);
    if (type_lower_case == "eq")
    {
        apply_expectation(path, expected, actual, std::greater<double>{}, "equal to");
    }
    else if (type_lower_case == "lt")
    {
        apply_expectation(path, expected, actual, std::less<double>{}, "less than");
    }
    else if (type_lower_case == "le")
    {
        apply_expectation(path, expected, actual, std::less_equal<double>{}, "less than or equal to");
    }
    else if (type_lower_case == "gt")
    {
        apply_expectation(path, expected, actual, std::greater<double>{}, "greater than");
    }
    else if (type_lower_case == "ge")
    {
        apply_expectation(path, expected, actual, std::greater_equal<double>{}, "greater than or equal to");
    }
    else if (type_lower_case == "near")
    {
        /* Use a lambda to provide the NEAR comparison */
        auto compare = [expectation, &path](double expected, double actual) -> bool
        {
            /* Get the defined accuracy */
            double accuracy{1e-6};
            if (expectation->hasAttrib("accuracy"))
            {
                accuracy = expectation->getDoubleAttrib("accuracy");
            }
            else if (expectation->hasAttrib("relative"))
            {
                accuracy = expectation->getDoubleAttrib("relative");
                if (std::abs(expected) > 1e-6) {
                    accuracy *= expected;
                } else {
                    std::cout << "Warning: Expected value is close to zero."
                    "=> Using absolute accuracy for the following test:\n"
                    << path << " => near " << expected << std::endl;
                }
            }
            else
            {
                std::cout << "Warning: No 'accuracy' or 'relative' attribute given for 'NEAR' expectation."
                    "=> Assuming 1e-6 as accuracy for the following test:\n"
                    << path << " => near " << expected << std::endl;
            }

            /* Check the expectation */
            return std::abs(expected - actual) < std::abs(accuracy);
        };
        apply_expectation(path, expected, actual, compare, "near");
    }
    else if (type_lower_case == "streq")
    {
        apply_expectation(
            path,
            static_cast<std::string>(expectation->at("value")),
            static_cast<std::string>(aircraft->at(path)),
            std::equal_to<std::string>{},
            "equal to");
    }
    else
    {
        throw std::invalid_argument("Unknown expectation type: " + std::string{type});
    }
}

/* === Classes === */
/**
 * @brief Module fixture base class for all module tests.
 * This class provides the temporary working directory and the standard paths for the
 * original and modified aircraft and configuration files.
 */
class ModuleFixture : public ::testing::Test
{
  public:
    /**
     * @brief Set the Up Test Suite by creating the temporary working
     * directory and copying the original files.
     */
    static void SetUpTestSuite()
    {
        /* Create the temporary working directory for this test suite */
        ModuleFixture::working_directory = std::filesystem::temp_directory_path() / (std::string{temp_folder_name} + TOOL_NAME);
        std::filesystem::create_directory(ModuleFixture::working_directory);

        /* Populate the standard paths for all tests of this test suite */
        ModuleFixture::aircraft_path = ModuleFixture::working_directory / aircraft_original;
        ModuleFixture::aircraft_under_test = ModuleFixture::working_directory / aircraft_modified;
        ModuleFixture::configuration_path = ModuleFixture::working_directory / config_original;
        ModuleFixture::configuration_under_test = ModuleFixture::working_directory / config_modified;

        /* Copy the original aircraft and configuration files to the working directory */
        std::filesystem::path local_configuration = std::filesystem::current_path() / (std::string{TOOL_NAME} + "_conf.xml");
        std::filesystem::copy(local_configuration, ModuleFixture::configuration_path);

        /* Read the base aircraft for this suite from the configuration and copy it */
        auto configuration = aixml::openDocument(ModuleFixture::configuration_path);
        std::filesystem::path base_aircraft{std::string(configuration->at("control_settings/aircraft_exchange_file_directory/value"))};
        std::string aircraft_name(configuration->at("control_settings/aircraft_exchange_file_name/value"));
        std::filesystem::copy(base_aircraft / aircraft_name, ModuleFixture::aircraft_path);

        /* Copy the folder structure */
        std::for_each(
            folder_structure.begin(),
            folder_structure.end(),
            [&base_aircraft](const std::string &folder)
            {
                std::filesystem::copy(base_aircraft / folder, ModuleFixture::working_directory / folder, std::filesystem::copy_options::recursive);
            });
    }

    /**
     * @brief Tear Down Test Suite by removing the temporary working directory.
     */
    static void TearDownTestSuite()
    {
        /* Clean up the working directory after the suite is finished */
        if (std::filesystem::exists(ModuleFixture::working_directory))
        {
            std::filesystem::remove_all(ModuleFixture::working_directory);
        }
    }

    /**
     * @brief Set up the test by providing a clean copy of the configuration and aircraft files.
     */
    void SetUp() override
    {
        /* Provide a clean copy for the configuration under test */
        std::filesystem::copy(
            ModuleFixture::configuration_path, ModuleFixture::configuration_under_test,
            std::filesystem::copy_options::overwrite_existing);

        /* Provide a clean copy for the aircraft under test */
        std::filesystem::copy(
            ModuleFixture::aircraft_path, ModuleFixture::aircraft_under_test,
            std::filesystem::copy_options::overwrite_existing);
    }

    /**
     * @brief Tear Down the test by removing the modified files.
     */
    void TearDown() override
    {
        /* Clean up the modified files */
        if (std::filesystem::exists(ModuleFixture::aircraft_under_test))
        {
            std::filesystem::remove(ModuleFixture::aircraft_under_test);
        }
        if (std::filesystem::exists(ModuleFixture::configuration_under_test))
        {
            std::filesystem::remove(ModuleFixture::configuration_under_test);
        }
    }

    /* Standard paths for each test suite */
    inline static std::filesystem::path working_directory;        /**< The working directory for the module tests */
    inline static std::filesystem::path aircraft_path;            /**< The original unchanged aircraft xml file */
    inline static std::filesystem::path aircraft_under_test;      /**< The modified aircraft xml file */
    inline static std::filesystem::path configuration_path;       /**< The original unchanged configuration xml file */
    inline static std::filesystem::path configuration_under_test; /**< The modified configuration xml file */
};

/**
 * @brief Module test class for all module tests.
 * This class provides the test body for the module tests.
 * It creates a clean copy of the input files and modifies them for each test case.
 * The module under test is then executed and the expectations are checked against the result.
 */
template <typename ModuleType>
class ModuleTest : public ModuleFixture
{
  public:
    /**
     * @brief Construct a new Module Test object.
     *
     * @param testnode The test node from the test case file.
     */
    explicit ModuleTest(const node *testnode) : testnode(testnode) {}

    /**
     * @brief Execute the test body.
     */
    void TestBody() override
    {
        /* Check whether this test should be skipped */
        if (testnode->hasAttrib("skip"))
        {
            GTEST_SKIP() << testnode->getStringAttrib("skip");
        }

        /* === Perform test === */
        this->execute_module_under_test();
        this->check_output_data();
    }

  private:
    /**
     * @brief Set up the test by providing a clean copy of the configuration and aircraft files
     * and then modify the files for the test case.
     */
    void SetUp() override
    {
        /* Call the base setup which provides a clean copy of the input files */
        ModuleFixture::SetUp();

        /* Load the configuration and aircraft under test */
        auto configuration = aixml::openDocument(ModuleFixture::configuration_under_test);
        auto aircraft = aixml::openDocument(ModuleFixture::aircraft_under_test);

        /* Update the aircraft xml path in the configuration */
        configuration->at("control_settings/aircraft_exchange_file_name/value") =
            ModuleFixture::aircraft_under_test.filename().string();
        configuration->at("control_settings/aircraft_exchange_file_directory/value") =
            ModuleFixture::aircraft_under_test.parent_path().string() + "/";

        /* Turn off irrelevant output */
        configuration->at("control_settings/console_output/value") = std::string{console_mode};
        configuration->at("control_settings/log_file_output/value") = "mode_0";
        configuration->at("control_settings/plot_output/enable/value") = "0";
        configuration->at("control_settings/report_output/value") = "0";
        configuration->at("control_settings/tex_report/value") = "0";
        configuration->at("control_settings/write_info_files/value") = "0";

        /* Set the input data for the module under test */
        std::string test_parameter(this->testnode->at("parameter/path"));
        std::string test_value(this->testnode->at("parameter/value"));
        if (test_parameter.starts_with("aircraft"))
        {
            aircraft->at(test_parameter) = test_value;
        }
        else if (test_parameter.starts_with("module"))
        {
            configuration->at(test_parameter) = test_value;
        }
        else
        {
            FAIL() << "Unknown parameter path: " << test_parameter;
        }

        /* Save the modified files again */
        aixml::saveDocument(*configuration, 5);
        aixml::saveDocument(*aircraft, 5);
    }

    /**
     * @brief Execute the module under test.
     */
    void execute_module_under_test()
    {
        try
        {
            /* Create and run the module under test */
            std::unique_ptr<Module> module =
                std::make_unique<ModuleType>(
                    TOOL_NAME, TOOL_VERSION, this->configuration_under_test);
            module->execute();
        }
        /*
         * Catch the string errors from UNICADO.
         * Errors based on std::exception are
         * handled by Googletest itself.
         */
        catch (const std::string &err)
        {
            FAIL() << "Error: " << err;
        }
    }

    /**
     * @brief Check the output data of the module under test.
     */
    void check_output_data()
    {
        /* Load the result */
        auto aircraft = aixml::openDocument(ModuleFixture::aircraft_under_test);

        /* Check all the expectations */
        auto expectations = this->testnode->getVector("expect");
        std::ranges::for_each(
            expectations,
            [aircraft](const node *expectation)
            { check_expectation(aircraft, expectation); });
    }

    /* === Test Properties */
    const node *testnode; /**< The test node from the test case file */
};

/**
 * @brief Register all tests from the test case file.
 *
 * @tparam ModuleType The module type to test.
 * @param test_nodes The test nodes from the test case file.
 */
template <typename ModuleType>
void register_all_test(const std::shared_ptr<node> test_nodes)
{
    /* Loop through all test suites */
    auto test_suites = test_nodes->getVector("test_suites/test_suite");
    for (const node *test_suite : test_suites)
    {
        /* Get the name and tests for this test suite */
        const std::string suite_name = test_suite->getStringAttrib("name");
        auto test_cases = test_suite->getVector("test");

        /* Loop through all test cases */
        for (const node *test_case : test_cases)
        {
            /*
             * Register the test as per Googletest doc:
             * http://google.github.io/googletest/advanced.html#registering-tests-programmatically
             */
            auto test_name = test_case->getStringAttrib("name");
            ::testing::RegisterTest(
                suite_name.c_str(), test_name.c_str(), nullptr, nullptr,
                TOOL_NAME, 0,
                [test_case]() -> ModuleFixture *
                { return new ModuleTest<ModuleType>(test_case); }); // <- Googletest takes ownership of the pointer!
        }
    }
}

/**
 * @brief Run all module tests.
 * Call this function from the main function of your blackbox test
 * it will handle all the test execution and return the exit code.
 *
 * @tparam ModuleType The module type to test.
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 * @return int The exit code of the test suite.
 */
template <typename ModuleType>
    requires std::derived_from<ModuleType, Module>
int run_module_tests(int argc, char **argv)
{
    try
    {
        /* Init Googletest */
        ::testing::InitGoogleTest(&argc, argv);

        /* Read the test_case file and register all tests */
        auto test_nodes = aixml::openDocument(std::filesystem::path(test_case_file));
        register_all_test<ModuleType>(test_nodes);

        /* Run all tests */
        return RUN_ALL_TESTS();
    }
    /* Catch string errors from UNICADO */
    catch (const std::string &e)
    {
        std::cerr << "Error: " << e << std::endl;
        return EXIT_FAILURE;
    }
}

#endif // BLACKBOXTEST_INCLUDE_BLACKBOXTEST_MODULE_TEST_H_
