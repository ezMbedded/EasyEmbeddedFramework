__author__ =        "Hai Nguyen"
__credits__ =       "Hai Nguyen"
__license__ =       "This file is published under the license described in LICENSE.md"
__maintainer__ =    "Hai Nguyen"
__email__ =         "hainguyen.eeit@gmail.com"

import logging
import argparse
import os
import re
from enum import Enum
from pathlib import Path

# create logger
logger = logging.getLogger('GENERATOR')
logger.setLevel(logging.DEBUG)
logger.propagate = False

ch = logging.StreamHandler()
formatter = logging.Formatter('%(name)s::%(funcName)s::%(levelname)s::%(message)s')
ch.setFormatter(formatter)
logger.addHandler(ch)

# Create the parser
my_parser = argparse.ArgumentParser(description='Generate header file containing activated features from cmake file')

class EXTRACT_RESULT(Enum):
    COMMENT=1
    FEATURE=2
    EMPTY_LINE=3
    ERROR=4

def extract_feature_by_line(feature_str:str):
    """Check the input line and try to extract the feature from that line

    Args:
        feature_str (str): input string for feature extraction

    Returns:
        _type_: (EXTRACT_RESULT, output string)
    """
    if feature_str == "":
        logger.info("found an empty line")
        return (EXTRACT_RESULT.EMPTY_LINE, None)
    elif feature_str[0] == '#':
        logger.info("found a comment")
        ret = "\n/* {} */\n".format(feature_str[1:].lstrip())
        return (EXTRACT_RESULT.COMMENT, ret)
    else:
        # tokenize string from option(ENABLE_EZ_EVENT_NOTIFIER    "Enable the Event Notifier module"       ON)
        # to ["option", "ENABLE_EZ_EVENT_NOTIFIER    "Enable the Event Notifier module"       ON", ""]
        tokens = re.split("[()]", feature_str)
        if len(tokens) == 3 and tokens[0] == "option":
            # tokenize string from "ENABLE_EZ_EVENT_NOTIFIER    "Enable the Event Notifier module"       ON"
            # to ["ENABLE_EZ_EVENT_NOTIFIER    ", "Enable the Event Notifier module", "       ON"]
            tokens = tokens[1].split("\"")
            if len(tokens) == 3:
                # cleanup the whitespace
                module_name = tokens[0].rstrip()[7:] # remove the first 7 characters "ENABLE_"
                activate_status = tokens[2].lstrip()
                description = tokens[1]
                if activate_status == "ON":
                    ret = "#define {} 1 /* {} */\n".format(module_name, description)
                    return (EXTRACT_RESULT.FEATURE, ret)
                elif activate_status == "OFF":
                    ret = "#define {} 0 /* {} */\n".format(module_name, description)
                    return (EXTRACT_RESULT.FEATURE, ret)
                else:
                    return (EXTRACT_RESULT.ERROR, None)
            else:
                return (EXTRACT_RESULT.ERROR, None)

def generate_output_header(input_path:str, output_path:str)->bool:
    """Generate the header file from the cmake feature file

    Args:
        input_path (str): path to cmake file
        output_path (str): path to output header file

    Returns:
        bool: True if success else False
    """
    with open(input_path, "r") as cmake_file:
        if os.path.exists(output_path):
            logger.warning("file exists. This file will be overwritten")
            os.remove(output_path)
        with open(output_path, "a") as header_file:
            text = cmake_file.read()
            lines = text.splitlines()
            header_file.write("#ifndef _{}_H\n".format(Path(output_path).stem.upper()))
            header_file.write("#define _{}_H\n".format(Path(output_path).stem.upper()))
            for line in lines:
                result, feature = extract_feature_by_line(line)
                if result == EXTRACT_RESULT.ERROR:
                    logger.error("Invalid format")
                    exit()
                elif result == EXTRACT_RESULT.FEATURE:
                    header_file.write(feature)
                    logger.debug(feature)
                elif result == EXTRACT_RESULT.COMMENT:
                    header_file.write(feature)
                    logger.debug(feature)
                else:
                    pass # do nothing
            header_file.write("\n#endif\n")
    logger.info("Generate success")

def main():
    """main, entry point of the application
    """
    # read arguments
    my_parser.add_argument( '-f',
                            '--cmake_file',
                            action='store',
                            required=True,
                            type=str,
                            help='name of the cmake file')

    my_parser.add_argument( '-o',
                            '--output',
                            action='store',
                            required=True,
                            type=str, 
                            help='path to output file')

    args = my_parser.parse_args()
    logger.debug("Input = {}".format(args.cmake_file))
    logger.debug("Output = {}".format(args.output))
    if os.path.exists(args.cmake_file) == False:
        logger.error("Path does not exist")
    else:
        generate_output_header(args.cmake_file, args.output + "/ez_target_config.h")

if __name__ == "__main__":
    main()