#include "file_loader.h"
#include "stdio.h"
#include "string.h"
#include "macros.h"

Array<char *> win32_list_all_files_in_directory(char * directory, bool search_recursively) { // @Default search_recursively = true
    Array<char *> files;

	WIN32_FIND_DATA result;

    char path[256];
    snprintf(path, 256, "%s\\*", directory);
    HANDLE handle = FindFirstFile(path, &result); // MSDN has interesting details about this.

	if (handle == INVALID_HANDLE_VALUE) {
		log_print("list_files", "Directory %s doesn't exist, is empty, or might be protected. No files will be returned.", directory);
		return files;
	}

	bool done = false;

    while(!done) {

        char * file_name = result.cFileName;

        if(strlen(file_name) <= 2) {
            if((strcmp(file_name, ".") == 0) || (strcmp(file_name, "..") == 0)) {
                done = !(FindNextFile(handle, &result));
                continue;
            }
        }

        char full_path[256];
        snprintf(full_path, 256, "%s/%s", directory, file_name);

        if(result.dwFileAttributes &= FILE_ATTRIBUTE_DIRECTORY) {

            if(search_recursively) {
                Array<char *> subtree = win32_list_all_files_in_directory(full_path);

                // @Speed, could do a single memcpy here.
                for_array(subtree.data, subtree.count) {
                    files.add(*it);
                }
            }

            done = !(FindNextFile(handle, &result));
            continue;
        }

        files.add(strdup(full_path));

        done = !(FindNextFile(handle, &result));
    }

	FindClose(handle);

    return files;
}
