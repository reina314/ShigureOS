#ifndef _KERNEL_INITRD_H
#define _KERNEL_INITRD_H

#include <stdlib.h>
#include "fs.h"

#define INITRD_MAGIC 0xDEAD
// File types
#define AFILE 0x01
#define DIRECTORY 0x02

typedef struct initrd_header
{
    uint32_t nfiles; // The number of files in the ramdisk
} initrd_header_t;

typedef struct initrd_file_header
{
    uint16_t magic;        // For error checking
    char name[64];         // Filename
    uint32_t offset;       // Offset in the initrd the file starts
    uint32_t length;       // The size of this file in bytes
    int32_t parent_number; // Identification number of parent dirs
    uint32_t number;       // Identification of this file
    uint8_t type;          // Filetype defined in fs.h
} initrd_file_header_t;

fs_node_t *initrd_initialize(uint32_t multiboot_addr);

#endif