#include "array.h"
#include "windows.h"

Array<char *> win32_list_all_files_in_directory(char * directory, bool search_recursively = true);
