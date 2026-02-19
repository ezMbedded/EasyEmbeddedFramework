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
logger = logging.getLogger('TEST_TEMPLATE_GENERATOR')
logger.setLevel(logging.INFO)
logger.propagate = False

ch = logging.StreamHandler()
formatter = logging.Formatter('%(name)s::%(funcName)s::%(levelname)s::%(message)s')
ch.setFormatter(formatter)
logger.addHandler(ch)


# Create the parser
my_parser = argparse.ArgumentParser(prog = 'Module Source header template generator',
                                    description='Create the header and source file')

_file_header ="""\
/*****************************************************************************
* Filename:         unittest_{0}
* Author:           {1}
* Original Date:    {2}
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/
"""

_doxygen_file_header=\
"""
/** @file   unittest_{0}
 *  @author {1}
 *  @date   {2}
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 * 
 */
"""

_doxygen_source_body=\
"""
/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "unity.h"
#include "unity_fixture.h"
"""


_test_file_body=\
"""
TEST_GROUP({0});

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
/* None */

/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/* None */

/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);

/******************************************************************************
* External functions
*******************************************************************************/
int main(int argc, const char *argv[])
{{
    return UnityMain(argc, argv, RunAllTests);
}}


TEST_SETUP({0})
{{
}}


TEST_TEAR_DOWN({0})
{{
}}


TEST_GROUP_RUNNER({0})
{{
    RUN_TEST_CASE({0}, TestTempPlate);
}}


TEST({0}, TestTempPlate)
{{
    TEST_ASSERT_EQUAL(false, false);
}}


/******************************************************************************
* Internal functions
*******************************************************************************/
static void RunAllTests(void)
{{
    RUN_TEST_GROUP({0});
}}


/* End of file */
"""

def _generate_header(file_path : str, author : str, module : str, generated_date : str):
    with open(file_path, "a") as header:
        header.write(_file_header.format(module + ".c",author, generated_date))
        header.write(_doxygen_file_header.format(module + ".c", author, generated_date))
        header.write(_doxygen_source_body.format(module))
    logger.info("complete")
        

def generate_test_file(path : str, module:str, author:str):
    """generate unit test template

    Args:
        path (str): where the uniti test is stored
        module (str): name of the moculde under test
        author (str): author of this unit test
    """
    logger.info("generating source file")
    now = datetime.now()
    dt_string = now.strftime("%d.%m.%Y")
    path = os.path.join(path, "unittest_" + module + ".c")
    _generate_header(path, author, module, dt_string)
    
    with open(path, "a") as body:
        body.write(_test_file_body.format(module))


def main():
    """main, entry point of the application
    """
    # read arguments
    my_parser.add_argument( '-m',
                        '--module', 
                        action='store', 
                        type=str, 
                        help='name of the module, which the test will be crated for')

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
    args = my_parser.parse_args()

    logger.info("author: {}".format(args.author))
    logger.info("filename: {}".format(args.module))
    logger.info("destination: {}".format(args.destination))

    generate_test_file(args.destination, args.module, args.author)

if __name__ == "__main__":
    main()