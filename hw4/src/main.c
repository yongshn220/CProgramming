#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "mush.h"

#include "data.h"
#include "sdata.h"

int main(int argc, char *argv[]) {
    prog_init();
    store_init();

    jobs_init();
    exec_interactive();
    jobs_fini();
}
