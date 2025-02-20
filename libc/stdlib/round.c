#include <stdlib.h>

/// @brief Rounds an unsigned int up to the nearest multiple of a given number
/// @param numToRound
/// @param multiple For example, 8, 16 or more common number for memory things
/// @return
unsigned int round_up_to_multiple(unsigned int numToRound, unsigned int multiple)
{
    if (multiple == 0)
    {
        return numToRound;
    }

    unsigned int remainder = numToRound % multiple;
    if (remainder == 0)
    {
        return numToRound;
    }

    return numToRound + multiple - remainder;
}