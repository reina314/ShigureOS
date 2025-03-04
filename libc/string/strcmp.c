#include <string.h>

/// @brief Compare characters between aptr and bptr
/// @param aptr
/// @param bptr
/// @return Return 0 if the same; otherwise difference between two characters
int strcmp(const char *aptr, const char *bptr)
{
    while (*aptr)
    {
        if (*aptr != *bptr)
            break;

        aptr++;
        bptr++;
    }

    return *(const unsigned char *)aptr - *(const unsigned char *)bptr;
}