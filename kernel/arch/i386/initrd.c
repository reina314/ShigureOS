#include <kernel/initrd.h>
#include <kernel/vm.h> // For kmalloc()
#include <stdint.h>
#include <stdio.h> // For debug
#include <string.h>

/// @brief Header
initrd_header_t *initrd_header;
/// @brief Pointer to list of file_header_t
initrd_file_header_t *file_headers;

/// @brief Root dir node
fs_node_t *initrd_root;
/// @brief Directory for mounting filesystem
fs_node_t *initrd_dev;
/// @brief Pointer to list of fs_node_t
fs_node_t *root_nodes;
/// @brief Number of files nodes
uint32_t nnodes;

/// @brief The node of version.txt file
fs_node_t *version_node;

/// @brief Read a file pointed to by node into a buffer; used to read file contents
/// @param node File node to read
/// @param offset Offset to start reading from; start_addr = file_addr + offset
/// @param size Size to read; in bytes
/// @param buffer The pointer to copy data into
/// @return Size read; 0 if error
static uint32_t initrd_read(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buffer)
{
    initrd_file_header_t header = file_headers[node->inode];
    if (offset > header.length)
        return 0;

    if (offset + (uint32_t)size > header.length)
        size = (size_t)(header.length - offset);

    memcpy(buffer, (uint8_t *)(header.offset + offset), size); // header.offset is file_addr
    return size;
}

/// @brief Return dir entries from node; used to list dir entries
/// @param node Directory node to list from
/// @param index
/// @return The pointer to dir entry; NULL otherwise
static dirent_t *initrd_readdir(fs_node_t *node, uint32_t index)
{
    static dirent_t dirent; // Ensure it persists between function calls

    if (node == initrd_root)
    {
        if (index == 0)
        {
            strcpy(dirent.name, "dev");
            dirent.name[3] = 0; // Might not be necessary
            dirent.inode = 0;
            return &dirent;
        }
        index--; // Adjust for dev entry
    }

    if (index >= nnodes)
        return NULL;

    strcpy(dirent.name, root_nodes[index].name);
    dirent.name[strlen(root_nodes[index].name)] = 0; // Might not be necessary
    dirent.inode = root_nodes[index].inode;
    return &dirent;
}

/// @brief Find a node with a specific name in a node; used to find a file by name
/// @param node
/// @param name
/// @return The pointer to node; NULL otherwise
static fs_node_t *initrd_finddir(fs_node_t *node, char *name)
{
    if (node == initrd_root && !strcmp(name, "dev"))
        return initrd_dev;

    for (uint32_t i = 0; i < nnodes; i++)
    {
        if (!strcmp(name, root_nodes[i].name))
            return &root_nodes[i];
    }

    return NULL;
}

/// @brief Initialize ramdisk in specified memory address
/// @param multiboot_addr Memory address to initialize ramdisk
/// @return The pointer to root node
fs_node_t *initrd_initialize(uint32_t multiboot_addr)
{
    // Initialize header and file header pointers
    initrd_header = (initrd_header_t *)multiboot_addr;
    file_headers = (initrd_file_header_t *)(multiboot_addr + sizeof(initrd_header_t));

    // Initialize the root dir
    initrd_root = (fs_node_t *)kmalloc(sizeof(fs_node_t));
    strcpy(initrd_root->name, "root");
    initrd_root->mask = initrd_root->uid = initrd_root->gid = initrd_root->inode = initrd_root->length = 0;
    initrd_root->flags = FS_DIRECTORY;
    initrd_root->read = 0;
    initrd_root->write = 0;
    initrd_root->open = 0;
    initrd_root->close = 0;
    initrd_root->readdir = &initrd_readdir;
    initrd_root->finddir = &initrd_finddir;
    initrd_root->ptr = 0;

    // Initialize the dev dir
    initrd_dev = (fs_node_t *)kmalloc(sizeof(fs_node_t));
    strcpy(initrd_dev->name, "dev");
    initrd_dev->mask = initrd_dev->uid = initrd_dev->gid = initrd_dev->inode = initrd_dev->length = 0;
    initrd_dev->flags = FS_DIRECTORY;
    initrd_dev->read = 0;
    initrd_dev->write = 0;
    initrd_dev->open = 0;
    initrd_dev->close = 0;
    initrd_dev->readdir = &initrd_readdir;
    initrd_dev->finddir = &initrd_finddir;
    initrd_dev->ptr = 0;

    // Page fault because cpu interprets initrd_header as virtual address rather than physical address
    // so map these memory area identically or get corresponding virtual address
    root_nodes = (fs_node_t *)kmalloc(sizeof(fs_node_t) * initrd_header->nfiles);
    nnodes = initrd_header->nfiles;

    // Edit file header of every files in initrd
    for (uint32_t i = 0; i < nnodes; i++)
    {
        // Check if magic number is valid
        if (file_headers[i].magic != INITRD_MAGIC)
            continue;

        // Convert offset so it is relative to the start of memory rather than start of initrd
        file_headers[i].offset += multiboot_addr;

        // Create new file node
        strcpy(root_nodes[i].name, file_headers[i].name);

        // Save version.txt
        if (strcmp(root_nodes[i].name, "version.txt") == 0)
            version_node = &root_nodes[i];

        root_nodes[i].mask = root_nodes[i].uid = root_nodes[i].gid = 0;
        root_nodes[i].length = file_headers[i].length;
        root_nodes[i].inode = file_headers[i].number;
        root_nodes[i].parent_inode = file_headers[i].parent_number;
        root_nodes[i].read = &initrd_read;
        root_nodes[i].write = 0;
        root_nodes[i].readdir = 0;
        root_nodes[i].finddir = 0;
        root_nodes[i].open = 0;
        root_nodes[i].close = 0;
        root_nodes[i].ptr = 0;

        switch (file_headers[i].type)
        {
        case AFILE:
            root_nodes[i].flags = FS_FILE;
            break;

        case DIRECTORY:
            root_nodes[i].flags = FS_DIRECTORY;
            break;

        default:
            break;
        }
    }

    return initrd_root;
}