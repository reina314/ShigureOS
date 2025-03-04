#define _XOPEN_SOURCE 500 // Ensure compatibility for nftw()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ftw.h>

#define INITRD_MAGIC 0xDEAD
#define INITRD_MAX_FILES 128
#define AFILE 0x1
#define DIRECTORY 0x2

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

initrd_file_header_t files[INITRD_MAX_FILES];
uint32_t file_count = 0, current_offset = 0;
FILE *image;

/// @brief Callback for ftw()
/// @param path
/// @param sb
/// @param typeflag
/// @param ftwbuf
/// @return
int process_entry(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    sb = sb; // Avoid warning

    if (file_count > INITRD_MAX_FILES)
        return 1;

    initrd_file_header_t *f = &files[file_count];
    f->magic = INITRD_MAGIC;
    f->number = file_count;

    // Determine parent number
    if (ftwbuf->level == 0)
        f->parent_number = 0;
    else
    {
        for (int i = file_count - 1; i >= 0; i--)
        {
            if (files[i].type == DIRECTORY && ftwbuf->level > 1)
            {
                f->parent_number = files[i].number;
                break;
            }
        }
    }
    strncpy(f->name, path + 7, 63); // Strip initrd/ prefix

    if (typeflag == FTW_D)
    {
        f->type = DIRECTORY;
        f->offset = 0;
        f->length = 0;
    }
    else if (typeflag == FTW_F)
    {
        f->type = AFILE;
        FILE *fp = fopen(path, "rb");
        if (!fp)
            return 1;

        fseek(fp, 0, SEEK_END);
        f->length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        f->offset = current_offset;
        char *buffer = malloc(f->length);
        size_t bytesRead = fread(buffer, 1, f->length, fp);
        if (bytesRead < sizeof(buffer) && ferror(fp))
            perror("Error reading file");
        fwrite(buffer, 1, f->length, image);
        fclose(fp);
        free(buffer);

        current_offset += f->length;
    }

    file_count++;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <output.img>\n", argv[0]);
        return 1;
    }

    image = fopen(argv[1], "wb");
    if (!image)
    {
        perror("fopen");
        return 1;
    }

    fseek(image, sizeof(initrd_header_t) + sizeof(initrd_file_header_t) * INITRD_MAX_FILES, SEEK_SET);
    current_offset = (uint32_t)(sizeof(initrd_header_t) + sizeof(initrd_file_header_t) * INITRD_MAX_FILES);
    nftw("initrd", process_entry, 10, FTW_PHYS);

    fseek(image, 0, SEEK_SET);
    initrd_header_t header = {.nfiles = file_count};
    fwrite(&header, sizeof(header), 1, image);
    fwrite(files, sizeof(initrd_file_header_t), file_count, image);

    fclose(image);
    printf("Initrd build successfully : %s\n", argv[1]);
    return 0;
}