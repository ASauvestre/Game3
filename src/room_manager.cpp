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

}
