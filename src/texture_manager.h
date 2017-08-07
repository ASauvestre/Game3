#include "asset_manager.h"

struct PlatformTextureInfo;

struct Texture : Asset{
    int width;
    int height;

    int bytes_per_pixel;

    unsigned char * bitmap;

    int width_in_bytes;
    int num_bytes;

    bool modified; // Was it changed this frame ?

    PlatformTextureInfo * platform_info; // Pointer to data structure containing platform specific fields
};

struct TextureManager : AssetManager_Poly<Texture> {
    void init();

    void reload_asset(char * file_path, char * file_name, char * extension);

    Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel);
    Texture * create_texture(char * name, unsigned char * data, int width, int height);

    void load_texture(char * name);

private:
    void do_load_texture(Texture * texture);
};
