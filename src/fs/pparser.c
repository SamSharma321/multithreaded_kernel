#include "pparser.h"
#include "kernel.h"
#include "string/string.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"

static int pathparser_path_valid_format(const char* filename) {
    int len = strnlen(filename, SAMOS_MAX_PATH);
    return (len >= 3) && (isdigit(filename[0])) && (memcmp((void*)&filename[1], ":/", 2) == 0);
}

static int pathparser_get_drive_by_path(const char** path) {
    if(!pathparser_path_valid_format(*path))
        return -EBADPATH;

    int drive_no = tonumericdigit(*path[0]);
    // Skip the first three byte to skip 0:/ 1:/ 2:/
    *path += 3; // Modification done here
    return drive_no;
}

// Function to create a path root based on provided root number
static struct path_root* pathparser_create_root(int drive_number) {
    struct path_root* path_r = kzalloc(sizeof(struct path_root));
    path_r->drive_no = drive_number;
    path_r->first = 0; // NULL initially - no directories present
    return path_r;
}

/* Returns the next immediate folder or directory or file */
static const char* pathparser_get_path_part(const char** path) {
    char* result = kzalloc(SAMOS_MAX_PATH);
    int i = 0;
    while ((**path != '/') && (**path != 0x00)) {
        result[i] = **path;
        (*path)++;
        i++;
    }
    if (**path == '/')
        // skip the forward slash to avoid infinite loop
        (*path)++;
    if (i == 0) {   // See if no directory present - free unused memory
        kfree(result);
        result = 0;
    }
    return result;
}

/* This function does the following: 
    1. Create a part of exists
    2. Attach it to last provided lick or part path */
struct part_path* pathparser_parser_path_part(struct part_path* last_part, const char** path) {
    const char* path_part_str = pathparser_get_path_part(path);
    if (!path_part_str) {
        return 0;
    }
    struct part_path* part = kzalloc(sizeof(struct part_path));
    part->part = path_part_str;
    part->next = 0x00;
    if (last_part) {
        last_part->next = part;
    }
    return part;
}

/* Function to free the structures of the path including the root directory. */
void pathparser_free_root(struct path_root* root) {
    struct part_path* part = root->first;
    while(part) {
        struct part_path* next_part = part->next;
        kfree((void*)part->part);
        kfree(part);
        part = next_part;
    }
    kfree(root);
}

/* Function to free the part definde by the path and its dependents. */
void pathparser_free_part(struct part_path* part) {
    while(part) {
        struct part_path* next_part = part->next;
        kfree((void*)part->part);
        kfree(part);
        part = next_part;
    }
}

/* Function to parse and create the necessary structures for the path defined. */
struct path_root* pathparser_parse(const char* path, const char* current_directory_path) {
    const char* temp_path = path;
    struct path_root* path_r = 0;
    if (strlen(path) > SAMOS_MAX_PATH)
        goto out;
    int res = pathparser_get_drive_by_path(&temp_path);
    if (res < 0)
        goto out;
    path_r = pathparser_create_root(res);
    if (!path_r)
        goto out;
    struct part_path* first_part = pathparser_parser_path_part(NULL, &temp_path);
    if (!first_part)
        goto out;
    path_r->first = first_part;
    struct part_path* part = pathparser_parser_path_part(first_part, &temp_path);
    while (part) {
        part = pathparser_parser_path_part(part, &temp_path);
    }
    // Successfully parsed status
    return path_r;
out:
    return 0;
}