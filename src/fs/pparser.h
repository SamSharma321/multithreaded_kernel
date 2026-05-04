#ifndef PPARSER_H_
#define PPARSER_H_

// This will be a linked list based inmplementation
// Each time new folder is created, the next variable will be updated in the structure of that directory

/* 0:/test/test.txt 
 Here:
    0 - path root
    test - first (path directory)
    test.txt - path to file or next. */
struct path_root {
    int drive_no;
    struct part_path* first;
};

struct part_path {
    const char* part;           // Why constant pointer here?
    struct part_path* next;
};

struct part_path* pathparser_parser_path_part(struct part_path* last_part, const char** path);
void pathparser_free_root(struct path_root* root);
void pathparser_free_part(struct part_path* part);
struct path_root* pathparser_parse(const char* path, const char* current_directory_path);

#endif