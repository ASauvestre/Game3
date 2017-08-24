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
	room->dimensions.x  = -1;
	room->dimensions.y  = -1;

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

    // Parse version number @Refactor
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

    int line_number = 1;
    bool successfully_parsed_file = true;

    while(true) {
        String line = bump_to_next_line(&file_data);
        line_number += 1;

        if(line.count == -1) break; // EOF

        if(line.count == 0) continue; // Empty line

        String field_name = cut_until_space(&line);

        if(field_name == "name") {
            String arg = cut_until_space(&line);

            if(!arg.count) {
                log_print("do_load_room", "Expected a name after \"name\" on line %d of file %s.", line_number, room->name);
            }

            if(line.count) {
                char * c_line = to_c_string(line);
                scope_exit(free(c_line));

                log_print("do_load_room", "Garbage left on line %d of file %s after the name: \"%s\".", line_number, room->name, c_line);

                successfully_parsed_file = false;
                continue;
            }

            // @Incomplete, do something with the name

        } else if(field_name == "dimensions") {
            Vector2 dimensions;
            bool success = string_to_v2(line, &dimensions);

            if(!success) {
                char * c_line = to_c_string(line);
                scope_exit(free(c_line));

                log_print("do_load_room", "Failed to parse dimensions on line %d of file %s, string was \"%s\".", line_number, room->name, c_line);

                successfully_parsed_file = false;
                continue;
            }

            new_room.dimensions = dimensions;
        } else if(field_name == "tile") {
            // Oh boy.
        } else {
                char * c_field = to_c_string(field_name);
                scope_exit(free(c_field));

                char * c_line = to_c_string(line);
                scope_exit(free(c_line));
                log_print("do_load_room", "Unknown field      \"%s\"     on line %d of file %s, remainder was \"%s\".", c_field, line_number, room->name, c_line);

                continue; // Not failing, we just ignore this line.
        }
    }

    if(successfully_parsed_file) {
        *room = new_room;
        log_print("do_load_room", "Successfully parsed dimensions of room in file %s, they're (%d, %d) ", room->name, room->dimensions.width, room->dimensions.height);
    }
}
