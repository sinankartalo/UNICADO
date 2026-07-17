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

"""Module providing run function of calculation method."""
# Import standard modules.
import sys
import importlib
from datapreprocessingmodule import method_data_preprocessing


def run_module(paths_and_names, routing_dict, runtime_output):
    """Conduct Python module.

    This function performs any UNICADO Python module. The process involves the following steps:
        (1) Method-specific preprocessing: The prerequisite for any UNICADO Python module is the acquisition of data
        from corresponding exchange files. These include the aircraft exchange and the module configuration file. This
        data preprocessing is crucial as it prepares the input data for the calculation method. The obtained data are
        stored in the two dictionaries 'aircraft_exchange_dict' and 'module_configuration_dict'.
        (2) Run calculation method: Depending on the user layer specified in the routing parameters, the function calls
        the appropriate calculation function. The selected function is dynamically imported and executed.
    The output dictionary 'run_output_dict' contains the result of the UNICADO Python module and is structured according
     to the following scheme:
        run_output_dict = {'parameter_name_1': value, ...}

    :param dict paths_and_names: Dictionary containing system paths and ElementTrees
    :param dict routing_dict: Dictionary containing routing parameters
    :param logging.Logger runtime_output: Logging object used for capturing log messages in the module
    :raises ModuleNotFoundError: Raised if module cannot be imported
    :return dict run_output_dict: Dictionary containing results of module execution
    """

    """Method specific preprocessing: Acquire necessary data."""
    # Run 'method_data_preprocessing' from 'datapreprocessingmodule'.
    aircraft_exchange_dict, module_configuration_dict = method_data_preprocessing(paths_and_names, routing_dict, runtime_output)

    """Run: Execute code depending on user layer."""
    # Prepare strings for dynamic imports of calculation functions. The 'import_command_method_user_layer' is build
    # according to the following scheme:
    # 'src.[value of layer_1].[value of layer_2].[value of layer_3].[value of user layer].method[value of user layer]'
    # The 'function_name' is generated according to the following scheme:
    #   'method_[value of user layer]'
    import_command_method_user_layer = (routing_dict['module_import_name'] + '.' + routing_dict['user_layer']
                                        + '.method' + routing_dict['user_layer'].replace('_', ''))
    function_name = 'method_' + routing_dict['user_layer']
    # Import calculation module.
    try:
        module = importlib.import_module(import_command_method_user_layer)
        # Call function depending on routing parameters.
        run_output_dict = getattr(module, function_name)(paths_and_names, routing_dict, aircraft_exchange_dict,
                                                         module_configuration_dict, runtime_output)
    # Exception handling for module import error.
    except ModuleNotFoundError as module_import_error:
        runtime_output.critical('Error: ' + str(module_import_error) + ' found in ' + routing_dict['module_name'] + '\n'
                                + '                                     ' + 'Program aborted!')
        sys.exit(1)

    return run_output_dict
