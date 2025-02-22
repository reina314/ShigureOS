#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/// @brief Create an ordered array
/// @param address Start address of the array
/// @param max_size 
/// @param less_than 
/// @return 
ordered_array_t place_ordered_array(void *address, uint32_t max_size, lessthan_predicate_t less_than) {
    ordered_array_t ret;
    ret.array = (type_t *)address;
    memset(ret.array, 0, max_size * sizeof(type_t)); // Initialize with 0
    ret.size = 0;
    ret.max_size = max_size;
    ret.less_than = less_than;
    return ret;
}

/// @brief Add an item to the array
/// @param item
/// @param array
void insert_ordered_array(type_t item, ordered_array_t *array)
{
    ASSERT(array->less_than);

    uint32_t iterator = 0;
    while (iterator < array->size && array->less_than(array->array[iterator], item))
    {
        iterator++;
    }

    if (iterator == array->size) // Add at the end of the array
    {
        array->array[array->size++] = item;
    }
    else
    {
        type_t tmp = array->array[iterator];
        array->array[iterator] = item;
        while (iterator < array->size)
        {
            iterator++;
            type_t tmp2 = array->array[iterator];
            array->array[iterator] = tmp;
            tmp = tmp2;
        }

        array->size++;
    }
}

/// @brief Remove item with given index from the array
/// @param index
/// @param array
void remove_ordered_array(uint32_t index, ordered_array_t *array)
{
    while (index < array->size)
    {
        array->array[index] = array->array[index + 1];
        index++;
    }

    array->size--;
}

/// @brief Look up the item at given index; if index > size then panic
/// @param index
/// @param array
/// @return Returns array->array[index]
type_t lookup_ordered_array(uint32_t index, ordered_array_t *array)
{
    ASSERT(index < array->size);
    return array->array[index];
}