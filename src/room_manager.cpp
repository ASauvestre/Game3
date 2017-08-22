#include "room_manager.h"
#include "macros.h"
#include "parsing.h"
#include "os/layer.h"

void RoomManager::init() {
    this->extensions.add("room");
}

void RoomManager::create_placeholder(char * name, char * path) {
    Room * room = (Room * ) malloc(sizeof(Room));

    room->name          = name;
    room->full_path     = path;

    this->table.add(name, room);
}

// @Think, make this part of AssetManager_Poly ?
void RoomManager::reload_or_create_asset(String full_path, String file_name) {
    char * c_file_name = to_c_string(file_name);
    char * c_full_path = to_c_string(full_path);

    Asset * asset = this->table.find(c_file_name);

    if(!asset) {
        create_placeholder(c_file_name, c_full_path);
        asset = this->table.find(c_file_name);
    } else {
        free(c_file_name);
        free(c_full_path);
    }

    do_load_room((Room *) asset);
}

void RoomManager::do_load_room(Room * room) {
    String file_data = os_specific_read_file(room->full_path);
    scope_exit(free(file_data.data));

    Room new_room = *room;

    // Parse version number
    {
        String line = bump_to_next_line(&file_data);
        if(line.count < 0) {
            log_print("do_load_room", "Failed to parse %s. It's empty.", room->name);
        }

        String token = cut_until_space(&line);

        if(token != "VERSION") {
            log_print("do_load_room", "Failed to parse the \"VERSION\" token at the beginning of the file %s. Please make sure it is there.", room->name);
            return;
        }

        String version_token = cut_until_space(&line);


        if(!version_token.count) {
            log_print("do_load_room", "We parsed the \"VERSION\" token at the beginning of the file %s, but no version number was provided after it. Please make sure to put it on the same line.", room->name);
            return;
        }

        if(line.count) {
            char * c_line = to_c_string(line);
            scope_exit(free(c_line));

            log_print("do_load_room", "There is garbage left on the first line of file %s after the version number: %s", room->name, c_line);

            return;
        }


        if(version_token.data[0] != '<' || version_token.data[version_token.count - 1] != '>') {
            char * c_version_token = to_c_string(version_token);
            scope_exit(free(c_version_token));

            log_print("do_load_room", "The version number after \"VERSION\" in file %s needs to be surrounded with '<' and '>', instead, we got this : %s", room->name, c_version_token);

            return;
        }

        // Isolating version number
        push(&version_token);
        version_token.count -= 1;

        int version;
        bool success = string_to_int(version_token, &version);

        if(!success) {
            char * c_version_token = to_c_string(version_token);
            scope_exit(free(c_version_token));

            log_print("do_load_room", "We expected a version number after \"VERSION\" in file %s, but instead we got this: %s.", room->name, c_version_token);

            return;
        }
    }

    skip_empty_lines(&file_data);
    String line = bump_to_next_line(&file_data);

    if(line.count < 0) {
        log_print("do_load_room", "Failed to parse %s. It's empty.", room->name);
        return;
    }

    String name = cut_until_space(&line);
    if(line.count) {
        char * c_line = to_c_string(line);
        scope_exit(free(c_line));

        log_print("do_load_room", "We parsed a name for file %s, but the was garbage left on the line: %s", room->name, c_line);

        return;
    }

    // Nothing. // @Temporary, we actually don't care about the name, at least for now, since it's being occupied but the name of the file.

    skip_empty_lines(&file_data);
    line = bump_to_next_line(&file_data);

    if(line.count < 0) {
        log_print("do_load_room", "Failed to parse %s. It ended before we could find the height and width of the room.", room->name);
        return;
    }

    bool success;
    String width_token = cut_until_space(&line);

    success = string_to_int(width_token, &new_room.width);

    if(!success) {
        char * c_token = to_c_string(width_token);
        scope_exit(free(c_token));

        log_print("do_load_room", "We failed to parse the width of the room in file %s, make sure it's an integer. Got %s", room->name, c_token);

        return;
    }

    if(!line.count) {
        log_print("do_load_room", "We parsed the width of the room in file %s, but there was no height given on the same line.", room->name);
        return;
    }

    String height_token = cut_until_space(&line);

    success = string_to_int(height_token, &new_room.height);

    if(!success) {
        char * c_token = to_c_string(height_token);
        scope_exit(free(c_token));

        log_print("do_load_room", "We failed to parse the height of the room in file %s, make sure it's an integer. Got %s", room->name, c_token);

        return;
    }

    if(line.count) {
        char * c_line = to_c_string(line);
        scope_exit(free(c_line));

        log_print("do_load_room", "We parsed height and width in file %s, but the was garbage left on the line: %s", room->name, c_line);

        return;
    }

    log_print("do_load_room", "Successfully parsed dimensions of room in file %s, they're (%d, %d) ", room->name, new_room.width, new_room.height);

}
