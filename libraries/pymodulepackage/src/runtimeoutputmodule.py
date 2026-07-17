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

"""Module configuring the runtime output."""
# Import standard modules.
import sys
import logging


def configure_runtime_output(paths_and_names):
    """ Initialize logging handler for console prints and log file writing, provide runtime_output instance.

    [Add some text here...]

    :param dict paths_and_names: Dictionary containing system paths and ElementTrees
    :raises AttributeError: ...
    :return:
    """
    # Define a new log level 'PRINT' with a value of 35.
    PRINT = 35
    logging.addLevelName(PRINT, "PRINT")

    # Create a custom log level class by subclassing logging.Filter.
    class PrintoutFilter(logging.Filter):
        def filter(self, record):
            return record.levelno == PRINT

    # Attach the custom filter to the 'root_logger'.
    root_logger = logging.getLogger()
    root_logger.addFilter(PrintoutFilter())

    # Add a custom method to the logger.
    def printout(self, message, *args, **kwargs):
        """
        :param self:
        :param message:
        :param args:
        :param kwargs:
        :return:
        """
        if self.isEnabledFor(PRINT):
            self._log(PRINT, message, args, **kwargs)

    # Attach the custom method to the logger.
    logging.Logger.print = printout

    # Set the logging level for the root logger.
    root_logger.setLevel(logging.DEBUG)

    """Genereate log file handler and initialze."""
    # Create a file handler with the desired file name and format.
    log_file_name = paths_and_names['working_directory'] + '/' + paths_and_names['tool_name'] + '.log'
    log_format = '%(asctime)s - %(levelname)s - %(message)s'
    file_handler = logging.FileHandler(log_file_name)
    file_handler.setFormatter(logging.Formatter(log_format))

    """Genereate console handler and initialze."""
    # Create a stream handler to output log messages to the console.
    console_format = '%(asctime)s - %(levelname)s - %(message)s'
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(logging.Formatter(console_format))

    """Set log file handler level to selected mode from module configuration file."""
    # Extract 'log_file_output' from 'root_of_module_config_tree'.
    try:
        log_file_mode = paths_and_names['root_of_module_config_tree'].find('.//log_file_output/value').text
    except AttributeError as e:
        # Attach both handlers to the 'root_logger'.
        root_logger.addHandler(file_handler)
        root_logger.addHandler(console_handler)
        logger = logging.getLogger('module_logger')
        logger.critical('Error: ' + str(e) + ' \n'
                         + '                                     '
                         + 'Node "log_file_output" not found in module configuration file. \n'
                         + '                                     ' + 'Program aborted!')
        sys.exit(1)

    match log_file_mode:
        # Only 'CRITICAL' logs displayed.
        case 'mode_0':
            file_handler.setLevel(logging.CRITICAL)
        # Logs of type 'CRITICAL', 'ERROR', 'PRINTOUT', and 'WARNING' displayed.
        case 'mode_1':
            file_handler.setLevel(logging.WARNING)
        # Logs of type 'CRITICAL', 'ERROR', 'PRINTOUT', 'WARNING', and 'INFO' displayed.
        case 'mode_2':
            file_handler.setLevel(logging.INFO)
        # Logs of type 'CRITICAL', 'ERROR', 'PRINTOUT', 'WARNING', 'INFO', and 'DEBUG' displayed.
        case 'mode_3':
            file_handler.setLevel(logging.DEBUG)

    """Set console handler level to selected mode from module configuration file."""
    # Extract 'console_output' from 'root_of_module_config_tree'.
    try:
        console_output = paths_and_names['root_of_module_config_tree'].find('.//console_output/value').text
    except AttributeError as e:
        # Attach both handlers to the 'root_logger'.
        root_logger.addHandler(file_handler)
        root_logger.addHandler(console_handler)
        logger = logging.getLogger('module_logger')
        logger.critical('Error: ' + str(e) + ' \n'
                        + '                                     '
                        + 'Node "console_output" not found in module configuration file. \n'
                        + '                                     ' + 'Program aborted!')
        sys.exit(1)

    match console_output:
        # Only 'CRITICAL' logs displayed.
        case 'mode_0':
            console_handler.setLevel(logging.CRITICAL)
        # Logs of type 'CRITICAL', 'ERROR', 'PRINTOUT', and 'WARNING' displayed.
        case 'mode_1':
            console_handler.setLevel(logging.WARNING)
        # Logs of type 'CRITICAL', 'ERROR', 'PRINTOUT', 'WARNING', and 'INFO' displayed.
        case 'mode_2':
            console_handler.setLevel(logging.INFO)
        # Logs of type 'CRITICAL', 'ERROR', 'PRINTOUT', 'WARNING', 'INFO', and 'DEBUG' displayed.
        case 'mode_3':
            console_handler.setLevel(logging.DEBUG)

    # Disable colorization for the console handler.
    console_handler.setStream(stream=sys.stdout)

    # Attach both handlers to the 'root_logger'.
    root_logger.addHandler(file_handler)
    root_logger.addHandler(console_handler)
