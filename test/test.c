#include "../bin/include/cdtp.h"
#include <stdio.h>

void tmp(int a, void *b, void *c)
{
    printf("tmp function called\n");
}

int main(int argc, char **argv)
{
    printf("Running tests...\n");

    CDTPServer server = cdtp_server_default(tmp, tmp, tmp, NULL, NULL, NULL);

    printf("Successfully passed all tests\n");
    return 0;
}
