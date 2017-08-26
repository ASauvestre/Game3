#include "asset_manager.h"
#include "math_m.h"

enum TileType {
    DEFAULT,
    BLOCK,
    SWITCH_ROOM
};

struct CollisionBlock {
    Quad quad;
};

struct Tile {
    char * texture = NULL;
    Vector2 position;

    TileType type = DEFAULT;

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
