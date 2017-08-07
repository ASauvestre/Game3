#include "asset_manager.h"

struct PlatformTextureInfo;

struct TextureManager : AssetManager {
    void reload_asset(char * file_path, char * file_name, char * extension);

    Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel);
    Texture * create_texture(char * name, unsigned char * data, int width, int height);

    void load_texture(char * name);

private:
    void do_load_texture(Texture * texture);
};
