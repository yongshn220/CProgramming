/*********************/
/* errmsg.c          */
/* for Par 3.20      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include "errmsg.h"  /* Makes sure we're consistent with the declarations. */

static char *errmsg = NULL;
const char * const outofmem = "Out of memory.\n";

void set_error(char *msg)
{
	clear_error();
	errmsg = calloc(strlen(msg),sizeof(char));
	errmsg = strdup(msg);
}

int is_error()
{
	if(errmsg)
	{
		return 1;
	}
	return 0;
}

int report_error(FILE *file)
{
	if(fprintf(file, "%s",errmsg) > 0)
	{
		clear_error();
		return 0;
	}
	clear_error();
	return -1;
}

void clear_error()
{
	if(errmsg)
	{
		free(errmsg);
	}
}
