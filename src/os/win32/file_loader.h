#include "array.h"
#include "windows.h"
#include "parsing.h"

String win32_read_file(String path);
Array<char *> win32_list_all_files_in_directory(char * directory, bool search_recursively = true);
