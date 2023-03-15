#include <stdio.h>
#include <stdlib.h>

#include "argo.h"
#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    if(validargs(argc, argv) == -1)
    {
        USAGE(*argv, EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    if(global_options == HELP_OPTION)
    {
        USAGE(*argv, EXIT_SUCCESS);
        return EXIT_SUCCESS;
    }

    if(global_options == VALIDATE_OPTION)
    {
        if(argo_read_value(stdin) == NULL)
        {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    ARGO_VALUE *arVal = argo_read_value(stdin);
    if(arVal == NULL)
    {
        return EXIT_FAILURE;
    }

    if(argo_write_value(arVal, stdout) == 0)
    {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
