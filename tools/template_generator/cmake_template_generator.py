__author__ =        "Hai Nguyen"
__credits__ =       "Hai Nguyen"
__license__ =       "This file is published under the license described in LICENSE.md"
__maintainer__ =    "Hai Nguyen"
__email__ =         "hainguyen.eeit@gmail.com"


import logging
import argparse
import os
from datetime import datetime

# create logger
logger = logging.getLogger('CMAKE_TEMPLATE_GENERATOR')
logger.setLevel(logging.INFO)
logger.propagate = False

ch = logging.StreamHandler()
formatter = logging.Formatter('%(name)s::%(funcName)s::%(levelname)s::%(message)s')
ch.setFormatter(formatter)
logger.addHandler(ch)

cmake_header="""\
# ----------------------------------------------------------------------------
# Author: {0}
# Name: {1}_{2}
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------
"""

cmake_lib="""
add_library({0}_{1} STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating {0}_{1} library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${{CMAKE_SOURCE_DIR}}/easy_embedded)

"""

cmake_test_start="""
add_executable({0}_{1})

message(STATUS "**********************************************************")
message(STATUS "* Generating {0}_{1} build files")
message(STATUS "**********************************************************")

"""


cmake_lib_body="""
# Source files ---------------------------------------------------------------
target_sources({0}_{1}
    PRIVATE
        # Please add source files here
)


# Definitions ----------------------------------------------------------------
target_compile_definitions({0}_{1}
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories({0}_{1}
    PUBLIC
        ${{CMAKE_CURRENT_LIST_DIR}}
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries({0}_{1}
    PUBLIC
        # Please add public libraries
    PRIVATE
        # Please add private libraries
    INTERFACE
        # Please add interface libraries
)

# End of file
"""


cmake_test_body="""
# Source files ---------------------------------------------------------------
target_sources({0}_{1}
    PRIVATE
        unittest_{0}.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions({0}_{1}
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories({0}_{1}
    PUBLIC
        # Please add private folders here
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries({0}_{1}
    PUBLIC
        # Please add public libraries
    PRIVATE
        unity
        easy_embedded_lib
    INTERFACE
        # Please add interface libraries
)

add_test(NAME {0}_{1}
    COMMAND {0}_{1}
)

# End of file
"""

# Create the parser
my_parser = argparse.ArgumentParser(prog = 'cmake template generator',
                                    description='Create a template for cmake file')

def is_tartget_name_valid(name:str) -> bool:
    """check if the target contain space or undercore. Do not check for
    special characters and co because it assumes that the users must know
    about cmake and special characters are not allowed.

    Args:
        name (str): target name

    Returns:
        bool: True if name is valid, else False
    """
    if " " in name or "-" in name:
        return False
    return True

def write_text_to_cmake(author:str, name:str, file_handle, target_type:str)->bool:
    """write text to the cmake file

    Args:
        author (str): author's name
        name (str): target's name
        file_handle (_type_): file handle to write text data
        target_type (str): type of the target, accept only lib, exec or test
    Returns:
        bool: True if success, else false
    """
    if target_type == "lib" or target_type == "exec" or target_type == "test":
        file_handle.write(cmake_header.format(author, name, target_type))
        if target_type == "lib":
            file_handle.write(cmake_lib.format(name, target_type))
            file_handle.write(cmake_lib_body.format(name, target_type))
        elif target_type == "test":
            file_handle.write(cmake_test_start.format(name, target_type))
            file_handle.write(cmake_test_body.format(name, target_type))
        else:
            logger.warning("Unsupported")
        return True
    else:
        return False


def generate_cmake_lib(author:str, name:str, path:str) -> bool:
    """Generate a cmake file for a library

    Args:
        author (str): author's name
        name (str): target's name
        path (str): path of the result CMake

    Returns:
        bool: True if success, else false
    """
    if is_tartget_name_valid(name) == True:
        path = os.path.join(path, "CMakeLists.txt")
        with open(path, "+a") as file:
            return write_text_to_cmake(author, name, file, "lib")
    else:
        logger.error("invalid targer name")
    return False


def generate_cmake_test(author:str, name:str, path:str) -> bool:
    """Generate a cmake file for a test

    Args:
        author (str): author's name
        name (str): target's name
        path (str): path of the result CMake

    Returns:
        bool: True if success, else false
    """
    if is_tartget_name_valid(name) == True:
        path = os.path.join(path, "CMakeLists.txt")
        with open(path, "+a") as file:
            return write_text_to_cmake(author, name, file, "test")
    else:
        logger.error("invalid targer name")
    return False

def generate_cmake_execute(author:str, name:str, path:str) -> bool:
    """Generate a cmake file for a test

    Args:
        author (str): author's name
        name (str): target's name
        path (str): path of the result CMake

    Returns:
        bool: True if success, else false
    """
    if is_tartget_name_valid(name) == True:
        path = os.path.join(path, "CMakeLists.txt")
        with open(path, "+a") as file:
            return write_text_to_cmake(author, name, file, "test")
    else:
        logger.error("invalid targer name")
    return False


def main():
    """main, entry point of the application
    """
    # read arguments
    my_parser.add_argument( '-a',
                            '--author', 
                            action='store', 
                            type=str, 
                            help='name of the author of the module')

    my_parser.add_argument( '-d',
                            '--destination', 
                            action='store', 
                            type=str, 
                            help='where the files will be saved')

    my_parser.add_argument("-l",
                            "--library",
                            action="store_true",
                            dest="library",
                            default="False",
                            help="flag to indicate this cmake file is for a library")
    
    my_parser.add_argument("-e",
                            "--executable",
                            dest="executable",
                            default="False",
                            action="store_true",
                            help="flag to indicate this cmake file is for an executable")

    my_parser.add_argument("-t",
                            "--test",
                            dest="test",
                            default="False",
                            action="store_true",
                            help="flag to indicate this cmake file is for a unit test")


    my_parser.add_argument( "-n",
                            "--name",
                            action="store",
                            type=str,
                            help="name of the library or executable")
    args = my_parser.parse_args()

    logger.info("author: {}".format(args.author))
    logger.info("destination: {}".format(args.destination))
    logger.info("name: {}".format(args.name))

    if ((args.library and args.executable) or (args.library and args.test) or (args.executable and args.test)) == True:
        logger.error("More than one option is activated")
    elif args.library == True:
        logger.info("cmake for library")
        generate_cmake_lib(args.author, args.name, args.destination)
    elif args.test == True:
        logger.info("cmake for test")
        generate_cmake_test(args.author, args.name, args.destination)
    elif args.executable == True:
        logger.warning("unsupported")
    else:
        pass

if __name__ == "__main__":
    main()