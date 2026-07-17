# UNICADO - UNIversity Conceptual Aircraft Design and Optimization
#
# Copyright (C) 2025 UNICADO consortium
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# Description:
# This file is part of UNICADO.

"""Module providing general UNICADO data preprocessing functions for Python code."""
# Import standard modules.
import os
import re
import sys
import logging
import xml.etree.ElementTree as ET
from pathlib import Path
from datetime import datetime
from inspect import currentframe, getframeinfo
from runtimeoutputmodule import configure_runtime_output


def method_data_preprocessing(paths_and_names, routing_dict, runtime_output):
    """General data preprocessing for current calculation method.

    This function performs general data preprocessing on input data obtained from aircraft exchange and module
    configuration files. It accomplishes the following tasks:
        (1) Data preparation: Extract root elements of aircraft exchange and module configuration trees from
        'paths_and_names' dict. Invoke 'user_method_data_preparation' function, specified in 'routing_dict', to obtain
        information on data to extract from these files, resulting in two dictionaries, namely the
        'data_to_extract_from_aircraft_exchange_dict' and the 'data_to_extract_from_module_configuration_dict'.
        (2) Read values from XML files: Using the above defined dictionaries with information on parameters that must
        be extracted from the aircraft exchange and module configuration file, the according values are read from the
        respective files and stored in 'tmp_aircraft_exchange_dict' and 'tmp_module_configuration_dict'. These
        temporary dictionaries have a specific format for each parameter, including the parameter's name, path, value,
        lower boundary, and upper boundary:
            tmp_dict = {'parameter_name_1': [path, expected data type, value, lower boundary, upper boundary],
                        'parameter_name_2': [...],
                        ...}
        (3) The code then iterates over both temporary dictionaries, type casts the values to their expected data types,
        checks if the values are within specified lower and upper boundaries, and stores the checked values in a new
        dictionary, 'dict_out_short'. This dictionary contains the values for the same parameters as the input
        dictionaries but with checked and possibly modified values.
    The code returns two dictionaries: 'short_aircraft_exchange_dict' and 'short_module_configuration_dict', that
    represent the preprocessed data for the aircraft exchange and module configuration file, respectively. The
    dictionaries represent condensed forms of the 'tmp_aircraft_exchange_dict' and the 'tmp_module_configuration_dict'
    and are structured according to the following scheme:
        dict = {'parameter_name_1': value, ...}

    In the case of a multi-parameter (xml path contains '@ID' identifier), the value of the parameter key
    ('parameter_name_1') contains a sub-dictionary with all existing parameter ID names ('parameter_name_1_ID...') and
    its corresponding values.
        dict = {'parameter_name_1': {'parameter_name_1_ID0': value, 'parameter_name_1_ID1': value}, ...}

    :param dict paths_and_names: Dictionary containing system paths and ElementTrees
    :param dict routing_dict: Dictionary containing information on necessary data from module configuration file
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :returns:
        - dict short_aircraft_exchange_dict: Dict containing parameters and acc. values from aircraft exchange file
        - dict short_module_configuration_dict: Dict containing parameters and acc. values from module config. file
    """

    """Data preparation."""
    # Extract roots of aircraft exchange and module configuration file.
    root_of_aircraft_exchange_tree = paths_and_names['root_of_aircraft_exchange_tree']
    root_of_module_config_tree = paths_and_names['root_of_module_config_tree']
    # Run 'user_method_data_preparation' from 'usermethoddatapreparation.py'.
    data_to_extract_from_aircraft_exchange_dict, data_to_extract_from_module_configuration_dict \
        = routing_dict['func_user_method_data_input_preparation'](routing_dict)

    """Read values from XML files."""
    # Read values from aircraft exchange and module configuration file.
    tmp_aircraft_exchange_dict = read_values_from_xml_file(data_to_extract_from_aircraft_exchange_dict,
                                                           root_of_aircraft_exchange_tree, runtime_output)
    tmp_module_configuration_dict = read_values_from_xml_file(data_to_extract_from_module_configuration_dict,
                                                              root_of_module_config_tree, runtime_output)

    """Extract, compute (type cast), and check values from output dictionary."""
    tmp_list = []
    # Iterate over both dictionaries.
    for tmp_dict in [tmp_aircraft_exchange_dict, tmp_module_configuration_dict]:
        dict_out_short = {}
        multi_parameter_dict = {}
        # Iterate over all elements of current dictionary.
        for key in tmp_dict.keys():
            # Extract and compute values.
            parameter_name = key
            expected_data_type = tmp_dict[key][1]
            # Check if the current expected data type is not tool_level.
            if expected_data_type != 'tool_level':
                value = convert_string_to_expected_data_type(
                    tmp_dict[key][-3], expected_data_type, parameter_name,
                    runtime_output)
                lower_boundary = convert_string_to_expected_data_type(tmp_dict[key][-2], expected_data_type,
                                                                    ("lower_boundary_of_" + parameter_name),
                                                                    runtime_output)
                upper_boundary = convert_string_to_expected_data_type(tmp_dict[key][-1], expected_data_type,
                                                                    ("upper_boundary_of_" + parameter_name),
                                                                    runtime_output)
                # Check if value is within specified limits.
                checked_value = check_boundaries(parameter_name, value, runtime_output, lower_boundary, upper_boundary)

                # Check if the current parameter to check is a multi-parameter with "@ID" xml path.
                if tmp_dict[key][2]:
                    if not tmp_dict[key][3] in multi_parameter_dict:
                        multi_parameter_dict[tmp_dict[key][3]] = {}
                    multi_parameter_dict[tmp_dict[key][3]][key] = checked_value
                # Else condition: current parameter is a single parameter.
                else:
                    # Set value to checked value and write to output dictionary.
                    dict_out_short[key] = checked_value
            # Else condition: The current expected data type is a tool_level.
            else:
                # Check if the value of tool_level is not None.
                if tmp_dict[key][-3] is not None:
                    dict_out_short[key] = int(tmp_dict[key][-1])
                # Else condition: The current value of tool_level is None.
                else:
                    dict_out_short[key] = None

        # Update and append 'dict_out_short'.
        dict_out_short = {**dict_out_short, **multi_parameter_dict}
        tmp_list.append(dict_out_short)

    # Extract short versions of dictionaries from 'tmp_list'.
    short_aircraft_exchange_dict = tmp_list[0]
    short_module_configuration_dict = tmp_list[1]

    return short_aircraft_exchange_dict, short_module_configuration_dict


def get_paths_and_names(module_configuration_file_name, argv):
    """Generate paths, names, and ElementTree based on module configuration file.

    This function generates paths and names as well as ElementTrees of the module configuration (config) and
    the associated aircraft exchange file. All generated parameters are returned via the output dictionary
    'paths_and_names'.

    The 'paths_and_names' output dictionary contains the following values:
        - 'working_directory': Current working directory of module (str)
        - 'parent_directory': Parent directory of module (str)
        - 'project_directory': Current project directory (str)
        - 'path_to_module_config_file': Path to module configuration file (str)
        - 'root_of_module_config_tree': Root of module configuration file tree (ElementTree)
        - 'path_to_aircraft_exchange_file': Path to aircraft exchange file (str)
        - 'root_of_aircraft_exchange_tree': Root of aircraft exchange file tree (ElementTree)
        - 'name_of_project': Name of the current aircraft project (str)
        - 'tool_name': Name of current tool (str)

    :param str module_configuration_file_name: Name of module configuration file
    :param list argv: Contains optional input arguments
    :returns:
        - dict paths_and_names: Dictionary containing system paths and ElementTrees
        - logging.Logger runtime_output: Logging object used for capturing log messages in the module
    """

    # Initialization.
    path_flag = False
    given_path = str()
    log_file_list = []
    current_parent_directory = str()
    current_working_directory = str()
    path_to_module_config_file = str()
    function_name = getframeinfo(currentframe()).function

    """Generate paths, names, and ElementTree for module configuration file."""
    # Determine the module's working directory and path to the module configuration file.
    # This section handles different cases depending on the presence of command line arguments.
    # Read and process command line arguments.
    if len(argv) == 1:
        # Read current working directory.
        current_working_directory = argv[-1]
        # Convert path of current working directory to python path (\ to /).
        current_working_directory = os.path.dirname(current_working_directory.replace(os.sep, '/'))
        if (len(argv[-1]) >= (len(os.path.splitext(module_configuration_file_name)[0][:-5]))) \
            and (len(current_working_directory) <= (len(os.path.splitext(module_configuration_file_name)[0][:-5]))):
            current_working_directory = os.getcwd()
        # Get current parent directory.
        count = current_working_directory.rfind('/')
        current_parent_directory = current_working_directory[0:count]
        # Generate path of module configuration file.
        path_to_module_config_file = (current_working_directory + '/' + module_configuration_file_name)
    else:
        # Handle a specific command line argument to set the given path.
        given_path = argv[-1]
        path_flag = True

    if path_flag:
        # Convert path of optional argument path of module configuration file to python path (\ to /).
        if not os.path.isabs(given_path):
            given_path = os.path.abspath(given_path)
            current_working_directory = given_path.replace(os.sep, '/')
        else:
            given_path = given_path.replace(os.sep, '/')
            if given_path[-1] == '/':
                current_working_directory = given_path[:-2]
            else:
                current_working_directory = given_path
        # Check if the optinal path argument is a directory or a file -> if a file -> correct the current_working_directory
        if not os.path.isdir(current_working_directory):
            count = current_working_directory.rfind('/')
            current_working_directory = current_working_directory[:count]
            # Generate path of module configuration file.
            path_to_module_config_file = given_path
        else:
           # Generate path of module configuration file.
            path_to_module_config_file = current_working_directory + '/' + module_configuration_file_name 
        # Get current parent directory.
        count = current_working_directory.rfind('/')
        current_parent_directory = current_working_directory[:count]
        

    # Determine the current module name 'tool_name' based on the module configuration file name.
    tool_name = os.path.splitext(module_configuration_file_name)[0][:-5]

    # Read ElementTree of module configuration file.
    frame_info = getframeinfo(currentframe())
    # Call function to read module configuration XML file as ElementTree.
    root_of_module_config_tree, __, log_file_list, error_flag = read_xml_information(
        path_to_module_config_file, os.path.splitext(module_configuration_file_name)[0], function_name,
        frame_info.lineno, log_file_list)

    """Generate paths, names, and ElementTree for aircraft exchange file."""
    if not error_flag:
        # Read aircraft project name and directory.
        current_aircraft_exchange_file_name = root_of_module_config_tree.find(
            "./control_settings/aircraft_exchange_file_name/value").text
        # Get name of project.
        name_of_project = os.path.splitext(current_aircraft_exchange_file_name)[0]        
        current_aircraft_exchange_file_directory = root_of_module_config_tree.find("./control_settings/aircraft_exchange_file_directory/value").text
        
        if not path_flag:
            # Check if current execution inside of an virtuell enviroment 
            #  -> if true: -> rebuild path to aircraft exchange file 
            if sys.prefix != sys.base_prefix: 
                if not os.path.isabs(current_aircraft_exchange_file_directory):
                    current_aircraft_exchange_file_directory = \
                        Path(current_aircraft_exchange_file_directory).resolve().relative_to(Path.cwd().parent)
                    current_aircraft_exchange_file_directory = str(current_parent_directory / current_aircraft_exchange_file_directory)
                
            else:
                # Get path to current aircraft project and aircraft exchange file.
                if not os.path.isabs(current_aircraft_exchange_file_directory):
                    current_aircraft_exchange_file_directory = os.path.abspath(current_aircraft_exchange_file_directory)
                       
            # get absolut path aircraft exchange file
            path_to_aircraft_exchange_file = current_aircraft_exchange_file_directory + '/' + current_aircraft_exchange_file_name
        else:
            name_of_project = os.path.splitext(current_aircraft_exchange_file_name)[0]
            path_to_aircraft_exchange_file = current_aircraft_exchange_file_directory + '/' \
                + current_aircraft_exchange_file_name

        # Read ElementTree of module configuration file.
        frame_info = getframeinfo(currentframe())
        # Call function to read aircraft exchange XML file as ElementTree.
        root_of_aircraft_exchange_tree, __, log_file_list, error_flag = read_xml_information(
            path_to_aircraft_exchange_file, name_of_project, function_name,
            frame_info.lineno, log_file_list)

        """Generate return dictionary."""
        paths_and_names = {'working_directory': current_working_directory,
                           'parent_directory': current_parent_directory,
                           'project_directory': current_aircraft_exchange_file_directory,
                           'path_to_module_config_file': path_to_module_config_file,
                           'root_of_module_config_tree': root_of_module_config_tree,
                           'path_to_aircraft_exchange_file': path_to_aircraft_exchange_file,
                           'root_of_aircraft_exchange_tree': root_of_aircraft_exchange_tree,
                           'name_of_project': name_of_project,
                           'tool_name': tool_name,
                           }
    else:
        paths_and_names = {'working_directory': current_working_directory, 'tool_name': tool_name}

    """Configure logger and initialize logger instance."""
    configure_runtime_output(paths_and_names)
    runtime_output = logging.getLogger(__name__)

    if error_flag:
        for entry in log_file_list:
            runtime_output.critical(entry)
        sys.exit(1)

    return paths_and_names, runtime_output


def read_xml_information(path, xml_file_name, function_name, code_line, log_file_list):
    """Read tree of XML file.

    This function reads and returns the ElementTree of the given XML file and its root.

    :param str path: Absolute path to the given XML file
    :param str xml_file_name: Name of the given XML file to read
    :param str function_name: Name of the function that called 'read_xml_information'
    :param int code_line: Code line number of function that called 'read_xml_information' in 1
    :param list log_file_list: Strings of workflow log file from caller function and added strings from this function
    :raises OSError: Error if XML file cannot be opened
    :returns:
        - ElementTree xml_tree: ElementTree of given XML file
        - ElementTree root_of_xml_tree: Root of ElementTree of given XML file
        - list log_file_list: List with log file entries
        - bool error_flag: Flag if error occurs (error: True, no error: False)
    """

    # Initialize local parameters.
    xml_tree = None
    error_flag = False
    root_of_xml_tree = None
    # Initialize element tree with content of file and return root element (if given).
    try:
        # Attempt to create an ElementTree and get the root element from the XML file.
        xml_tree = ET.ElementTree(file=path)
        root_of_xml_tree = xml_tree.getroot()
    # Exception handling for operating system (OS) error.
    except OSError:
        # Handle an error if the XML file cannot be opened. Print an error message and log it to a log file.
        log_file_list.append('Error in file "' + function_name + '.py" (line ' + str(code_line + 2) + ') \n'
                             '                                     ' + 'The "' + xml_file_name +
                             '.xml" file could not be opened.  \n'
                             '                                     ' + 'Program aborted!')

        error_flag = True

    return xml_tree, root_of_xml_tree, log_file_list, error_flag


def read_routing_values_from_xml(input_dict, root_of_aircraft_exchange_tree, root_of_module_configuration_tree,
                                 runtime_output, module_configuration_tmp_path=None):
    """Read routing values from XML file.

    This function reads and extracts routing values from an XML file based on the provided input dictionary and
    ElementTrees.

    The output dictionary 'return_dict' contains the following values:
        - 'layer_1': First routing layer (str)
        - 'layer_2': Second routing layer (str)
        - 'layer_3': Third routing layer (str)
        - 'user_layer': User layer (own code is implemented on this layer) (str)
        - 'tool_level': Tool level of current tool (str)

    :param dict input_dict: Input dictionary containing layer descriptions
    :param ElementTree root_of_aircraft_exchange_tree: Root of aircraft exchange XML
    :param ElementTree root_of_module_configuration_tree: Root of module configuration XML
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :param string module_configuration_tmp_path: Optional parameter for routing layer paths with ID - defaults to None
    :raises AttributeError: Error if the "own_tool_level" node does not exist
    :return dict return_dict: Output dictionary containing layer information
    """

    # Read lists with n entries from XML file (n equals number of layers).
    return_dict = input_dict
    element_exists = True
    # Iterate over keys from input dict.
    for key in input_dict:
        # Check, if 'key' contains information to be read from file.
        if input_dict[key][0] is not None:
            # Generate absolute and relative paths to parameter (key).
            absolute_path_to_parameter = input_dict[key][0]
            relative_path_to_parameter = './' + absolute_path_to_parameter.split('/', 1)[1]
            # Extract first part of path string (equals file type: 'aircraft_exchange_file' or
            # 'module_configuration_file').
            file_type = absolute_path_to_parameter.split('/')[0]
            if file_type == 'aircraft_exchange_file':
                root_of_tree = root_of_aircraft_exchange_tree
            else:
                root_of_tree = root_of_module_configuration_tree
            # Check if element (path) exists.
            tmp = root_of_tree.findall(relative_path_to_parameter)
            if tmp is None:
                element_exists = False
            # Set value of parameter if element given.
            if element_exists:
                # Only on element of layer value exist -> no ID element in the path for the routing layer node.
                if len(tmp) == 1:
                    return_dict[key] = tmp[0].text
                # At least 2 elements with the same routing layer exist -> ID element in the path for the routing layer.
                # Check if the optional parameter "module_configuration_tmp_path" is not None
                #  -> if true: -> prepare relative path to routing layer node with ID from routing layer 1.
                elif module_configuration_tmp_path is not None:
                    if module_configuration_tmp_path[-1] == '/':
                        module_configuration_tmp_path = module_configuration_tmp_path[:-1]
                    module_configuration_tmp_path = './' + module_configuration_tmp_path.split('/', 1)[1]
                    relative_path_to_parameter = relative_path_to_parameter.split(module_configuration_tmp_path)[-1]
                    id_path = module_configuration_tmp_path + '[@ID="' + next(iter(return_dict.values())) + '"]/' \
                              + relative_path_to_parameter
                    return_dict[key] = root_of_tree.find(id_path).text
                # At least 2 elements with the same routing layer exist but no optional paramter is given
                #  -> raise an error and abort program.
                else:
                    runtime_output.critical('Error: At least there are two possible parameter nodes for the routing layer. \n' #noPep8 e501
                                            '                                            Please call the function "read_routing_values_from_xml" with the optional parameter as described in "datapreprocessing.py".\n'
                                            '                                            Program abortet!')
                    sys.exit(1)

            # Set value of parameter to 'None' if not given.
            else:
                return_dict[key] = None
        # If 'key' is None, write 'None' into 'return_dict'.
        else:
            return_dict[key] = None

    # Add tool level to return dictionary.
    try:
        return_dict['tool_level'] = root_of_module_configuration_tree.find('./control_settings/own_tool_level/value').text
    except AttributeError as e:
        # Attach both handlers to the root logger
        runtime_output.critical('Error: ' + str(e) + ' \n'
                                + '                                     '
                                + 'Node "own_tool_level" not found in module configuration file. \n'
                                + '                                     ' + 'Program aborted!')
        sys.exit(1)

    return return_dict


def read_values_from_xml_file(input_dict, root_of_xml_file, runtime_output):
    """Read values from XML file.

    This function extracts specific values from a XML file, including the parameter's value, lower boundary, and upper
    boundary, based on the information provided in the 'input_dict'. It processes the XML structure of the file and
    constructs an output dictionary with the extracted values.

    The data of the output dictionary 'return_dict' are structured according to the following scheme:
    return_dict = {'parameter_name_1': [path, expected data type, bool for parameter with ID, parameter name,
                                        value, lower boundary, upper boundary],
                   'parameter_name_2': [...],
                   ...}

    :param dict input_dict: Input dictionary with information on values to read from XML file
    :param ElementTree root_of_xml_file: Root of XML tree
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :raises ValueError: Raised if parameter does not exist in node
    :return dict return_dict: Dictionary containing parameter from XML file
    """

    # Initialization.
    id_tag = str()
    cleaned_string = str()
    key_list_to_delete = []
    return_dict = input_dict
    parameter_list = ['value', 'lower_boundary', 'upper_boundary']
    file_type = root_of_xml_file._root.tag.replace('_', ' ')
    # Extraction of values from the XML file.
    try:
        # Iterate over every parameter in 'input_dict'.
        for key in input_dict:
            tmp_dict = {}
            # Find corresponding node in 'root_of_xml_file'.
            if '[@ID="0"]' in input_dict[key][0] or '[@id="0"]' in input_dict[key][0] \
                    or '[@UID="0"]' in input_dict[key][0] or '[@uid="0"]' in input_dict[key][0]:
                if '[@ID="0"]' in input_dict[key][0]:
                    cleaned_string = re.sub(r'\[@ID="0"\]', '', input_dict[key][0])
                    id_count = len(re.findall(r'\[@ID="0"\]', input_dict[key][0]))
                    id_tag = '@ID="0"'
                    id_naming = '@ID='
                elif '[@id="0"]' in input_dict[key][0]:
                    cleaned_string = re.sub(r'\[@id="0"\]', '', input_dict[key][0])
                    id_count = len(re.findall(r'\[@id="0"\]', input_dict[key][0]))
                    id_tag = '@id="0"'
                    id_naming = '@id='
                elif '[@UID="0"]' in input_dict[key][0]:
                    cleaned_string = re.sub(r'\[@UID="0"\]', '', input_dict[key][0])
                    id_count = len(re.findall(r'\[@UID="0"\]', input_dict[key][0]))
                    id_tag = '@UID="0"'
                    id_naming = '@UID='
                elif '[@uid="0"]' in input_dict[key][0]:
                    cleaned_string = re.sub(r'\[@uid="0"\]', '', input_dict[key][0])
                    id_count = len(re.findall(r'\[@uid="0"\]', input_dict[key][0]))
                    id_tag = '@uid="0"'
                    id_naming = '@uid='

                # Extract the number of existing end nodes in the aircraft exchange file of current parameter.
                key_id_list = root_of_xml_file.findall(cleaned_string)

                # Check if at least one end node is existing.
                #  -> if true: -> generate all xml paths to the existing end nodes
                key_list_to_delete.append(key)
                if len(key_id_list) > 0:
                    indexes_of_ids = []
                    index = input_dict[key][0].find(id_tag)
                    # Loop through the entire input xml path to get ID identifier indexes.
                    while index != -1:
                        indexes_of_ids.append(index)
                        index = input_dict[key][0].find(id_tag, index + 1)
                    indexes_of_ids = [x - 1 for x in indexes_of_ids]

                    string_part_list = []
                    test_string_list = []
                    # Loop across the number of indexes to split the input xml path in separate parts.
                    i = []
                    for i in range(0, len(indexes_of_ids)):
                        string_part = cleaned_string[:indexes_of_ids[i] - i * (len(id_tag) + 2)]
                        if i == 0:
                            string_part_list.append(input_dict[key][0][:(indexes_of_ids[i] + len(id_tag) + 2)])
                        else:
                            string_part_list.append(
                                input_dict[key][0][indexes_of_ids[i-1] + (len(id_tag) + 2):indexes_of_ids[i]
                                                                                           + (len(id_tag) + 2)])
                        tmp_list = [string_part, len(root_of_xml_file.findall(string_part))]
                        test_string_list.append(tmp_list)

                    # Add the xml path part behind the last ID identifier to string part list.
                    string_part_list.append(input_dict[key][0][(indexes_of_ids[i] + len(id_tag) + 2):])

                    # Generate ID list of one single parent node with all possible child nodes to target parameter.
                    number_of_fist_elements = test_string_list[0][1]
                    id_list = [int(test_string_list[0][1] / number_of_fist_elements) - 1]
                    for j in range(1, len(test_string_list)):
                        id_list.append(int(test_string_list[j][1] / test_string_list[j-1][1]) - 1)

                    # Loop across all possible nodes to generate all xml-paths to the end node of current parameter.
                    loop_count = 0
                    parameter_path_list = []
                    for i in range(len(id_list)-1, -1, -1):
                        dummy_list = []
                        # Check if current loop is the first -> if true: -> generate initial xml path elements.
                        if i == len(id_list)-1:
                            # Initialize all possible end node IDs of current parameter for the ID="0" parent root.
                            for j in range(0, id_list[i] + 1):
                                part_with_id =(
                                        string_part_list[i][:string_part_list[i].find(id_tag)
                                                             + len(id_naming)] + '"' + str(j) + '"]')
                                dummy_list.append(part_with_id + string_part_list[i+1])
                        else:
                            for j in range(0, id_list[i] + 1):
                                part_with_id =(
                                        string_part_list[i][:string_part_list[i].find(id_tag)
                                                             + len(id_naming)] + '"' + str(j) + '"]')
                                for k in range(0, len(parameter_path_list[loop_count-1])):
                                    dummy_list.append(part_with_id + parameter_path_list[loop_count-1][k])

                        parameter_path_list.append(dummy_list)
                        loop_count += 1

                    # Convert final parameter path lists of list to on final paths list.
                    if isinstance(parameter_path_list, list):
                        parameter_path_list = parameter_path_list[-1]
                    else:
                        parameter_path_list = [parameter_path_list]

                    # Check if more than one parent root node of parameter exists.
                    #  -> if true: -> add all remaining xml paths to parameter path list
                    if number_of_fist_elements > 1:
                        first_part = parameter_path_list[0][:(indexes_of_ids[0] + len(id_naming) + 1)]
                        for i in range(1, number_of_fist_elements):
                            for j in range(0, len(parameter_path_list)):
                                parameter_path_list.append(first_part + '"' + str(i) + '"' + parameter_path_list[j][(indexes_of_ids[0] + len(id_tag) + 1):])  # noPep8 e501

                    # Generate temporary dictionary with names, xml paths and expected data type.
                    for i in range(0, len(parameter_path_list)):
                        numerical_values = re.findall(r'@ID="(\d+)"', parameter_path_list[i])
                        numerical_string = ['_ID' + str(value) for value in numerical_values]
                        numerical_string = key + ''.join(numerical_string)
                        tmp_dict[numerical_string] = [parameter_path_list[i], input_dict[key][1], True, key]

                # Else condition: no one end node of current key is existing in the aircraft exchange file.
                else:
                    numerical_values = re.findall(r'@ID="(\d+)"', input_dict[key][0])
                    numerical_string = ['_ID' + str(value) for value in numerical_values]
                    numerical_string = key + ''.join(numerical_string)
                    tmp_dict[numerical_string] = [input_dict[key][0], input_dict[key][1], True, key]

            # Else condition: The string of the xml path of current key, contains no ID identifier.
            else:
                tmp_dict[key] = [input_dict[key][0], input_dict[key][1], False, key]

            # Update return dict.
            return_dict = {**return_dict, **tmp_dict}
            # Loop across all temporary key elements to read the responding values from the element tree.
            for tmp_key, value in tmp_dict.items():
                # Try to find temporary element from xml-tree.
                tmp = root_of_xml_file.find(tmp_dict[tmp_key][0])

                # Initialize 'value', 'lower_boundary', and 'upper_boundary' of value with 'None' if node does not exist
                if tmp is None or tmp_dict[tmp_key][1] is None:
                    if tmp_dict[tmp_key][2]:
                        return_dict[tmp_key] = [tmp_dict[tmp_key][0], tmp_dict[tmp_key][1], True, tmp_dict[tmp_key][3],
                                                None, None, None]
                    else:
                        return_dict[tmp_key] = [tmp_dict[tmp_key][0], tmp_dict[tmp_key][1], False, tmp_dict[tmp_key][3],
                                                None, None, None]
                    runtime_output.info('Attention: Node "' + tmp_dict[tmp_key][0] + '" not found in ' + file_type
                                         + '. Value, lower, and upper boundary initialized with "None".')
                    if not tmp is None and tmp_dict[tmp_key][1] is None:
                        return_dict[tmp_key][1] = bool
                        return_dict[tmp_key][4] = 'True'
                    elif tmp_dict[tmp_key][1] is None:
                        return_dict[tmp_key][1] = bool
                        return_dict[tmp_key][4] = 'False'
                elif tmp_dict[tmp_key][1] == 'tool_level':
                    tmp_parameter = root_of_xml_file.find(tmp_dict[tmp_key][0])
                    if tmp_parameter is not None:
                        if 'tool_level' not in tmp_parameter.attrib:
                            current_tool_level = 0
                        else:
                            current_tool_level = tmp_parameter.attrib['tool_level']
                        return_dict[tmp_key] += [current_tool_level]
                else:
                    # Check existence of every parameter in 'parameter_list' and append text if given and 'None' if not.
                    for parameter in parameter_list:
                        parameter_exists = True
                        # Append parameter to path and check existence.
                        tmp_parameter = root_of_xml_file.find(tmp_dict[tmp_key][0] + '/' + parameter)
                        # Raise error if parameter 'value' does not exist in current node.
                        if parameter == 'value' and tmp_parameter is None:
                            parameter_exists = False
                            raise ValueError('Node "' + tmp_dict[tmp_key][0] + '/' + parameter + '" not found in '
                                             + file_type + '. Program aborted!')
                        # Set 'parameter_exists' to 'False' if 'lower_boundary' or 'upper_boundary' missing, print warning.
                        elif tmp_parameter is None:
                            parameter_exists = False
                            runtime_output.info('Attention: Node "' + tmp_dict[tmp_key][0] + '/' + parameter
                                                 + '" not found in ' + file_type + '.')
                        # Append parameter text if existing (equals value of parameter).
                        if parameter_exists:
                            return_dict[tmp_key] += [tmp_parameter.text]
                        # Append 'None' to 'return_dict' if parameter does not exist, print a warning.
                        else:
                            return_dict[tmp_key] += [None]
                            runtime_output.info('Attention: No "' + parameter + '" defined for "' + tmp_key
                                                 + '". Set to "None" instead.')

        for key in key_list_to_delete:
            del return_dict[key]

    # Exception handling for ValueError.
    except ValueError as e:
        runtime_output.critical('Error:' + str(e))
        sys.exit(1)

    return return_dict


def convert_string_to_expected_data_type(input_value, expected_data_type, variable_name, runtime_output):
    """This function converts a string to a desired data type.

    This function converts an input string to the given data type (if valid). Valid data types are
        - int (integer),
        - float,
        - str (string), and
        - bool.
    The function enforces two conditions for a successful conversion:
        1) Valid expected data type: If the data type is invalid, the function returns 'None' for the return value and
        raises an error.
        2) The input value must not be 'None': This is particularly important when converting the limit values, as they
        may not exist and thus be read out as 'None' from the configuration file.
    If a value is convertible, the conversion is executed in dependence of the data type. If conversion is not
    possible, a ValueError is raised.

    :param str input_value: Input value
    :param <class 'type'> expected_data_type: Expected data type
    :param str variable_name: Name of the input variable
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :raises ValueError: Error if value cannot be converted to expected data type
    :return int/float/str/bool converted_value: Input value converted to expected data type
    """

    # Initialize output parameter (only changed if valid conversion possible).
    converted_value = None
    # Define expected data type and check if it is valid.
    expected_class_int = str(expected_data_type) == "<class 'int'>"
    expected_class_float = str(expected_data_type) == "<class 'float'>"
    expected_class_str = str(expected_data_type) == "<class 'str'>"
    expected_class_bool = str(expected_data_type) == "<class 'bool'>"
    valid_expected_data_type = (
        expected_class_int or expected_class_float or expected_class_str or expected_class_bool)
    # If 'bool' expected, the following inputs are accepted as true/false. Note that this are no conversion dicts,
    # instead they define which variations of boolean values are accepted.
    dict_bool_true = {'True': True, 'true': True, '1': True, '1.0': True}
    dict_bool_false = {'False': True, 'false': True, '0': True, '0.0': True}

    # Check if input value is of class 'NoneType'.
    input_of_class_none_type = (input_value is None) or (input_value == 'None')

    # If valid data type and value is not 'None'.
    if valid_expected_data_type and not input_of_class_none_type:
        # If expected data type is of "<class 'int'>".
        if expected_class_int:
            # Check if value is of type 'int' (could subsequently be converted to 'int').
            try:
                converted_value = expected_data_type(input_value)
            # Otherwise value is not of type 'int'.
            except ValueError:
                # Check if value is of type 'float' (could subsequently be converted to 'float' and 'int').
                try:
                    converted_value = expected_data_type(float(input_value))
                    runtime_output.info("Attention: Expected data type was 'int' but input value was of type 'float'."
                                         "The value was first converted to a float value and then to an int."
                                         "Decimal places are lost in the process.")
                # Value error (value not of type 'int' or 'float').
                except ValueError:
                    runtime_output.info(
                        ("Attention: Expected data type was 'int' but input value was neither of type 'int' "
                         "nor 'float'. Value conversion not possible for parameter '" + variable_name + "'."))
        # If expected data type is of "<class 'float'>".
        if expected_class_float:
            # Check if value can be converted to 'float' (means value is of type 'int' or 'float').
            try:
                converted_value = expected_data_type(input_value)
            # Handle exception if value is not of type 'int' or 'float'.
            except ValueError:
                runtime_output.info(
                    ("Attention: Expected data type was 'float', but the input value seems to be of type string "
                     "or bool. Value conversion not possible for parameter '" + variable_name + "'."))
        # If expected data type is of "<class 'str'>".
        if expected_class_str:
            converted_value = input_value
        # If expected data type is of "<class 'bool'>".
        if expected_class_bool:
            # Check if input is a valid expression for 'True'.
            if dict_bool_true.get(input_value):
                converted_value = True
            # Check if input is a valid expression for 'False'.
            elif dict_bool_false.get(input_value):
                converted_value = False
            # Input does not contain a valid expression for boolean values.
            else:
                runtime_output.info(
                    ("Attention: Expected data type was 'bool', "
                     "but input does not seems to contain valid expressions for boolean values."
                     "Value conversion not possible for parameter '" + variable_name + "'."))
    # No valid data type or value is 'None' (often the case if no default values provided in configuration file).
    else:
        runtime_output.info("Attention: Invalid data type or input value is 'None' (" + variable_name + ").")

    return converted_value


def check_boundaries(parameter_name, input_value, runtime_output, lower_boundary=None, upper_boundary=None):
    """Verify that a value is within specified limits.

    This function checks whether a given input value falls within specified boundaries (lower and upper limits). It is
    designed to handle values of different data types, including int, float, str, and bool. It raises errors or
    warnings when the input does not meet the expected criteria.

    :param str parameter_name: Name of the parameter
    :param int/float/str/bool input_value: Value of the parameter
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :param int/float/str/bool lower_boundary: Lower boundary (parameter value must be greater), defaults to None
    :param int/float/str/bool upper_boundary: Upper boundary (parameter value must be smaller), defaults to None
    :raises ValueError: Error if parameter value is outside the specified boundaries
    :return int/float/str/bool checked_value: Checked input value
    """

    # Initialize local parameter.
    checked_value = input_value

    # Check if boundary check possible (Value of type 'int'/'float'?).
    if isinstance(input_value, bool):
        boundary_check_possible = False
    else:
        boundary_check_possible = isinstance(input_value, (int, float))
    # Check if boundaries are given.
    boundaries_given = (lower_boundary is not None and upper_boundary is not None)

    # Perform boundary checks.
    try:
        # If value is of data type that allows boundary check.
        if boundary_check_possible:
            # If both boundaries are given.
            if boundaries_given:
                # Check if given input value lower than given lower boundary. Raise error if true.
                if input_value < lower_boundary:
                    user_value_error_string = ('The parameter "' + parameter_name
                                               + '" is lower than the expected lower boundary ('
                                               + str(lower_boundary) + '). Program aborted!')
                    raise ValueError(user_value_error_string)
                # Check if given input value higher than given upper boundary. Raise error if true.
                elif input_value > upper_boundary:
                    user_value_error_string = ('The parameter "' + parameter_name
                                               + '" is higher than the expected upper boundary ('
                                               + str(upper_boundary) + '). Program aborted!')
                    raise ValueError(user_value_error_string)
            # Raise error if no boundaries given but required.
            else:
                user_value_error_string = ('The data type "' + str(type(input_value))
                                           + ') of the given input parameter "' + parameter_name
                                           + '" requires lower and upper boundaries. Program aborted!')
                raise ValueError(user_value_error_string)
        # Input value is not of a valid data type for boundary checking.
        else:
            runtime_output.info(
                ('Attention: The data type of the given input parameter "' + parameter_name +
                 '" (' + str(type(input_value)) +
                 ') is not of type int or float. Therefore no boundaries were checked.'))
    # Exception handling if values outside the limits or no boundaries given.
    except ValueError as e:
        runtime_output.critical('Error: ' + str(e))
        sys.exit(1)

    return checked_value
