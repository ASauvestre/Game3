#include <assert.h>
#include <windows.h>
#include <stdio.h>

#include "macros.h"
#include "asset_manager.h"
#include "parsing.h"

struct Directory {
    char * name;
    HANDLE handle;

    FILE_NOTIFY_INFORMATION * notifications;
    OVERLAPPED overlapped;
};

struct AssetChange {
    String file_name; // Subset of full_path
    String full_path; // On the heap
};

// Private functions
static void issue_read_directory(Directory * directory);

static void handle_notifications();

static FILE_NOTIFY_INFORMATION * bump_ptr_to_next_notification(FILE_NOTIFY_INFORMATION * notification);

// Const
const int NOTIFICATION_BUFFER_LENGTH = 10000; // @Temporary figure out how much space is actually reasonably required

// Globals
static Directory dir;

static Array<AssetChange> asset_changes;

static Array<AssetManager *> managers;

void release(AssetChange * ac) {
    free(ac->full_path.data);
}

void init_hotloader() {
    dir.name = "data";

    // Allocate space for windows' notifications
    dir.notifications = (FILE_NOTIFY_INFORMATION *) malloc(NOTIFICATION_BUFFER_LENGTH);

    // Create event needed for overlapped operation
    HANDLE event = CreateEvent(NULL, false, false, NULL);
    if(event == NULL) {
        log_print("init_hotloader", "Failed to create event with CreateEvent. PANIC");
        assert(false); // @Temporary, simply disable hotloading in that case.
    }

    // Fill overlapped struct
    dir.overlapped.Offset = 0;
    dir.overlapped.hEvent = event;

    // Get directory handle
    HANDLE handle = CreateFileA(dir.name, FILE_LIST_DIRECTORY,
                                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    if(handle == INVALID_HANDLE_VALUE) {
        log_print("init_hotloader", "Failed to get a handle to the %s directory with CreateFile. PANIC", dir.name);
        assert(false);
    }

    dir.handle = handle;

    // Successfully prepped the directory, now let's read it.
    issue_read_directory(&dir);
}

void register_manager(AssetManager * am) {
    managers.add(am);
}

void check_hotloader_modifications() {
    handle_notifications();

    for(int i = 0; i < asset_changes.count; i++) { // for_array
        AssetChange * change = &asset_changes.data[i];

        String extension = find_char_from_left('.', change->file_name);

        // Look for a manager who's interested in changes of files with this extension
        for_array(managers.data, managers.count) {
            AssetManager * am = *it;
            for(int j = 0; j < am->extensions.count; j++) { // @Cleanup This should be another for_array, but I can't nest them without redeclaring it and it_index, which is no good. We should be able to name it manually
                if(extension == am->extensions.data[j]) {
                    // printf("Match\n");
                    am->assets_to_reload.add(change->full_path);
                }
            }
        }
    }

    asset_changes.reset(true);
}

static void handle_notifications() {
    // Check if the read request has completed or not
    if(!HasOverlappedIoCompleted(&dir.overlapped)) {
        return;
    }

    int bytes_transferred;
    bool success = GetOverlappedResult(dir.handle, &dir.overlapped, (LPDWORD) &bytes_transferred, false);
    // @Note last bool of previous function is set to false, meaning that if the operation is
    // not complete, it will fail, it should be complete though since the call to the macro returned true.

    if(success == false) {
        log_print("check_hotloader_modifications", "Failed to get notification. PANIC");
        assert(false);
    }

    // Issue next read request
    issue_read_directory(&dir);

    // No notifications to report
    if(bytes_transferred == 0) {
        return;
    }

    // log_print("check_hotloader_modifications", "Hotloader notification, bytes_transferred = %d", bytes_transferred);

    // Fill our own notification struct, AssetChange
    AssetChange change;

    FILE_NOTIFY_INFORMATION * notification = dir.notifications;
    while(notification != NULL) {
        if      (notification->Action == FILE_ACTION_MODIFIED)         {} // @Incomplete, maybe send the action to the relevant catalogs
        else if (notification->Action == FILE_ACTION_ADDED)            {} // @Incomplete, maybe send the action to the relevant catalogs
        else if (notification->Action == FILE_ACTION_RENAMED_NEW_NAME) {} // @Incomplete, maybe send the action to the relevant catalogs
        else {
            // log_print("check_hotloader_modifications", "Ignored action %d, NextEntryOffset is %d", notification->Action, notification->NextEntryOffset);
            notification = bump_ptr_to_next_notification(notification);
            continue;
        }

        const int NAME_BUFFER_LENGTH = 1000;
        char name_buffer[NAME_BUFFER_LENGTH];

        int path_length = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, notification->FileName,
                                         notification->FileNameLength / sizeof(WCHAR),
                                         name_buffer, NAME_BUFFER_LENGTH, NULL, NULL);

        if(path_length == 0) {
            // log_print("check_hotloader_modifications", "Failed to convert filename. PANIC");
            assert(false);
        }

        notification = bump_ptr_to_next_notification(notification); // We'll bump it now to allow easy early-outs later

        change.full_path.data = (char *) malloc(path_length);
        memcpy(change.full_path.data, name_buffer, path_length);

        change.full_path.count = path_length;

        // Replace Windows' \ by /
        for (int i = 0; i < path_length; i++) {
            if (change.full_path[i] == '\\') change.full_path[i] = '/';
        }

        // Isolate file name from the path
        String file_name = find_char_from_right('/', change.full_path);
        if (file_name.count) {
            change.file_name = file_name;
        } else {
			change.file_name = change.full_path;
		}

		String extension = find_char_from_left('.', change.file_name);

		if (extension.count) { // Check that we have an extension
			if (extension == "tmp") continue; // PS tmp file. We skip those.
			// change.file_name = change.full_path;
		} else {
			// printf("Change in directory %s, skipping notification\n", to_c_string(change.full_path));
			continue; // Directory change, we ignore that.
		}

        // Check uniqueness @Meh
        bool success = true;
        for_array(asset_changes.data, asset_changes.count) {
			if (it->full_path == change.full_path) {
                success = false;
                it_index = asset_changes.count; // Break
            }
        }

        if(success) {
            // printf("Added %s\n", to_c_string(change.full_path));
            asset_changes.add(change);
        } else {
            // printf("Skipped duplicate %s\n", to_c_string(change.full_path));

        }
    }
}

static FILE_NOTIFY_INFORMATION * bump_ptr_to_next_notification(FILE_NOTIFY_INFORMATION * notification) {
    int offset = notification->NextEntryOffset;

    // Last notification
    if(offset == 0) return NULL;

    return (FILE_NOTIFY_INFORMATION *)((char *)notification + notification->NextEntryOffset);
}

static void issue_read_directory(Directory * directory) {
    bool success = ReadDirectoryChangesW(directory->handle, directory->notifications,
                                         NOTIFICATION_BUFFER_LENGTH, true,
                                         FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                         NULL, &directory->overlapped, NULL);

    if(!success) {
        log_print("issue_read_directory", "Failed to issue read request to directory %s", directory->name);
    }
}

void shutdown_hotloader() {
    CloseHandle(dir.handle);
    free(dir.notifications);
}
