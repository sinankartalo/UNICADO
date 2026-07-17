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

"""Module providing general UNICADO data postprocessing functions for Python code."""
# Import standard modules.
import os
import re
import sys
import collections
import xml.etree.ElementTree as ET
from datetime import datetime


def create_element_tree_from_paths(input_dict):
    """ Create element tree from paths.

    This function creates an element tree from the xml-paths inside the given input dictionary.

    :param root input_dict: Dict containing module specific output datas.
    :return: root
    """
    ''' initialize local parameter '''
    paths = []
    values = []

    # Generate lists of paths and values
    for _, value in input_dict.items():
        paths.append(value[0])
        values.append(value[1])

    # Create the root element.
    root_name = paths[0].split('/')[1]
    root = ET.Element(root_name)

    # Build the XML tree.
    for index, path in enumerate(paths):
        # Split the path into parts.
        parts = re.split(r'\/(?![^\[]*\])', path.lstrip('./'))
        current_element = root

        for part in parts:
            # Check if part has an ID attribute
            id_match = re.search(r'(.+?)\[@ID="(\d+)"\]', part)
            if id_match:
                tag, id_value = id_match.groups()
                # Check if an element with the same tag and ID already exists.
                existing_element = current_element.find(f"./{tag}[@ID='{id_value}']")
                if existing_element is not None:
                    current_element = existing_element
                else:
                    new_element = ET.SubElement(current_element, tag, ID=id_value)
                    current_element = new_element
            else:
                # Check if an element with the same tag already exists.
                existing_element = current_element.find(part)
                if existing_element is not None:
                    current_element = existing_element
                else:
                    new_element = ET.SubElement(current_element, part)
                    current_element = new_element

        # Add 'value' sub-node with None as text content to the end node.
        value_node = ET.SubElement(current_element, 'value')
        value_node.text = str(values[index])

    return root


def insert_missing_elements(main_tree, root_of_tree_to_insert):
    """ Insert missing elements.

    This function searches and inserts missing module-dependent node elements in the aircraft exchange tree.

    :param tree main_tree: The element tree into which all data from the second tree is to be inserted.
    :param root root_of_tree_to_insert: The root node contains all the data to be inserted into the main tree.
    :return: None
    """
    root = main_tree.getroot()

    def insert_elements(first_parent, second_parent):
        for second_child in second_parent:
            # Find or create the corresponding child in the first tree
            first_child = first_parent.find(second_child.tag)
            if first_child is None:
                # If the element doesn't exist in the first tree, append it
                first_child = ET.SubElement(first_parent, second_child.tag, attrib=second_child.attrib)
                first_child.text = second_child.text
            else:
                # Update the attributes and text of the existing element
                first_child.attrib.update(second_child.attrib)
                if first_child.text is None:
                    first_child.text = second_child.text
                elif second_child.text is not None:
                    first_child.text += second_child.text
            # Recursively insert missing elements for child elements
            insert_elements(first_child, second_child)

    # Start recursive insertion from the roots
    insert_elements(root, root_of_tree_to_insert)


def find_and_remove_paths_in_tree(element_tree, cleaned_paths):
    """ Find and remove paths in tree.

    This function searches and removes given XML paths from a given element tree.
    Attention: The function has different behavior for entries in the 'component_design' node. Due to the unknown
    number of ID nodes, the entire module-dependent subtree is deleted here.
    For all other nodes, only the target nodes are removed.

    :param tree element_tree: Element tree containing all node datas.
    :param list cleaned_paths: List containing all xml paths to remove.
    :return: None
    """
    # Nested function to find parent node of current subtree node
    def find_parent(root, element):
        for parent in root.iter():
            for child in parent:
                if child == element:
                    return parent
        return None

    root = element_tree.getroot()
    # Create map for parent-child relations.
    parent_map = {c: p for p in element_tree.iter() for c in p}

    # Loop across all paths to remove from aircraft exchange tree.
    for path in cleaned_paths:
        # Convert the './' prefixed path to the standard XPath by removing the leading './'.
        xpath = path.lstrip('./')
        # Get first node of current path.
        first_node = xpath.split('/')[0]
        # Check if the first node is not 'component_design' -> if true: -> remove only the end nodes of current path.
        if not first_node == 'component_design':
            # Find elements matching the XPath.
            elements_to_remove = root.findall(xpath)
            # Check each element if is existing -> if true: -> remove node from element tree
            for elem in elements_to_remove:
                parent = parent_map.get(elem)
                if parent is not None:
                    parent.remove(elem)

        # Else condition: the first node of current path is 'component_design'
        #   -> Remove all nodes from second node to end of current path.
        else:
            second_node = xpath.split('/')[1]
            sub_tree_to_remove = root.find('component_design/' + second_node)
            if sub_tree_to_remove is not None:
                # Use a list to collect all descendants
                elements_to_remove = []
                stack = [sub_tree_to_remove]
                while stack:
                    current_element = stack.pop()
                    elements_to_remove.append(current_element)
                    stack.extend(list(current_element))
                # Remove all collected elements
                for elem in elements_to_remove[::-1]:
                    # Call nested function to find parend node of current sub tree node.
                    parent = find_parent(root, elem)
                    if parent is not None:
                        parent.remove(elem)


def convert_dictionary_to_element_tree(parameters_dict, parent=None):
    """ Convert dictionary to element tree.

    This function converts the module-dependent key parameter dict into a consistent module-dependent element tree.

    :param dict parameters_dict: Dict containing parameter for the element tree to generate.
    :param node parent: The Parent node element of current module key parameter.
    :return: element parent
    """
    # Check if is parent is None -> if true: -> initialize root node of element tree as 'module_dependent_root'.
    #  Otherwise, the given parent is an ET.Element
    if parent is None:
        parent = ET.Element('module_dependent_root')

    # Loop across the key value pairs of given dictionary to convert to an element tree.
    for key, value in parameters_dict.items():
        # Check if the current key is 'attribute' -> if true: -> set current value as an attribute of parent node
        if key == 'attributes':
            for attr_key, attr_value in value.items():
                parent.set(attr_key, str(attr_value))
        # Else if condition: Check if the current value is a dictionary -> if true: -> build sub-dictionary recursively.
        elif isinstance(value, dict):
            element = ET.Element(key)
            parent.append(element)
            # Call function for recursive tree building.
            convert_dictionary_to_element_tree(value, element)
        # Else condition: Current key value pair is an end-node -> set value of dictionary entry as text element.
        else:
            element = ET.SubElement(parent, key)
            element.text = str(value)

    return parent

def convert_element_tree_to_dictionary(root_of_tree):
    """ Converts an ElementTree or Element into a dictionary.

    :param (xml.etree.ElementTree.Element): The root element to convert.
    :return dict dictionary: A dictionary representation of the ElementTree.
    """

    def _etree_to_dict(tree):
        dictionary = {tree.tag: {} if tree.attrib else None}
        children = list(tree)
        if children:
            data_dict = {}
            for data_child in map(_etree_to_dict, children):
                for key, value in data_child.items():
                    if key in data_dict:
                        if isinstance(data_dict[key], list):
                            data_dict[key].append(value)
                        else:
                            data_dict[key] = [data_dict[key], value]
                    else:
                        data_dict[key] = value
            dictionary = {tree.tag: data_dict}
        if tree.attrib:
            dictionary[tree.tag].update(('@' + key, value) for key, value in tree.attrib.items())
        if tree.text:
            text = tree.text.strip()
            if children or tree.attrib:
                if text:
                    dictionary[tree.tag]['#text'] = text
            else:
                dictionary[tree.tag] = text
        return dictionary

    return _etree_to_dict(root_of_tree)

def get_paths_of_element_tree(element_tree, parent_path=""):
    """ Get paths of element tree.

    This function extracts all xml paths of the given element tree.

    :param tree element_tree: The element tree containing the module dependent parameter.
    :param string parent_path: The string contains the parent path of current element.
    :return: list paths
    """
    paths = []
    current_path = f"{parent_path}/{element_tree.tag}" if parent_path else element_tree.tag
    # If the element has no children, add the current path to list of paths.
    if len(element_tree) == 0:
        paths.append(current_path)
    # Run through the children recursively
    for child in element_tree:
        # Call function for recursive path generation.
        paths.extend(get_paths_of_element_tree(child, current_path))

    return paths


def prepare_element_tree_for_module_key_parameter(paths_and_names, module_key_parameters_dict):
    """Prepare element tree.

    This function prepares the element tree for the current module.

    :param dict paths_and_names: Dictionary containing system paths and ElementTrees
    :param dict module_key_parameters_dict: Dict containing information on module nodes in aircraft exchange file
    :return: dict paths_and_names
    """
    # Call function to convert the module key parameter dict to a module dependent element tree.
    module_dependent_tree = convert_dictionary_to_element_tree(module_key_parameters_dict)

    # Call function to generate all xml path of the module dependent element tree.
    element_tree_paths = get_paths_of_element_tree(module_dependent_tree)

    # Sort the list of xml paths, delete duplicates and prepare for element tree operations.
    cleaned_paths = sorted(list(set(['./' + '/'.join(path.split('/')[1:-1]) for path in element_tree_paths])))

    # Call function to remove old elements from aircraft exchange tree.
    find_and_remove_paths_in_tree(paths_and_names['root_of_aircraft_exchange_tree'], cleaned_paths)

    # Call function to insert module dependent entries to the aircraft exchange tree.
    insert_missing_elements(paths_and_names['root_of_aircraft_exchange_tree'], module_dependent_tree)

    return paths_and_names


def write_key_data_to_aircraft_exchange_file(root_of_aircraft_exchange_tree, path_to_aircraft_exchange_file,
                                             paths_to_key_parameters_list, user_output_dict, tool_level,
                                             runtime_output):
    """Write key data to the aircraft exchange file.

    This function takes key data, verifies and writes it to the aircraft exchange file.
        (1) Preparation: Using the paths contained in the 'user_output_dict', a list with user paths is generated that
        is subsequently cleaned of duplicates. Next, every path in the path list is assigned to one of the four
        categories and appended to the corresponding list:
            (a) user path already exists in aircraft exchange file ('paths_already_in_aircraft_exchange_file_list')
            (b) valid user path ('valid_user_paths_list')
            (c) invalid user path (invalid_user_paths_list)
            (d) user paths that need further checks ('user_paths_to_check_list')
        Note: Only the paths of the last category will be considered further in the following steps.
        For further processing, the paths in 'user_paths_to_check_list' are sorted in ascending order according to
        their IDs. In addition, all paths that have one or more IDs are then extracted from the key paths and appended
        to 'key_paths_with_id_list' for further use.
        (2) Path validation and key parameter check: One by one, each key path is generalized. This means that the
        number(s) of the ID(s) contained are replaced by an 'X'. All user paths from 'user_paths_to_check_list' are
        checked to see whether they match the pattern of the current generalized key path. All paths that match this
        pattern are added to the 'matching_user_paths_list'. If there are matching user paths, the code performs
        various checks to assign these user paths to either the 'valid_user_paths_list' or 'invalid_user_paths_list'.
        The checks include examining the structure and values of IDs within the user paths. After processing the key
        paths, the code checks for any user paths that are neither in the 'valid_user_paths_list' nor in the
        'invalid_user_paths_list'. These paths are considered invalid, and a warning is issued. If there are user path
        errors, error messages are generated, and the program is prepared for possible abort. The code checks whether
        all key parameters are written by the user. If any key parameter path is missing, an error message is issued.
        If there are either user path errors or missing key path errors, the code generates error messages and,
        depending on the error type, raises a ValueError exception. This exception serves as a signal to terminate the
        program.
        Note: Only the 'valid_user_path_list' will be considered further in the following steps.
        (3) Initialization of tree structure: Ensure that all necessary paths exist in aircraft exchange file to enable
        upcoming step. Furthermore, the values are checked to ensure that they are within the defined limits.
        (4) Completion: Write to aircraft exchange file. If the file cannot be opened, an OSError is raised.

    :param ElementTree root_of_aircraft_exchange_tree: Root of aircraft exchange file tree
    :param str path_to_aircraft_exchange_file: Path to aircraft exchange file
    :param list paths_to_key_parameters_list: List with paths to key parameters in aircraft exchange file
    :param dict user_output_dict: Dictionary containing parameter name, path to parameter, and value of key parameters
    :param int tool_level: Tool level of current module
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :raises ValueError: Raised if unsuccessful validation (faulty user paths, missing key paths or value out of limits)
    or failed writing to the aircraft exchange file
    :return: None
    """

    """Preparation."""
    # Generate list with user defined paths in 'user_output_dict' and ensure operating system conformity for path
    # separators.
    user_defined_path_list = [user_output_dict[key][0].replace(os.sep, '/') for key in user_output_dict]
    # Count the occurrences of each path.
    path_counter = collections.Counter(user_defined_path_list)
    duplicate_paths = [path for path, count in path_counter.items() if count > 1]
    # Remove duplicates and generate a warning.
    if duplicate_paths:
        runtime_output.warning('Warning: Duplicate paths found. Removing the following duplicates:')
        for duplicate_paths in duplicate_paths:
            runtime_output.warning('                                     ' + f"Duplicate path: {duplicate_paths}")
            user_defined_path_list.remove(duplicate_paths)

    # Initialize local parameters that indicate which paths are valid, invalid, already in aircraft exchange file, and
    # which need further checks.
    valid_user_paths_list, invalid_user_paths_list, user_paths_to_check_list = [], [], []
    paths_already_in_aircraft_exchange_file_list = []

    # Iterate over all user defined paths in 'user_defined_path_list' and assign each path to one category.
    for user_path in user_defined_path_list:
        path_not_in_aircraft_exchange_file = False
        # Check if user path (including 'value' sub-node) exists in aircraft exchange file and append to
        # 'paths_already_in_aircraft_exchange_file_list' and 'valid_user_paths_list'.
        if root_of_aircraft_exchange_tree.find(user_path + '/value') is not None:
            paths_already_in_aircraft_exchange_file_list.append(user_path)
            valid_user_paths_list.append(user_path)
        # Else: Set 'path_not_in_aircraft_exchange_file' to 'True'.
        else:
            path_not_in_aircraft_exchange_file = True

        # If path is not already contained in aircraft exchange file, append path to one of the following three lists:
        # 'valid_user_paths_list', 'invalid_user_paths_list', or 'user_paths_to_check_list'.
        if path_not_in_aircraft_exchange_file:
            # If last character of 'user_path' is ']', path is considered invalid.
            if user_path[-1] == ']':
                invalid_user_paths_list.append(user_path)
                continue
            # Count the number of IDs in 'user_path' string.
            user_path_id_count = user_path.count('[@ID="')
            # If current user path is contained in list of key parameter paths, append user path to list of valid user
            # paths.
            if user_path in paths_to_key_parameters_list:
                valid_user_paths_list.append(user_path)
            # If current user path is not contained in list of key parameter paths and user path does not contain an
            # ID, append user path to list of invalid user paths.
            elif user_path not in paths_to_key_parameters_list and user_path_id_count == 0:
                invalid_user_paths_list.append(user_path)
            # If none of above criteria apply, the user path is appended to the list of paths that need further checks.
            else:
                user_paths_to_check_list.append(user_path)

    # Extract the values after "@ID=" from the paths in the list of paths that need further checks and sort this list.
    id_values = [int(re.search(r'@ID="(\d+)"', path).group(1)) for path in user_paths_to_check_list]
    user_defined_path_list_sorted = [path for _, path in sorted(zip(id_values, user_paths_to_check_list))]

    # Extract key paths that contain "@ID".
    key_paths_with_id_list = [path for path in paths_to_key_parameters_list if re.search(r'@ID="\d+"', path)]

    """Path validation and key parameter check."""
    # Classify each existing user path into a category based on generalized key parameter paths and check for missing
    # key parameter paths.
    try:
        # Initialization of variables for error tracking.
        error_path_dict = {}
        user_path_error = False
        user_path_error_counter = 0
        missing_key_path_error = False
        # Iterate over key paths in list of key paths with IDs.
        for current_key_path in key_paths_with_id_list:
            # Check if current key path exists in 'valid_user_paths_list' (True/False).
            key_path_exists_in_user_paths = current_key_path in valid_user_paths_list
            # Define ID pattern to generalize "@ID" attribute in 'current_key_path' (replace ID number with 'X').
            id_pattern = re.compile(r'@ID="(\d+)"')
            # Store current generalized key parameter path.
            current_generalized_key_path = re.sub(r'@ID="\d+"', '@ID="X"', current_key_path)
            # Create empty list of matching user paths.
            # Iterate over all user paths in sorted list of user defined paths.
            # - Replace ID number with 'X' for generalization purposes.
            # - Append all user paths that match the pattern of the current generalized key parameter path.
            matching_user_paths_list = [user_path for user_path in user_defined_path_list_sorted if
                re.sub(r'@ID="\d+"', '@ID="X"', user_path) == current_generalized_key_path]

            # Sub function to sort all paths by his last ID entry in numerical order.
            def extract_id(matching_user_paths_list):
                matches = re.findall(r'@ID="(\d+)"', matching_user_paths_list)
                return int(matches[-1]) if matches else float('inf')

            # Sort the list by the extracted ID value.
            matching_user_paths_list = sorted(matching_user_paths_list, key=extract_id)

            # If any user paths match the current generalized key parameter path, various checks are performed to
            # assign the corresponding user paths to either the "valid_user_paths_list" or the
            # "invalid_user_paths_list".
            if len(matching_user_paths_list) > 0:
                # Create empty list of current user path IDs.
                user_path_ids_list = []
                # Iterate over list with user paths that match current key parameter path pattern and extract the
                # values of contained IDs.
                for current_matching_user_path in matching_user_paths_list:
                    # Find all IDs of current user path.
                    values_of_user_ids_list = re.findall(id_pattern, current_matching_user_path)
                    # Convert the numbers from strings to integers.
                    values_of_user_ids_list = [int(num) for num in values_of_user_ids_list]
                    # Generate list of IDs for current user path.
                    user_path_ids_list.append(values_of_user_ids_list)

                # If the 'current_key_path' exists in 'valid_user_paths_list' and there are user path IDs, the IDs are
                # compared and possible errors handled.
                if key_path_exists_in_user_paths and len(user_path_ids_list) != 0:
                    # Create a list with n zeros (n corresponds to the number of IDs in the current generalized key
                    # parameter path) and store the list in which all IDs are zero as 'first_list'.
                    zero_list = [0 for _ in range(len(user_path_ids_list[0]))]
                    user_path_ids_list.insert(0, zero_list)
                    first_list = user_path_ids_list[0]
                    # Check each position in the lists.
                    for i in range(1, len(user_path_ids_list)):
                        # Check whether the first element of the previous list is NOT the same as the first element of
                        # the current list.
                        if first_list != user_path_ids_list[i]:
                            # If the two IDs differ by more than 1, add the path to the list of invalid paths and go on
                            # with the next generalized key parameter path.
                            if abs(sum(first_list) - sum(user_path_ids_list[i])) > 1 \
                                    and (abs(first_list[-1] - user_path_ids_list[i][-1]) > 1):
                                user_path_error_counter += 1
                                error_path_dict[current_generalized_key_path] = matching_user_paths_list[i-1:]
                                [invalid_user_paths_list.append(matching_user_paths_list[j])
                                    for j in range(0, len(matching_user_paths_list))]
                                break
                            # If the first ID differs by 1 compared to the previous path, then a check of the following
                            # IDs is performed.
                            else:
                                valid_user_paths_list.append(matching_user_paths_list[i-1])
                                first_list = user_path_ids_list[i]
                        # If the first ID of the current path matches the first ID of the previous path, the subsequent
                        # IDs are subjected to further checks.
                        else:
                            # If the difference between the two lists is greater than 1, append the current path to the
                            # list of invalid paths and continue with the next generalized key parameter path.
                            if abs(sum(first_list) - sum(user_path_ids_list[i])) > 1:
                                user_path_error_counter += 1
                                error_path_dict[current_generalized_key_path] = matching_user_paths_list[i-1:]
                                [invalid_user_paths_list.append(matching_user_paths_list[j])
                                    for j in range(0, len(matching_user_paths_list))]
                                break
                            # If the difference between the two lists is less than or equal to 1, then append the
                            # current path to the list of valid paths.
                            elif abs(sum(first_list) - sum(user_path_ids_list[i])) <= 1:
                                valid_user_paths_list.append(matching_user_paths_list[i-1])
                                first_list = user_path_ids_list[i]
                # If the 'current_key_path' exists in 'valid_user_paths_list', but there are no matching user paths, a
                # warning is issued.
                elif key_path_exists_in_user_paths and len(user_path_ids_list) == 0:
                    runtime_output.warning('Warning: No matching user paths according to key pattern: '
                                           + current_generalized_key_path)
                    continue
                # If the 'current_key_path' does not exist in 'valid_user_paths_list' and is not contained in
                # 'paths_already_in_aircraft_exchange_file_list', a warning is issued and the current user paths are
                # appended to the invalid paths.
                elif not key_path_exists_in_user_paths \
                        and current_key_path not in paths_already_in_aircraft_exchange_file_list:
                    runtime_output.warning('Warning: Key path missing in user defined path list: ' + current_key_path)
                    user_path_error_counter += 1
                    error_path_dict[current_generalized_key_path] = matching_user_paths_list
                    [invalid_user_paths_list.append(matching_user_paths_list[j])
                        for j in range(0, len(matching_user_paths_list))]
                    continue

        # After processing the key paths, it is checked for any user paths that are neither in 'valid_user_paths_list'
        # nor in 'invalid_user_paths_list'. These paths are considered invalid, and a warning is issued.
        for tmp_path in user_defined_path_list_sorted:
            if tmp_path not in valid_user_paths_list and tmp_path not in invalid_user_paths_list:
                invalid_user_paths_list.append(tmp_path)
                runtime_output.warning(
                    ('Warning: The path "' + tmp_path + '" is not a key value and therefore not written to aircraft '
                     'exchange file. Please contact module manager for further instructions.'))

        # If there are user path errors, error messages are generated.
        if user_path_error_counter > 0:
            user_path_error = True
            # Generate error messages.
            for key, value in error_path_dict.items():
                runtime_output.error('Error: The following user paths of the pattern "' + key + '" are invalid:')
                for i, value in enumerate(value):
                    runtime_output.error('                                     ' + value)
                user_path_error_string = 'Please change user paths according to style guidelines.'

        # Check whether all key parameters are written by the user.
        missing_key_path_list = []
        missing_key_path_error_string = str()
        for tmp_key_path in paths_to_key_parameters_list:
            # If a key parameter path is missing, an error message is issued.
            if tmp_key_path not in valid_user_paths_list:
                runtime_output.error('Error: The following key parameter is not set: ' + tmp_key_path)
                missing_key_path_list.append(tmp_key_path)
        # If there are missing key path errors, error messages are generated.
        if len(missing_key_path_list) != 0:
            missing_key_path_error = True
            missing_key_path_error_string = 'Please make sure to write all necessary key parameters of your method.'

        # If there are user path errors or missing key path errors, error messages are generated and the program is
        # aborted with a ValueError exception.
        if user_path_error or missing_key_path_error:
            if user_path_error and not missing_key_path_error:
                raise ValueError(user_path_error_string + ' Program aborted!')
            elif not user_path_error and missing_key_path_error:
                raise ValueError(missing_key_path_error_string + ' Program aborted!')
            else:
                raise ValueError(user_path_error_string[:-1] + ' and ' + missing_key_path_error_string.lower()
                                 + ' Program aborted!')

    # Exception handling for value error.
    except ValueError as e:
        runtime_output.critical('Error: ' + str(e))
        sys.exit(1)

    """Initialization of tree structure."""
    # Initialization.
    component_layer_old = str()
    sub_node_list = ['value', 'unit', 'lower_boundary', 'upper_boundary']
    # Extract the corresponding dictionary entries to the valid user paths.
    valid_key_dict = {key: value for (key, value) in user_output_dict.items()
                      if user_output_dict[key][0] in valid_user_paths_list}

    # Create all necessary nodes in the aircraft exchange file and check whether the results are within the expected
    # limits.
    try:
        # Iterate over all parameters in 'valid_key_dict'.
        for key in valid_key_dict:
            # Extract path.
            tmp_string = valid_key_dict[key][0]
            # Split 'tmp_string' at operating system separator.
            parts_list = tmp_string.split('/')
            # Delete all empty list entries if existing.
            filtered_parts = [part for part in parts_list if part]
            # Store third element as 'component_layer'.
            component_layer = filtered_parts[2]
            # Initialization of necessary variables.
            parent_path = []
            path_to_check = '.'
            first_id_parent = []
            tmp_zero_path = str()
            path_contains_id = False
            # Check if the current part of string is existing in the aircraft exchange ElementTree.
            for part in filtered_parts[1:]:
                # Extend the 'path_to_check' with the current 'part'.
                path_to_check = os.path.join(path_to_check, part).replace(os.sep, '/')
                # Check if the path exist in aircraft exchange ElementTree.
                path_flag = root_of_aircraft_exchange_tree.find(path_to_check)
                # Check if the 'component_layer' is the same as the 'component_layer_old'.
                same_component_layer = component_layer == component_layer_old
                # Set 'tool_level' attribute if path exists, the current part equals 'component_layer', and if the
                # 'component_layer' is different from the previous one.
                if path_flag is not None and part == component_layer and not same_component_layer:
                    path_flag.set('tool_level', tool_level)
                    component_layer_old = component_layer
                # Add the current part of string to the ElementTree as a new sub-node if the node does not exist.
                if path_flag is None:
                    # Check if current part of string contains '@' (indicating ID).
                    if '@' in part:
                        # Set flag if string contains '@'.
                        path_contains_id = True
                        if len(first_id_parent) == 0:
                            first_id_parent = parent_path
                        # Handle attribute 'ID' (extract 'ID' and value of ID, generate new sub-node under current
                        # 'parent_path', and set attribute 'ID' with according value).
                        attribute_name, attribute_value = part.split('=')
                        attribute_name = attribute_name.split('[@')
                        attribute_id = attribute_name[1]
                        attribute_value = attribute_value[attribute_value.find('"')+1:attribute_value.rfind('"')]
                        node_name = attribute_name[0]
                        new_node = ET.SubElement(parent_path, node_name)
                        new_node.set(attribute_id, attribute_value)
                        # Handle attribute 'description' (set the description to description of the 'tmp_zero_path').
                        tmp_pattern = r'"(.*?)"'
                        tmp_zero_path = re.sub(tmp_pattern, '"0"', path_to_check)
                        tmp_description = root_of_aircraft_exchange_tree.find(tmp_zero_path).get('description')
                        new_node.set('description', tmp_description)
                    # Current path does not contain '@'.
                    else:
                        if len(tmp_zero_path) != 0:
                            tmp_zero_path = tmp_zero_path + '/' + part
                        else:
                            tmp_pattern = r'"(.*?)"'
                            tmp_zero_path = re.sub(tmp_pattern, '"0"', path_to_check)
                            path_contains_id = True
                        element_to_add = ET.Element(part)
                        # Check if description exists.
                        if path_to_check == './component_design/fuselage/specific/geometry/fuselage[@ID="0"]/mass_breakdown/fuselage_furnishing/component_mass[@ID="0"]/mass':
                            formatted_xml = ET.tostring(root_of_aircraft_exchange_tree.getroot(), encoding='unicode', method='xml')
                            formatted_xml_with_indent = minidom.parseString(formatted_xml).toprettyxml(indent="    ")
                        description_of_zero_path = \
                            root_of_aircraft_exchange_tree.find(tmp_zero_path).get('description')
                        element_to_add.set('description', description_of_zero_path)
                        # Append 'element_to_add' to 'parent_path'.
                        parent_path.append(element_to_add)
                parent_path = root_of_aircraft_exchange_tree.find(path_to_check)
                # Check if the current 'part' is the last element in the 'filtered_parts' list.
                if part == filtered_parts[-1]:
                    # Check if 'path_to_check' contains an ID.
                    if path_contains_id:
                        # Check if 'path_to_check' is not equal to 'tmp_zero_path'.
                        if path_to_check != tmp_zero_path:
                            # Check if the current parameter not is not 'name'
                            #  -> if true: -> add all sub nodes to current parameter
                            if part != 'name':
                                # Iterate through 'sub_node_list'.
                                for sub_node in sub_node_list:
                                    # Check if 'sub-node' exists in 'tmp_zero_path'.
                                    tmp_sub_node_exists_in_zero_path = \
                                        root_of_aircraft_exchange_tree.find(tmp_zero_path + '/' + sub_node)
                                    if tmp_sub_node_exists_in_zero_path is not None:
                                        # Create new XML subelement with same name as 'sub_node' under 'parent_path'.
                                        ET.SubElement(parent_path, sub_node)
                                        # Get associated element. Set text to text of element in 'tmp_zero_path/sub_node'.
                                        tmp_path = root_of_aircraft_exchange_tree.find(path_to_check + '/' + sub_node)
                                        if sub_node == 'value':
                                            if isinstance(user_output_dict[key][1], bool):
                                                if user_output_dict[key][1]:
                                                    tmp_path.text = 'true'
                                                else:
                                                    tmp_path.text = 'false'
                                            else:
                                                tmp_path.text = str(user_output_dict[key][1])
                                        else:
                                            tmp_path.text = str(root_of_aircraft_exchange_tree.find(
                                                tmp_zero_path + '/' + sub_node).text)
                            # Else condition: The current parameter not is 'name'
                            #  -> add only 'value' sub note to current parameter
                            else:
                                # Check if 'value' exists in 'tmp_zero_path'.
                                tmp_sub_node_exists_in_zero_path = \
                                    root_of_aircraft_exchange_tree.find(tmp_zero_path + '/value')
                                if tmp_sub_node_exists_in_zero_path is not None:
                                    # Create new XML subelement with same name as 'sub_node' under 'parent_path'.
                                    ET.SubElement(parent_path, 'value')
                                    # Get associated element. Set text to text of element in 'tmp_zero_path/sub_node'.
                                    tmp_path = root_of_aircraft_exchange_tree.find(path_to_check + '/value')
                                    tmp_path.text = str(user_output_dict[key][1])
                    # 'path_to_check' does not contain an ID.
                    else:
                        # Find the XML element at 'path_to_check' + '/value'.
                        tmp_path = root_of_aircraft_exchange_tree.find(path_to_check + '/value')
                        # Get value associated with the key from 'user_output_dict'.
                        tmp_value = user_output_dict[key][1]
                        # Find lower and upper boundary elements.
                        lower_boundary = root_of_aircraft_exchange_tree.find(path_to_check + '/lower_boundary')
                        upper_boundary = root_of_aircraft_exchange_tree.find(path_to_check + '/upper_boundary')
                        # Check if lower and upper boundaries are checkable (checkable means not None and not "None").
                        lower_boundary_checkable = lower_boundary is not None and \
                            (lower_boundary.text is not None and lower_boundary.text != 'None')
                        upper_boundary_checkable = upper_boundary is not None and \
                            (upper_boundary.text is not None and upper_boundary.text != 'None')
                        # Check if the value falls below the lower boundary (if checkable).
                        if lower_boundary_checkable and tmp_value < float(lower_boundary.text):
                            raise ValueError('The value of the parameter ' + str(key) + ' = ' + str(tmp_value)
                                             + ' falls below the given lower boundary of ' + lower_boundary.text
                                             + '. Program aborted!')
                        # Check if the value exceeds the upper boundary (if checkable).
                        if upper_boundary_checkable and tmp_value > float(upper_boundary.text):
                            raise ValueError('The value of the parameter ' + str(key) + ' = ' + str(tmp_value) +
                                             ' exceeds the given upper boundary of ' + upper_boundary.text
                                             + '. Program aborted!')
                        # If no boundary conditions were violated, set tmp_path.text to the value.
                        if isinstance(tmp_value, bool):
                            if tmp_value:
                                tmp_value = 'true'
                            else:
                                tmp_value = 'false'

                        tmp_path.text = str(tmp_value)

            # Sort all child nodes alphabetically according to their tags.
            sort_root = first_id_parent
            children = list(sort_root)
            children.sort(key=lambda x: x.tag)
            # Delete all child nodes from root element.
            for child in children:
                sort_root.remove(child)
            # Add the sorted child nodes back to the root element.
            for child in children:
                sort_root.append(child)

    # Exception handling for value error.
    except ValueError as e:
        runtime_output.critical('Error:' + str(e))
        sys.exit(1)

    """Completion."""
    # Ensure proper indentation.
    ET.indent(root_of_aircraft_exchange_tree, space="    ", level=0)
    # Write all key parameters to aircraft exchange file.
    try:
        # Write data to file.
        root_of_aircraft_exchange_tree.write(path_to_aircraft_exchange_file,  encoding='utf-8')
    # Exception handling for operating system error.
    except OSError:
        runtime_output.critical('Error: Writing to aircraft exchange file failed. Program aborted!')
        sys.exit(1)


def method_data_postprocessing(paths_and_names, routing_dict, data_dict, method_specific_output_dict, runtime_output):
    """General data postprocessing for current calculation method.

    This function executes the method's own postprocessing. It is divided into general postprocessing and user layer
    specific postprocessing:
        - General postprocessing: The general postprocessing contains operations that are always carried out regardless
        of the user layer. This includes general reports and plots.
        - User layer specific postprocessing: Specific postprocessing includes, for example, plots that can/should only
        be created if the user layer contains a certain value. The same applies to reports with values that are only
        determined for certain user layer values.
    Note that it may also be possible that the specific part is omitted, as the entire postprocessing is independent of
    the user layer.

    :param dict paths_and_names: Dictionary containing system paths and ElementTrees
    :param dict routing_dict: Dictionary containing routing parameters
    :param dict data_dict: Dictionary containing results of module execution
    :param dict method_specific_output_dict: Dictionary containing method-specific output data
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :raises OSError: Raised if any method-specific postprocessing function fails
    :return: None
    """

    # Read output switches from module configuration file.
    root_of_module_config_tree = paths_and_names['root_of_module_config_tree']
    plot_switch = (eval(root_of_module_config_tree.find('.//plot_output/enable/value').text.capitalize()))
    html_switch = eval(root_of_module_config_tree.find('.//report_output/value').text.capitalize())
    tex_switch = eval(root_of_module_config_tree.find('.//tex_report/value').text.capitalize())
    if root_of_module_config_tree.find('.//xml_output/value') is not None:
        xml_export_switch = eval(root_of_module_config_tree.find('.//xml_output/value').text.capitalize())
    else:
        xml_export_switch = False

    # Plot functionality.
    if plot_switch:
        if not os.path.isdir(paths_and_names['project_directory'] + '/reporting/plots'):
            os.makedirs(paths_and_names['project_directory'] + '/reporting/plots')
        try:
            # Run 'method_plot' from 'methodplot.py'.
            routing_dict['func_user_method_plot'](paths_and_names, routing_dict, data_dict, method_specific_output_dict,
                                                  runtime_output)
        except OSError as e:
            runtime_output.error(str(e) + '\n '
                                 + '                                     '
                                 + 'Error: "method_plot" function failed. No plots generated and saved.')
    else:
        runtime_output.warning('Warning: "plot_output" switch in module configuration file set to "False". '
                               + 'No plots generated.')

    # HTML report functionality.
    if html_switch:
        if not os.path.isdir(paths_and_names['project_directory'] + '/reporting/report_html'):
            os.makedirs(paths_and_names['project_directory'] + '/reporting/report_html')
            
        try:
            # Run 'method_html_report' from 'methodhtmlreport.py'.
            routing_dict['func_user_method_html_report'](paths_and_names, routing_dict, data_dict,
                                                         method_specific_output_dict, runtime_output)
        except OSError as e:
            runtime_output.error(str(e) + '\n '
                                 + '                                     '
                                 + 'Error: "method_html_report" function failed. '
                                 + 'No additional data written to HTML report file.')
    else:
        runtime_output.warning(
            'Warning: "html_output" switch in module configuration file set to "False". No HTML report generated.'
        )

    # XML export functionality.
    if xml_export_switch:
        if not os.path.isdir(paths_and_names['project_directory'] + '/reporting/report_xml'):
            os.makedirs(paths_and_names['project_directory'] + '/reporting/report_xml')
            
        xml_export_tree, path_to_results_file = prepare_method_specific_xml_file(paths_and_names, routing_dict,
                                                                                 runtime_output)
        try:
            # Run 'method_xml_export' from 'methodxmlexport.py'.
            routing_dict['func_user_method_xml_export'](paths_and_names, routing_dict, data_dict,
                                                        method_specific_output_dict, xml_export_tree,
                                                        path_to_results_file, runtime_output)
        except OSError as e:
            runtime_output.error(str(e) + '\n '
                                 + '                                     '
                                 + 'Error: "method_xml_export" function failed. '
                                 + 'No additional data written to module specific XML results file.'
                                 )
    else:
        runtime_output.warning('Warning: "xml_output" switch in module configuration file set to "False". '
                               + 'No XML results file generated.')

    # TeX output functionality.
    if tex_switch:
        if not os.path.isdir(paths_and_names['project_directory'] + '/reporting/report_tex'):
            os.makedirs(paths_and_names['project_directory'] + '/reporting/report_tex')
            
        try:
            # Run 'method_tex_output' from 'methodtexoutput.py'.
            routing_dict['func_user_method_tex_output'](paths_and_names, routing_dict, data_dict,
                                                        method_specific_output_dict, runtime_output)
        except OSError as e:
            runtime_output.error(str(e) + '\n '
                                 + '                                     '
                                 + 'Error: "method_tex_output" function failed. '
                                 + 'No TeX report file generated.'
                                 )
    else:
        runtime_output.warning(
            'Warning: "tex_output" switch in module configuration file set to "False". No TeX report file generated.')


def prepare_method_specific_xml_file(paths_and_names, routing_dict, runtime_output):
    """Generate XML file with general information on module execution to prepare the method-specific data output.

    This function generates the basic structure of an XML file that is intended for the export of method-specific data.
    This involves the following steps:
        (1) Generate the file and module name as well as the path to the results file using information provided by the
        'paths_and_names' dictionary.
        (2) Delete older versions of the file (if existing).
        (3) Create the XML structure
            3.1) Create a 'general_information' block that contains the following information:
                - Version of the current UNICADO workflow
                - Code execution date and time
                - Current aircraft project name
                - Calculation method name
            3.2) Create a 'routing_layer' block that contains information on the current routing layers.
            3.3) Create a 'calculation_results' block that serves as a placeholder for the subsequent export of data
            (if desired)

    :param dict paths_and_names: Dictionary containing system paths and ElementTrees
    :param dict routing_dict: Dictionary containing routing parameters
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :raises OSError: Raised if workflow version file not found
    :returns:
        - ElementTree xml_export_tree: Element tree of method-specific XML tree
        - str path_to_results_file: Path to method-specific output XML file
    """

    # Initialize parameters.
    file_name = paths_and_names['tool_name'] + '_results.xml'
    module_name = paths_and_names['tool_name'].replace('_', ' ').capitalize()
    path_to_results_file = paths_and_names["project_directory"] + '/reporting/report_xml/' + file_name

    # Delete older output file if existing.
    if os.path.isfile(path_to_results_file):
        os.remove(path_to_results_file)

    # Create directory for xml reports, if not existing
    os.makedirs(paths_and_names["project_directory"] + '/reporting/report_xml/', exist_ok = True)

    # Generate new ElementTree.
    xml_export_root = ET.Element("module_results_file")
    # Set name of current tool 'Name' of root element and generate ElementTree.
    xml_export_root.set("Name", module_name + " specific outputs")
    xml_export_tree = ET.ElementTree(xml_export_root)
    # Add 'general_information' sub-node.
    child = ET.SubElement(xml_export_root, "general_information")
    child.set("description", "General information on module execution")

    try:
        # Initialize general information parameters.
        if os.path.isfile(paths_and_names['working_directory'] + '/version.txt'):
            # Open file and read version information.
            with open(paths_and_names['working_directory'] + '/version.txt', 'r') as file:
                # Read first line.
                workflow_version = file.readline()
        else:
            workflow_version = "not available"
    except OSError as e:
        runtime_output.warning('Warning: ' + str(e) + ' \n'
                               + '                                     '
                               + 'Workflow version file not found.')

    execution_date = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
    root_of_module_config_tree = paths_and_names['root_of_module_config_tree']
    project_name = (
        root_of_module_config_tree.find('./control_settings/aircraft_exchange_file_name/value').text.split(".xml"))[0]
    method_name = root_of_module_config_tree.find('./program_settings/configuration/method_name/value').text
    # Definition of subnodes of 'general_information'.
    # Format: general_information_subnodes = { 'name_of_sub-node': [description, value], ...}
    general_information_subnodes = {
        'workflow_version': ['Version number of the current workflow', workflow_version],
        'execution_date': ['Execution date and time of the code', execution_date],
        'project_name': ['Name of the current aircraft project', project_name],
        'method_name': ['Name of current module calculation method', method_name]
    }
    # Iterate over 'general_information_subnodes' dictionary and add all keys as children.
    for key, value in general_information_subnodes.items():
        # Create a subelement for each key.
        key_element = ET.SubElement(child, key)
        # Add an attribute "description" and set the value to the first entry of the value-list.
        key_element.set("description", value[0])
        # Add a subelement "value" and set the value as text.
        value_element = ET.SubElement(key_element, "value")
        value_element.text = value[1]
    # Add routing layer block.
    routing_layer_element = ET.SubElement(child, 'routing_layer')
    routing_layer_element.set("description", "Routing layer information")
    # Iterate over 'routing_dict' and add keys that contain 'layer' as children of 'routing_layer_element'.
    for key, value in routing_dict.items():
        if 'layer' in key:
            key_element = ET.SubElement(routing_layer_element, key)
            key_element.set("description", 'Routing ' + str(key))
            value_element = ET.SubElement(key_element, "value")
            value_element.text = value
    # Add 'calculation_results' block.
    child = ET.SubElement(xml_export_root, "calculation_results")
    child.set("description", "Results of calculation method")

    # Ensure proper indentation and write file.
    ET.indent(xml_export_root, space="    ", level=0)
    try:
        xml_export_tree.write(path_to_results_file)
    except OSError as e:
        runtime_output.critical('Error: ' + str(e))
        sys.exit(1)

    return xml_export_tree, path_to_results_file
