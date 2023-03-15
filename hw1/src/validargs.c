#include <stdlib.h>

#include "argo.h"
#include "global.h"
#include "debug.h"

static int strcmp(char* a, char* b);
int tryGetPNum(char *input);
/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */

int validargs(int argc, char **argv) {

    global_options = (0x0);

    if(argc < 2 || argc > 4)
    {
        return -1;
    }

    char *flagH, *flagV, *flagC, *flagP;
    flagH = "-h";
    flagV = "-v";
    flagC = "-c";
    flagP = "-p";
    
    if(strcmp(*(argv+1), flagH) == 0 && strcmp(*(argv+1), flagV) == 0 && strcmp(*(argv+1), flagC) == 0)
    {
        return -1;
    }
    if(strcmp(*(argv+1), flagH) == 1)
    {
        global_options = HELP_OPTION;
        return 0;
    }
    if(strcmp(*(argv+1), flagV) == 1)
    {
        if(argc > 2)
        {
            return -1;
        }
        global_options = VALIDATE_OPTION;
        return 0;
    }

    // if argv[1] == "-c"
    if(strcmp(*(argv+1), flagC) == 1)
    {
        if(argc > 2) // if there is more argument.
        {
            if(strcmp(*(argv+2), flagP)) // if argv[2] == "-p"
            {
                if(argc > 3) // if there is more argument.
                {
                    int pNum = tryGetPNum(*(argv+3)); // try get argv[3]. if not number: -1 // if number: number of indent.
                    if(pNum < 0) // if not number
                    {
                        return -1;
                    }
                    global_options = CANONICALIZE_OPTION + PRETTY_PRINT_OPTION + pNum;
                    return 0;
                }
                // if no number provided after "-p" flag;
                global_options = CANONICALIZE_OPTION + PRETTY_PRINT_OPTION + 4;
                return 0;
            }
            // if argv[2] != "-p"
            else
            {
                return -1;
            }
        }
        // if only "-c" provided.
        global_options = CANONICALIZE_OPTION;
        return 0;
    }
    return 0;
    abort();
}

static int strcmp(char* a, char* b)
{
    int flag = 1;
    int index = 0;
    while(flag == 1)
    {
        if(*(a+index) == '\0' && *(b+index) == '\0')
        {
            return 1;
        }
        if(*(a+index) == '\0' || *(b+index) == '\0')
        {
            return 0;
        }
        if(*(a+index) != *(b+index))
        {
            return 0;
        }
        index = index + 1;
    }
    if(index >= 10)
    {
        return 0;
    }
    return 0;
}

int tryGetPNum(char* input)
{
    int index = 0;

    int result = 0;

    if(*input == '\0')
    {
        return -1;
    }

    // while end of string
    while(*(input + index) != '\0')
    {
        // if char is number
        if(*(input+index) >= '0' && *(input+index) < '9')
        {
            // if input[0] == 0 and number comes out after 0. ex) 01, 02, 00  (not integer)
            if(index > 0 && *input == '0')
            {
                return -1;
            }
            // add number to result
            int curNum = *(input+index) - '0';
            result  = result*10 + curNum;
            index++;
        }
        // if char isnot number
        else
        {
            return -1;
        }
    }
    // if number exceeds 11111111
    if(result > 255)
    {
        return 255;
    }
    return result;
}
