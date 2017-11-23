#include "asset_manager.h"

struct PlatformTextureInfo;

struct Texture : Asset{
    int width;
    int height;

    int bytes_per_pixel;

    unsigned char * bitmap;

    int width_in_bytes;
    int num_bytes;

    bool dirty; // Was it changed this frame ?

    PlatformTextureInfo * platform_info; // Pointer to data structure containing platform specific fields
};

struct TextureManager : AssetManager_Poly<Texture> {
    void init();

    void reload_or_create_asset(String file_path, String file_name);
    void create_placeholder(char * name, char * path);

    Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel = 4);


private: // @Cleanup I don't like this. If it's private, don't make it part of the struct namespace, just use a local function.
    void do_load_texture(Texture * texture);
};
