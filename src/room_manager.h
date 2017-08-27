#include "asset_manager.h"
#include "math_m.h"

enum CollisionActionType {
    UNSET,
    TELEPORT
};

struct CollisionAction {
    // Nothing here for now
};

struct TeleportCollisionAction : CollisionAction {
    Vector2f target = {};
    char * target_room_name = NULL;
};

struct CollisionBlock {
    Quad quad;

    CollisionActionType action_type = UNSET;
    CollisionAction * action = NULL;
};

struct Tile {
    char * texture = NULL;
    Vector2 position;

    int room_target_id;         // Used if type is SWITCH_ROOM
    Vector2 target_tile_coords; // Used if type is SWITCH_ROOM
};

struct Room : Asset{
    Vector2 dimensions;
    Array<Tile> tiles;
    Array<CollisionBlock> collision_blocks;
};

struct RoomManager : AssetManager_Poly<Room> {
    void init();

    void reload_or_create_asset(String file_path, String file_name);
    void create_placeholder(char * name, char * path);

private:
    void do_load_room(Room * room);
};
