#include <kernel/fs.h>
#include <stdint.h>
#include <stdio.h> // For putchar()

/// @brief Root of the filesystem
fs_node_t *fs_root = 0;

/// @brief List all contents in the initrd
void list_fs(void)
{
    int i = 0;
    dirent_t *node = 0;

    while ((node = readdir_fs(fs_root, i)) != 0)
    {
        fs_node_t *fsnode = finddir_fs(fs_root, node->name);

        if (fsnode->flags == FS_DIRECTORY)
            ;
        else
        {
            uint8_t buffer[256];
            for (int i = 0; i < 256; i++)
                buffer[i] = 'X';

            // Read the contents of the file into buffer
            uint32_t size = read_fs(fsnode, 0, 256, buffer);

            for (int j = 0; j < (int)size; j++)
                putchar(buffer[j]);
        }

        i++;
    }
}

/// @brief Read a file into buffer
/// @param node File node to read
/// @param offset Offset to read file
/// @param size Size to read in bytes
/// @param buffer Pointer to buffer to read a file into
/// @return Size read in bytes; 0 if not readable
uint32_t read_fs(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buffer)
{
    // Check if the node has read callback
    if (node->read != 0)
        return node->read(node, offset, size, buffer);
    else
        return 0;
}

/// @brief Write data from a buffer into a file
/// @param node File node to write into
/// @param offset Offset to write data
/// @param size Size to write in bytes
/// @param buffer Pointer to buffer to write data from
/// @return Size written in bytes; 0 if not writable
uint32_t write_fs(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buffer)
{
    // Check if the node has write callback
    if (node->write != 0)
        return node->write(node, offset, size, buffer);
    else
        return 0;
}

/// @brief Open a file
/// @param node File node to open
/// @param read --
/// @param write --
void open_fs(fs_node_t *node, uint8_t read, uint8_t write)
{
    // Avoid warnings
    read = read;
    write = write;
    if (node->open != 0)
        return node->open(node);
}

/// @brief Close a file
/// @param node File node to close
void close_fs(fs_node_t *node)
{
    if (node->close != 0)
        return node->close(node);
}

/// @brief Read a directory
/// @param node Dir node to read
/// @param index
/// @return NULL if not readable
dirent_t *readdir_fs(fs_node_t *node, uint32_t index)
{
    if (node->flags == FS_DIRECTORY && node->readdir != 0)
        return node->readdir(node, index);
    else
        return NULL;
}

/// @brief Find a node with a specific name
/// @param node Node to search within
/// @param name Name of dir to search for
/// @return Dir node; NULL if error
fs_node_t *finddir_fs(fs_node_t *node, char *name)
{
    if (node->flags == FS_DIRECTORY && node->finddir != 0)
        return node->finddir(node, name);
    else
        return NULL;
}