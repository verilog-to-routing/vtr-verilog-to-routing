#include <stdio.h>

int main()
{
    if (sizeof(void*) == 8)     // Assume 64-bit Linux if pointers are 8 bytes.
        printf("-DLIN64 ");
    else
        printf("-DLIN ");

    printf("-DSIZEOF_VOID_P=%d -DSIZEOF_LONG=%d -DSIZEOF_INT=%d\n",
           (int)sizeof(void*),
           (int)sizeof(long),
           (int)sizeof(int) );


    return 0;
}
