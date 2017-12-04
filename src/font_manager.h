#include "asset_manager.h"

#include "stb_truetype.h"

struct TextureManager;
struct Texture;

struct SpecificFont {
    int size;
    Texture * texture;
    stbtt_bakedchar char_data[96]; // 96 ASCII characters @Temporary
};

struct Font : Asset {
    Array<SpecificFont *> specific_fonts;
};

struct FontManager : AssetManager_Poly<Font> {
    void init(TextureManager * texture_manager);

    void reload_or_create_asset(String file_path, String file_name);
    void create_placeholder(String name, String path);

    SpecificFont * get_font_at_size(Font * font, int size);

private:
    void do_load_font(Font * font);
    SpecificFont * load_font_at_specific_size(Font * font, int size);
};
