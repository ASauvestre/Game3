#include "asset_manager.h"

#include "stb_truetype.h"

struct TextureManager;
struct Texture;

struct Font : Asset {
    Texture * texture;
    stbtt_bakedchar char_data[96]; // 96 ASCII characters @Temporary
};

struct FontManager : AssetManager_Poly<Font> {
    void init(TextureManager * texture_manager);

    void reload_or_create_asset(String file_path, String file_name);
    void create_placeholder(char * name, char * path);

private:
    void do_load_font(Font * font);
};
