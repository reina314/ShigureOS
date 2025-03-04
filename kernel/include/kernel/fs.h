#ifndef _KERNEL_FS_H
#define _KERNEL_FS_H

#include <stdlib.h>
#include <stdint.h>

// Node types
#define FS_FILE 0x01
#define FS_DIRECTORY 0x02
#define FS_CHARDEVICE 0x04
#define FS_BLOCKDEVICE 0x08
#define FS_PIPE 0x10
#define FS_SYMLINK 0x20
#define FS_MOUNTPOINT 0x40 // Active mountpoint or not

struct fs_node;

// Type of callbacks
// Improve extensibility by polymorphism
typedef uint32_t (*read_t)(struct fs_node *, uint32_t, size_t, uint8_t *);
typedef uint32_t (*write_t)(struct fs_node *, uint32_t, size_t, uint8_t *);
typedef void (*open_t)(struct fs_node *);
typedef void (*close_t)(struct fs_node *);
typedef struct dirent *(*readdir_t)(struct fs_node *, uint32_t);
typedef struct fs_node *(*finddir_t)(struct fs_node *, char *);

typedef struct fs_node
{
    char name[128];        // Filename
    uint32_t mask;         // Permission mask
    uint8_t uid;           // Owner user
    uint8_t gid;           // Owner group
    uint8_t flags;         // Node type defined above
    uint32_t inode;        // Used by a filesystem to identify files
    uint32_t parent_inode; // Parent's inode
    uint32_t length;       // Size of file in bytes
    read_t read;           // Read callback func
    write_t write;         // Write callback func
    open_t open;           // Open callback func
    close_t close;         // Close callback func
    readdir_t readdir;     // Readdir callback func
    finddir_t finddir;     // Finddir callback func
    struct fs_node *ptr;   // Used by mountpoints and symlinks
} fs_node_t;

typedef struct dirent
{
    char name[128]; // Filename
    uint32_t inode; // Inode number
} dirent_t;

uint32_t read_fs(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buffer);
uint32_t write_fs(fs_node_t *node, uint32_t offset, size_t size, uint8_t *buffer);
void open_fs(fs_node_t *node, uint8_t read, uint8_t write);
void close_fs(fs_node_t *node);
dirent_t *readdir_fs(fs_node_t *node, uint32_t index);
fs_node_t *finddir_fs(fs_node_t *node, char *name);
void list_fs(void);

#endif