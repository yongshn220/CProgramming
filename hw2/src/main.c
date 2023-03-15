#include <stdlib.h>

extern int original_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    original_main(argc, argv);
}
