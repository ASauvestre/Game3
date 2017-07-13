#include <assert.h>
#include <windows.h>
#include <stdio.h>
#include <vector>

#include "macros.h"

struct Directory {
    char * name;
    HANDLE handle;

    FILE_NOTIFY_INFORMATION * notifications;
    OVERLAPPED overlapped;
};

enum AssetChangeAction {
    ADDED,
    MODIFIED,
    RENAMED
};

struct AssetChange {
    char * file_name; // Subset of full_path, no need to free
    char * full_path; // Needs to be freed

    AssetChangeAction action;
};

// Private functions
static void issue_read_directory(Directory * directory);

static void handle_notifications();

static char * find_char_from_right(char c, char * string);

static FILE_NOTIFY_INFORMATION * bump_ptr_to_next_notification(FILE_NOTIFY_INFORMATION * notification);

// Const
const int NOTIFICATION_BUFFER_LENGTH = 10000; // @Temporary figure out how much space is actually reasonably required

// Globals
static Directory dir;

static std::vector<AssetChange> asset_changes;

void release(AssetChange * ac) {
    free(ac->full_path);
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

void check_hotloader_modifications() {
    handle_notifications();

    for(int i = 0; i < asset_changes.size(); i++) {
        AssetChange * change = &asset_changes[i];

        log_print("check_hotloader_modifications", "Detected modication on file %s", change->file_name);

        release(change);
    }

    asset_changes.clear();
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
        if      (notification->Action == FILE_ACTION_ADDED)            change.action = ADDED;
        else if (notification->Action == FILE_ACTION_MODIFIED)         change.action = MODIFIED;
        else if (notification->Action == FILE_ACTION_RENAMED_NEW_NAME) change.action = ADDED;
        else {
            // log_print("check_hotloader_modifications", "Ignored action %d", notification->Action);
            notification = bump_ptr_to_next_notification(notification);
            continue;
        }

        const int NAME_BUFFER_LENGHT = 1000;
        char name_buffer[NAME_BUFFER_LENGHT];

        int result = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, notification->FileName, 
                                         notification->FileNameLength / sizeof(WCHAR),
                                         name_buffer, NAME_BUFFER_LENGHT, NULL, NULL);

        if(result == 0) {
            log_print("check_hotloader_modifications", "Failed to convert filename. PANIC");
            assert(false);
        }

        change.full_path = (char *) malloc(result + 1);
        memcpy(change.full_path, name_buffer, result);
        
        // Adding 0-termination to the string
        change.full_path[result] = 0;

        // log_print("check_hotloader_modifications", "Detected action %d on file %s", change.action, change.full_path);

        notification = bump_ptr_to_next_notification(notification);

		char * t = find_char_from_right('/', change.full_path);
		if (t) {
			change.file_name = t + 1;
		}
		else {
			change.file_name = change.full_path;
		}

        asset_changes.push_back(change);
    }
}

static char * find_char_from_right(char c, char * string) {
    char * cursor = string;
    char * last_char_occurence = NULL;

    // Compute length;
    while(*cursor != '\0') {
        if(*cursor == c) {
            last_char_occurence = cursor;
        }
        cursor += 1; // Move to next character
    }

    return last_char_occurence;
}

static FILE_NOTIFY_INFORMATION * bump_ptr_to_next_notification(FILE_NOTIFY_INFORMATION * notification) {
    int offset = notification->NextEntryOffset;

    // Last notification
    if(offset == 0) return NULL;

    return *(&notification + notification->NextEntryOffset);
}

static void issue_read_directory(Directory * directory) {
    bool success = ReadDirectoryChangesW(directory->handle, directory->notifications, 
                                         NOTIFICATION_BUFFER_LENGTH, true, 
                                         FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE, 
                                         NULL, &directory->overlapped, NULL);

    if(!success) {
        log_print("issue_read_directory", "Failed to issue read request to directory %s", directory->name);      
    }
}

void shutdown_hotloader() {
    CloseHandle(dir.handle);
    free(dir.notifications);
}