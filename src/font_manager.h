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

    void reload_asset(char * file_path, char * file_name, char * extension);

    void load_font(char * name);
};
