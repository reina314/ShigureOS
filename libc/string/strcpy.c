#include <string.h>

/// @brief Copy characters from source to destination
/// @param dstptr Destination to copy characters into
/// @param srcptr Pointer to character (string) to copy
void strcpy(char *dstptr, const char *srcptr)
{
    int i = 0;
    while ((dstptr[i] = srcptr[i]) != '\0')
        i++;
}