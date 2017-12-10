#include "room_manager.h"
#include "macros.h"
#include "parsing.h"
#include "os/layer.h"

void RoomManager::init() {
    this->extensions.add("room");
}

void RoomManager::create_placeholder(String name, String path) {
    Room * room = (Room * ) malloc(sizeof(Room));

    room->name            = name;
    room->full_path       = path;

    // @Cleanup Those init shouldn't be here, either make a manual initializer or use new instead of malloc.

    // Init position vector
    room->dimensions = { -1, -1 };

    // Init arrays
    room->tiles            = {};
    room->collision_blocks = {};

    this->table.add(name, room);
}

// @Think, make this part of AssetManager_Poly ?
void RoomManager::reload_or_create_asset(String full_path, String file_name) {
    Asset * asset = this->table.find(file_name);

    if(!asset) {
        create_placeholder(file_name, full_path);
        asset = this->table.find(file_name);
    } else {
        free(full_path.data);
    }

    do_load_room((Room *) asset);
}

void RoomManager::do_load_room(Room * room) {
    char * c_name = to_c_string(room->name);
    scope_exit(free(c_name));

    String file_data = os_specific_read_file(room->full_path);
    scope_exit(free(file_data.data));

    if (!file_data.data) return; // Should have already errored.

    Array<String> lines = strip_comments_from_file(file_data);

    Room new_room = *room;

    Array<Tile> new_tile_array;
    Array<CollisionBlock> new_collision_block_array;

    // Parse version number @Refactor
    {
        if(lines.count <= 0) {
            log_print("do_load_room", "Failed to parse %s. It's empty.", c_name);
        }

        String line = lines.data[0];

        String token = cut_until_space(&line);

        if(token != "VERSION") {
            log_print("do_load_room", "Failed to parse the \"VERSION\" token at the beginning of the file %s. Please make sure it is there.", c_name);
            return;
        }

        String version_token = cut_until_space(&line);


        if(!version_token.count) {
            log_print("do_load_room", "We parsed the \"VERSION\" token at the beginning of the file %s, but no version number was provided after it. Please make sure to put it on the same line.", c_name);
            return;
        }

        if(line.count) {
            char * c_line = to_c_string(line);
            scope_exit(free(c_line));

            log_print("do_load_room", "There is garbage left on the first line of file %s after the version number: %s", c_name, c_line);

            return;
        }


        if(version_token.data[0] != '<' || version_token.data[version_token.count - 1] != '>') {
            char * c_version_token = to_c_string(version_token);
            scope_exit(free(c_version_token));

            log_print("do_load_room", "The version number after \"VERSION\" in file %s needs to be surrounded with '<' and '>', instead, we got this : %s", c_name, c_version_token);

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

            log_print("do_load_room", "We expected a version number after \"VERSION\" in file %s, but instead we got this: %s.", c_name, c_version_token);

            return;
        }
    }

    int line_number = 1;
    bool successfully_parsed_file = true;

    int current_tile_index = -1;
    int current_collision_block_index = -1;

    while(true) { // @Cleanup, for_array with offset ?

        String line = lines.data[line_number];
        line_number += 1;

        if(line.count == -1) break; // EOF

		cut_spaces(&line);
        if(line.count == 0) continue; // Empty line

		String field_name = cut_until_space(&line);

        if(field_name == "name") {
            String arg = cut_until_space(&line);

            if(!arg.count) {
                log_print("do_load_room", "Expected a name after \"name\" on line %d of file %s.", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            if(line.count) {
                char * c_line = to_c_string(line);
                scope_exit(free(c_line));

                log_print("do_load_room", "Garbage left on line %d of file %s after the name: \"%s\".", line_number, c_name, c_line);

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

                log_print("do_load_room", "Failed to parse dimensions on line %d of file %s, string was \"%s\".", line_number, c_name, c_line);

                successfully_parsed_file = false;
                continue;
            }

            new_room.dimensions = dimensions;
        } else if(field_name == "begin_tile") {
            if(current_tile_index != -1) {
                log_print("do_load_room", "Got a \"begin_tile\" before getting an \"end_tile\" on line %d of file %s", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            Vector2 coords;

            bool success = string_to_v2(line, &coords);

            if(!success) {
                char * c_line = to_c_string(line);
                scope_exit(free(c_line));

                log_print("do_load_room", "Failed to parse coordinates of the tile on line %d of file %s, string was \"%s\".", line_number, c_name, c_line);

                successfully_parsed_file = false;
                continue;
            }

            // @Incomplete, we allow to have multiple tile on the same spot for now. Probably shouldn't.

            Tile tile;
            tile.position = coords;
            new_tile_array.add(tile);

            current_tile_index = new_tile_array.count - 1;

        } else if(field_name == "end_tile") {
            if(current_tile_index == -1) {
                log_print("do_load_room", "Got an \"end_tile\" before getting an \"begin_tile\" on line %d of file %s", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            // Reset tile index
            current_tile_index = -1;


        } else if(field_name == "texture") {
            if(current_tile_index == -1) {
                log_print("do_load_room", "Got a \"texture\" before outside if a tile block on line %d of file %s", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            if(new_tile_array.data[current_tile_index].texture.count) {

                char * c_texture = to_c_string(new_tile_array.data[current_tile_index].texture);
                scope_exit(free(c_texture));

                log_print("do_load_room", "Trying to set \"texture\" on line %d of file %s, but it has aleady been set. (Current: %s)", line_number, c_name, c_texture);

                successfully_parsed_file = false;
                continue;
            }

            String arg = cut_until_space(&line);

            if(!arg.count) {
                log_print("do_load_room", "Expected a texture name after \"texture\" on line %d of file %s.", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            if(line.count) {
                char * c_line = to_c_string(line);
                scope_exit(free(c_line));

                log_print("do_load_room", "Garbage left on line %d of file %s after the texture name: \"%s\".", line_number, c_name, c_line);

                successfully_parsed_file = false;
                continue;
            }

            new_tile_array.data[current_tile_index].texture = to_string(to_c_string(arg)); // @Cleanup UUHHHH, make a copy function.

        } else if(field_name == "begin_collision") {
            if(current_collision_block_index != -1) {
                log_print("do_load_room", "Got a \"begin_collision\" before getting an \"end_collision\" on line %d of file %s", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            bool success = true;

            bool set_x = false;
            bool set_y = false;

            float x0;
            float x1;
            float y0;
            float y1;

            String orig_line = line;
            while(line.count) {

                String param_name = cut_until_space(&line);

                if(param_name == "x") {

                    if(set_x) {
                        char * c_line = to_c_string(orig_line);
                        scope_exit(free(c_line));

                        log_print("do_load_room", "Tried to set x coordinates of collision block twice at line %d of file %s, string was \"%s\".", line_number, c_name, c_line);


                        success = false;
                        break;
                    }

                    String x_str = cut_until_space(&line);

                    float x;
                    bool param_success = string_to_float(x_str, &x);

                    if(param_success) {
                        set_x = true;
                        x0 = x;
                        x1 = x;
                    } else {
                        char * c_line = to_c_string(x_str);
                        scope_exit(free(c_line));

                        log_print("do_load_room", "Failed to parse x coordinate of the collision block on line %d of file %s, string was \"%s\".", line_number, c_name, c_line);

                        success = false;
                        break;
                    }
                } else if(param_name == "y") {

                    if(set_y) {
                        char * c_line = to_c_string(orig_line);
                        scope_exit(free(c_line));

                        log_print("do_load_room", "Tried to set y coordinates of collision block twice at line %d of file %s, string was \"%s\".", line_number, c_name, c_line);


                        success = false;
                        break;
                    }

                    String y_str = cut_until_space(&line);

                    float y;
                    bool param_success = string_to_float(y_str, &y);

                    if(param_success) {
                        set_y = true;
                        y0 = y;
                        y1 = y;
                    } else {
                        char * c_line = to_c_string(y_str);
                        scope_exit(free(c_line));

                        log_print("do_load_room", "Failed to parse y coordinate of the collision block on line %d of file %s, string was \"%s\".", line_number, c_name, c_line);

                        success = false;
                        break;
                    }
                } else if(param_name == "x_range") {

                    if(set_x) {
                        char * c_line = to_c_string(orig_line);
                        scope_exit(free(c_line));

                        log_print("do_load_room", "Tried to set x coordinates of collision block twice at line %d of file %s, string was \"%s\".", line_number, c_name, c_line);

                        success = false;
                        break;
                    }

                    String x0_str;
                    String x1_str;

                    x0_str = cut_until_space(&line);
                    x1_str = cut_until_space(&line);

                    bool param_success = true;

                    if(!x0_str.count) param_success = false;
                    if(!x1_str.count) param_success = false;

                    float tx0, tx1;

                    param_success &= string_to_float(x0_str, &tx0);
                    param_success &= string_to_float(x1_str, &tx1);

                    if(param_success) {
                        set_x = true;
                        x0 = tx0;
                        x1 = tx1;
                    } else {
                        char * c_x0 = to_c_string(x0_str);
                        scope_exit(free(c_x0));

                        char * c_x1 = to_c_string(x1_str);
                        scope_exit(free(c_x1));

                        log_print("do_load_room", "Failed to parse x coordinate of the collision block on line %d of file %s, range was \"%s\" - \"%s\" .", line_number, c_name, c_x0, c_x1);

                        success = false;
                        break;
                    }
                } else if(param_name == "y_range") {

                    if(set_y) {
                        char * c_line = to_c_string(orig_line);
                        scope_exit(free(c_line));

                        log_print("do_load_room", "Tried to set y coordinates of collision block twice at line %d of file %s, string was \"%s\".", line_number, c_name, c_line);

                        success = false;
                        break;
                    }

                    String y0_str;
                    String y1_str;

                    y0_str = cut_until_space(&line);
                    y1_str = cut_until_space(&line);

                    bool param_success = true;

                    if(!y0_str.count) param_success = false;
                    if(!y1_str.count) param_success = false;

                    float ty0, ty1;

                    param_success &= string_to_float(y0_str, &ty0);
                    param_success &= string_to_float(y1_str, &ty1);

                    if(param_success) {
                        set_y = true;
                        y0 = ty0;
                        y1 = ty1;
                    } else {
                        char * c_y0 = to_c_string(y0_str);
                        scope_exit(free(c_y0));

                        char * c_y1 = to_c_string(y1_str);
                        scope_exit(free(c_y1));

                        log_print("do_load_room", "Failed to parse y coordinate of the collision block on line %d of file %s, range was \"%s\" - \"%s\" .", line_number, c_name, c_y0, c_y1);

                        success = false;
                        break;
                    }
                } else {
                    char * c_param = to_c_string(param_name);
                    scope_exit(free(c_param));

                    char * c_line = to_c_string(line);
                    scope_exit(free(c_line));

                    log_print("do_load_room", "Unknown parameter \"%s\" for \"begin_collision\" on line %d of file %s, remainder was \"%s\".", c_param, line_number, c_name, c_line);

                    success = false;
                    break;
                }
            }

            if(!success) {
                successfully_parsed_file = false;
                continue;
            }

            if(!set_x) {
                log_print("do_load_room", "Missing x coordinates of the collision block on line %d of file %s, please use \"x\" or \"x_range\" to set them.", line_number, c_name);
                successfully_parsed_file = false;
                continue;
            }

            if(!set_y) {
                log_print("do_load_room", "Missing y coordinates of the collision block on line %d of file %s, please use \"y\" or \"y_range\" to set them.", line_number, c_name);
                successfully_parsed_file = false;
                continue;
            }

            CollisionBlock block;

            if(x0 > x1) swap(x0, x1);
            if(y0 > y1) swap(y0, y1);

            block.quad.x0 = x0;
            block.quad.x1 = x1;
            block.quad.y0 = y0;
            block.quad.y1 = y1;

            new_collision_block_array.add(block);

            current_collision_block_index = new_collision_block_array.count - 1;

            // log_print("do_load_room", "Added block in room %s with coords (x0: %f, y0: %f, x1: %f, y1: %f)", c_name, x0 , y0, x1, y1);
        } else if(field_name == "flags") {
            if(current_collision_block_index == -1) {
                log_print("do_load_room", "Got a \"flags\" outside of a collision block on line %d of file %s", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            CollisionBlock * current_block = &new_collision_block_array.data[current_collision_block_index];

            while(line.count) {
                String flag = cut_until_space(&line);

                bool is_valid_flag = false;

                if (flag == "disabled") {
                    current_block->flags |= COLLISION_DISABLED;
                    is_valid_flag = true;
                }

                if(flag == "player_only") {
                    current_block->flags |= COLLISION_PLAYER_ONLY; // @Incomplete Currently ignored
                    is_valid_flag = true;
                }


                if(!is_valid_flag) {
                    char * c_flag = to_c_string(flag);
                    scope_exit(free(c_flag));

                    log_print("do_load_room", "Unknown flag \"%s\" for \"flags\" on line %d of file %s. Ignoring.", c_flag, line_number, c_name);
                    continue;
                }

            }
        } else if(field_name == "teleport") {
            if(current_collision_block_index == -1) {
                log_print("do_load_room", "Got a \"teleport\" outside of a collision block on line %d of file %s", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            CollisionBlock * current_block = &new_collision_block_array.data[current_collision_block_index];

            if(current_block->action_type != UNSET) {
                log_print("do_load_room", "Trying to set multiple actions for the collision block on line %d of file %s, previous action was %d", line_number, c_name, current_block->action_type);

                successfully_parsed_file = false;
                continue;
            }

            String x_str;
            String y_str;

            x_str = cut_until_space(&line);
            y_str = cut_until_space(&line);

            bool param_success = true;

            if(!x_str.count) param_success = false;
            if(!y_str.count) param_success = false;

            float x, y;

            param_success &= string_to_float(x_str, &x);
            param_success &= string_to_float(y_str, &y);

            if(param_success) {
                String target_room;

                if(line.count) {
                    target_room = cut_until_space(&line);

                    if(line.count) {
                        char * c_line = to_c_string(line);
                        scope_exit(free(c_line));

                        log_print("do_load_room", "Garbage left on line %d of file %s after the target room name: \"%s\".", line_number, c_name, c_line);

                        successfully_parsed_file = false;
                        continue;
                    }
                }

                TeleportCollisionAction * action = (TeleportCollisionAction *) malloc(sizeof(TeleportCollisionAction));
                action->target = {x, y};
                action->target_room_name = to_string(to_c_string(target_room)); // @Cleanup

                current_block->action      = action;
                current_block->action_type = TELEPORT;

            } else {
                char * c_x = to_c_string(x_str);
                scope_exit(free(c_x));

                char * c_y = to_c_string(y_str);
                scope_exit(free(c_y));

                log_print("do_load_room", "Failed to parse coordinates of the target for the \"teleport\" on line %d of file %s, coords were \"%s\" - \"%s\" .", line_number, c_name, c_x, c_y);

                successfully_parsed_file = false;
                continue;
            }
        } else if(field_name == "end_collision") {
            if(current_collision_block_index == -1) {
                log_print("do_load_room", "Got an \"end_collision\" before getting a valid \"begin_collision\" on line %d of file %s", line_number, c_name);

                successfully_parsed_file = false;
                continue;
            }

            // Reset block index
            current_collision_block_index = -1;
        } else {
                char * c_field = to_c_string(field_name);
                scope_exit(free(c_field));

                char * c_line = to_c_string(line);
                scope_exit(free(c_line));
                log_print("do_load_room", "Unknown field  \"%s\"  on line %d of file %s, remainder was \"%s\".", c_field, line_number, c_name, c_line);

                continue; // Not failing, we just ignore this line. @Temporary, we should probably fail here.
        }
    }



    if(successfully_parsed_file) {
        *room = new_room;

        // @Incomplete
        // Those resets are going to mess up things that have pointers to this tile, eg. Editor panel.
        // Should mostly be solved after we start using a vector hash table (but problem of duplicates will remain.)


        // See above. Instead of making a full on hash table, we could simply have a function that gives us f(x,y)->index.
        // Instead of using a dynamic array, we have a fixed size x*y, which might be inefficient for some maps, but we probably
        // don't care, since it's just an array of pointers that's only allocated once. We'd still have to be carefull about
        // collisions, but do we expect to have multiple tiles on the same spot ? Should this be disallowed ? How would you even
        // handle the draw order ? Should we move to 3D coords with f(x,y,z) -> index ? I'd say not for now, but keep in mind for
        // the future.
        //                                                                          -Adrien, 2017-12-09

        for_array(room->tiles.data, room->tiles.count) {
            free(it->texture.data);
        }

        room->tiles.reset(true);
        room->collision_blocks.reset(true);

        room->tiles            = new_tile_array;
        room->collision_blocks = new_collision_block_array;
    }
}
