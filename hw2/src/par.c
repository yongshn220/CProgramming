/*********************/
/* par.c             */
/* for Par 3.20      */
/* Copyright 1993 by */
/* Adam M. Costello  */
/*********************/

/* This is ANSI C code. */


#include "errmsg.h"
#include "buffer.h"    /* Also includes <stddef.h>. */
#include "reformat.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <getopt.h> // yj added

#undef NULL
#define NULL ((void *) 0)

int getopt_long_c(int argc, char ** argv, int *widthbak, int *prefixbak,
  int *suffixbak, int *hangbak, int *lastbak, int *minbak);

const char * const progname = "par";
const char * const version = "3.20";
static char *tmperrm = NULL;

static int digtoint(char c)

/* Returns the value represented by the digit c,   */
/* or -1 if c is not a digit. Does not use errmsg. */
{
  return c == '0' ? 0 :
         c == '1' ? 1 :
         c == '2' ? 2 :
         c == '3' ? 3 :
         c == '4' ? 4 :
         c == '5' ? 5 :
         c == '6' ? 6 :
         c == '7' ? 7 :
         c == '8' ? 8 :
         c == '9' ? 9 :
         -1;

  /* We can't simply return c - '0' because this is ANSI  */
  /* C code, so it has to work for any character set, not */
  /* just ones which put the digits together in order.    */
}


static int strtoudec(const char *s, int *pn)

/* Puts the decimal value of the string s into *pn, returning */
/* 1 on success. If s is empty, or contains non-digits,       */
/* or represents an integer greater than 9999, then *pn       */
/* is not changed and 0 is returned. Does not use errmsg.     */
{
  int n = 0;

  if (!*s) return 0;

  do {
    if (n >= 1000 || !isdigit(*s)) return 0;
    n = 10 * n + digtoint(*s);
  } while (*++s);

  *pn = n;

  return 1;
}


static void parseopt(
  const char *opt, int *pwidth, int *pprefix,
  int *psuffix, int *phang, int *plast, int *pmin
)
/* Parses the single option in opt, setting *pwidth, *pprefix,     */
/* *psuffix, *phang, *plast, or *pmin as appropriate. Uses errmsg. */
{
  const char *saveopt = opt;
  char oc;
  int n, r;

  if (*opt == '-') ++opt;

  if (!strcmp(opt, "version")) {
    tmperrm = calloc(100, sizeof(char));
    sprintf(tmperrm, "%s %s\n", progname, version);
    set_error(tmperrm);
    free(tmperrm);
    return;
  }

  oc = *opt;

  if (isdigit(oc)) {
    if (!strtoudec(opt, &n)) goto badopt;
    if (n <= 8) *pprefix = n;
    else *pwidth = n;
  }
  else {
    if (!oc) goto badopt;
    n = 1;
    r = strtoudec(opt + 1, &n);
    if (opt[1] && !r) goto badopt;

    if (oc == 'w' || oc == 'p' || oc == 's') {
      if (!r) goto badopt;
      if      (oc == 'w') *pwidth  = n;
      else if (oc == 'p') *pprefix = n;
      else                *psuffix = n;
    }
    else if (oc == 'h') *phang = n;
    else if (n <= 1) {
      if      (oc == 'l') *plast = n;
      else if (oc == 'm') *pmin = n;
    }
    else goto badopt;
  }

  //*errmsg = '\0';
  return;

badopt:
  tmperrm = calloc(100, sizeof(char));
  sprintf(tmperrm, "Bad option: %.149s\n", saveopt);
  set_error(tmperrm);
  free(tmperrm);
}


static char **readlines(void)

/* Reads lines from stdin until EOF, or until a blank line is encountered, */
/* in which case the newline is pushed back onto the input stream. Returns */
/* a NULL-terminated tmparray of pointers to individual lines, stripped of    */
/* their newline characters. Uses errmsg, and returns NULL on failure.     */
{
  struct buffer *cbuf = NULL, *pbuf = NULL;
  int c, blank;
  char ch, *ln = NULL, *nullline = NULL, nullchar = '\0', **lines = NULL;

  cbuf = newbuffer(sizeof (char));
  if (is_error()) goto rlcleanup;
  pbuf = newbuffer(sizeof (char *));
  if (is_error()) goto rlcleanup;

  for (blank = 1;  ; ) {
    c = getchar();
    if (c == EOF) break;
    if (c == '\n') {
      if (blank) {
        ungetc(c,stdin);
        break;
      }
      additem(cbuf, &nullchar);
      if (is_error()) goto rlcleanup;
      ln = copyitems(cbuf);
      if (is_error()) goto rlcleanup;
      additem(pbuf, &ln);
      if (is_error()) goto rlcleanup;

      clearbuffer(cbuf);
      blank = 1;
    }
    else {
      if (!isspace(c)) blank = 0;
      ch = c;
      additem(cbuf, &ch);
      if (is_error()) goto rlcleanup;
    }
  }

  if (!blank) {
    additem(cbuf, &nullchar);
    if (is_error()) goto rlcleanup;
    ln = copyitems(cbuf);
    if (is_error()) goto rlcleanup;
    additem(pbuf, &ln);
    if (is_error()) goto rlcleanup;
  }

  additem(pbuf, &nullline);
  if (is_error()) goto rlcleanup;
  lines = copyitems(pbuf);


rlcleanup:
  // if(ln)
  // {
  //   free(ln);
  // }
  if (cbuf) freebuffer(cbuf);
  if (pbuf) {
    if (!lines){
      for (;;) {
        lines = nextitem(pbuf);
        if (!lines) break;
        free(*lines);
      }
    }
    freebuffer(pbuf); // (yongjung: added)
  }

  return lines;
}


static void setdefaults(
  const char * const *inlines, int *pwidth, int *pprefix,
  int *psuffix, int *phang, int *plast, int *pmin
)
/* If any of *pwidth, *pprefix, *psuffix, *phang, *plast, *pmin are     */
/* less than 0, sets them to default values based on inlines, according */
/* to "par.doc". Does not use errmsg because it always succeeds.        */
{
  int numlines;
  const char *start, *end, * const *line, *p1, *p2;

  if (*pwidth < 0) *pwidth = 72;
  if (*phang < 0) *phang = 0;
  if (*plast < 0) *plast = 0;
  if (*pmin < 0) *pmin = *plast;

  for (line = inlines;  *line;  ++line);
  numlines = line - inlines;

  if (*pprefix < 0)
  {
    if (numlines <= *phang + 1)
    {
      *pprefix = 0;
    }
    else
    {
      start = inlines[*phang];
      for (end = start;  *end;  ++end);
      for (line = inlines + *phang + 1;  *line;  ++line)
      {
        for (p1 = start, p2 = *line;  p1 < end && *p1 == *p2;  ++p1, ++p2);
        end = p1;
      }
      *pprefix = end - start;
    }
  }

  if (*psuffix < 0)
  {
    if (numlines <= 1)
    {
      *psuffix = 0;
    }
    else
    {
      start = *inlines;
      for (end = start;  *end;  ++end);
      for (line = inlines + 1;  *line;  ++line)
      {
        for (p2 = *line;  *p2;  ++p2);
        for (p1 = end;
             p1 > start && p2 > *line && p1[-1] == p2[-1];
             --p1, --p2);
        start = p1;
      }
      while (end - start >= 2 && isspace(*start) && isspace(start[1])) ++start;
      *psuffix = end - start;
    }
  }
}


static void freelines(char **lines)
/* Frees the strings pointed to in the NULL-terminated array lines, then */
/* frees the array. Does not use errmsg because it always succeeds.      */
{
  char *line;

  // for (line = *lines;  *line;  ++line){
  //     free(line);
  // }
  int num = 0;
  for(;;){
    line = *(lines + num);
    if(line)
    {
      free(line);
    }
    else
    {
      break;
    }
    num++;
  }
  free(lines);
}

int getopt_long_c(int argc, char ** argv, int *widthbak, int *prefixbak,
  int *suffixbak, int *hangbak, int *lastbak, int *minbak)
{
  int c;
  int prev_optind = -1;


  while (1) {
    //int this_option_optind = optind ? optind : 1;
    int option_index = 0;

    int int_optarg = -1;


    static struct option long_options[] = {
        {"version",     no_argument, 0,  'v' },
        {"width",  required_argument, NULL,  'w' },
        {"prefix",  required_argument, NULL,  'p' },
        {"suffix", required_argument, NULL,  's' },
        {"hang",  required_argument, NULL, 'h'},
        {"last",    no_argument, NULL,  't' },
        {"no-last",    no_argument, NULL, 'x' },
        {"min",    no_argument, NULL,  'n' },
        {"no-min",    no_argument, NULL,  'y' },
        {0,         0,                 0,  0 }
    };

    if(optind)
    {
      prev_optind = optind;
    }
    c = getopt_long(argc, argv, "-:w:p:s:h:l:m:tnxy", long_options, &option_index);


    if (c == -1){
      break;
    }

    switch (c) {
      case '\1':
        if(isdigit(*optarg))
        {
          strtoudec(optarg, &int_optarg);
          if(int_optarg >= 9 && int_optarg <= 9999)
          {
            *widthbak = int_optarg;
          }
          else if (int_optarg >= 0 && int_optarg < 9)
          {
            *prefixbak = int_optarg;
          }
          else
          {
            goto error_section;
          }
        }
        else
        {
          goto error_section;
        }
        break;
      case ':':
        if(strcmp("-h", argv[optind - 1]) == 0)
        {
          *hangbak = 1;
        }
        else if(strcmp("-l", argv[optind - 1]) == 0)
        {
          *lastbak = 1;
        }
        else if(strcmp("-m", argv[optind - 1]) == 0)
        {
          *minbak = 1;
        }
        else
        {
          goto error_section;
        }
        break;
      case 'v':
        printf("par 3.20\n");
        return -1;
        break;
      case 'w':
        if (isdigit(*optarg)){
          strtoudec(optarg, &int_optarg);
          if(int_optarg == -1)
          {
            goto error_section;
          }
          *widthbak = int_optarg;
        }
        else
        {
          goto error_section;
        }
        break;
      case 'p':
        if (isdigit(*optarg)){
          strtoudec(optarg, &int_optarg);
          if(int_optarg == -1)
          {
            goto error_section;
          }
          *prefixbak = int_optarg;
        }
        else
        {
          goto error_section;
        }
        break;
      case 's':
        if (isdigit(*optarg)){
          strtoudec(optarg, &int_optarg);
          if(int_optarg == -1)
          {
            goto error_section;
          }
          *suffixbak = int_optarg;
        }
        else
        {
          goto error_section;
        }
        break;
      case 'h':
        if (isdigit(*optarg)){
          strtoudec(optarg, &int_optarg);
          if(int_optarg == -1)
          {
            goto error_section;
          }
          *hangbak = int_optarg;
        }
        else if(optind - prev_optind == 2)
        {
          *hangbak = 1;
          optind--;
        }
        else
        {
          goto error_section;
        }
        break;
      case 'l':
        if (isdigit(*optarg)){
          strtoudec(optarg, &int_optarg);
          if(int_optarg == 0 || int_optarg == 1)
          {
            *lastbak = int_optarg;
          }
          else
          {
            goto error_section;
            break;
          }
        }
        else if(optind - prev_optind == 2)
        {
          *lastbak = 1;
          optind--;
        }
        else
        {
          goto error_section;
        }
        break;
      case 'm':
        if (isdigit(*optarg)){
          strtoudec(optarg, &int_optarg);
          if(int_optarg == 0 || int_optarg == 1)
          {
            *minbak = int_optarg;
          }
          else
          {
            goto error_section;
          }
        }
        else if(optind - prev_optind == 2)
        {
          *minbak = 1;
          optind--;
        }
        else
        {
          goto error_section;
        }
        break;

      case 't':
        *lastbak = 1;
        break;
      case 'n':
        *minbak = 1;
        break;
      case 'x':
        *lastbak = 0;
        break;
      case 'y':
        *minbak = 0;
        break;
      case '?':
        goto error_section;
        break;

      default:
        printf("?? getopt returned character code 0%o ??\n", c);
    }
  }
  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc){
      printf("%s ", argv[optind++]);
    }
    printf("\n");
  }
  return 0;

  error_section:
  printf("Invalid option.\n");
  return -1;
}

//int original_main(int argc, const char * const *argv)
int original_main(int argc, char *argv[])
{
  int width, widthbak = -1, prefix, prefixbak = -1, suffix, suffixbak = -1,
      hang, hangbak = -1, last, lastbak = -1, min, minbak = -1, c;
  char *parinit, *picopy = NULL, *opt, **inlines = NULL, **outlines = NULL,
       **line;
  const char * const whitechars = " \f\n\r\t\v";

  parinit = getenv("PARINIT");
  if (parinit) {
    picopy = malloc((strlen(parinit) + 1) * sizeof (char));
    if (!picopy) {
      tmperrm = calloc(100, sizeof(char));
      strcpy(tmperrm, outofmem);
      set_error(tmperrm);
      free(tmperrm);
      goto parcleanup;
    }
    strcpy(picopy,parinit);
    opt = strtok(picopy,whitechars);
    while (opt) {
      parseopt(opt, &widthbak, &prefixbak,
               &suffixbak, &hangbak, &lastbak, &minbak);
      if (is_error()) goto parcleanup;
      opt = strtok(NULL,whitechars);
    }
    free(picopy);
    picopy = NULL;
  }

  // yj added
  int glcr = getopt_long_c(argc, argv, &widthbak, &prefixbak,
             &suffixbak, &hangbak, &lastbak, &minbak);

  if(glcr == -1)
  {
    exit(EXIT_SUCCESS);
  }

/*
  while (*++argv) {
    parseopt(*argv, &widthbak, &prefixbak,
             &suffixbak, &hangbak, &lastbak, &minbak);
    if (*errmsg) goto parcleanup;
  }
*/


  for (;;){
    for (;;) {
      c = getchar();
      if (c != '\n') break;
      putchar(c);
    }
    ungetc(c,stdin);

    inlines = readlines();
    if (is_error()) goto parcleanup;
    if (!*inlines) {
      free(inlines);
      inlines = NULL;
      break;
    }

    width = widthbak;  prefix = prefixbak;  suffix = suffixbak;
    hang = hangbak;  last = lastbak;  min = minbak;
    setdefaults((const char * const *) inlines,
                &width, &prefix, &suffix, &hang, &last, &min);

    outlines = reformat((const char * const *) inlines,
                        width, prefix, suffix, hang, last, min);
    if (is_error()) goto parcleanup;

    freelines(inlines);
    inlines = NULL;

    for (line = outlines;  *line;  ++line)
      puts(*line);

    freelines(outlines);
    outlines = NULL;
  }


parcleanup:

  if (picopy) free(picopy);
  if (inlines) freelines(inlines);
  if (outlines) freelines(outlines);

  if (is_error()) {
    report_error(stderr);
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
