#include "include/syn-plus.h"
#include <stdio.h>

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("Please specify file input\n");
        return 1;
    }
    
    syn_compile_file(argv[1]);
    return 0;
}