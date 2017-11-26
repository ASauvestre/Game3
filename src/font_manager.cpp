#include "font_manager.h"
#include "texture_manager.h"
#include "macros.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "os/layer.h"

static TextureManager * tm; // Make this a member ?

static int get_file_size(FILE * file) {
    fseek (file , 0 , SEEK_END);
    int size = ftell (file);
    fseek (file , 0 , SEEK_SET);
    return size;
}

void FontManager::init(TextureManager * texture_manager) {
    this->extensions.add("ttf");
    tm = texture_manager;
}

void FontManager::do_load_font(Font * font) {
    String file_data = os_specific_read_file(font->full_path);

    if(!file_data.data) return;

    scope_exit(free(file_data.data));

    unsigned char * bitmap = (unsigned char *) malloc(512 * 512 * 4); // Our bitmap is 512x512 pixels and each pixel takes 4 bytes @Robustness, make sure the bitmap is big enough for the font

    unsigned char * c_file_data = (unsigned char *) to_c_string(file_data);
    scope_exit(free(c_file_data));

    int result = stbtt_BakeFontBitmap(c_file_data, 0, 16.0f, bitmap, 512, 512, 32, 96, font->char_data); // @Robustness From stb_truetype.h : "no guarantee this fits!""

    if(result <= 0) {
        char * c_name = to_c_string(font->name);
        scope_exit(free(c_name));
        log_print("load_font", "The font %s could not be loaded, it is too large to fit in a 512x512 bitmap", c_name);
        return;
    }
    if(tm) {
        font->texture = tm->create_texture(font->name, bitmap, 512, 512, 1);
        // log_print("load_font", "Loaded font %s", font->name);
    } else {
        char * c_name = to_c_string(font->name);
        scope_exit(free(c_name));
        log_print("load_font", "Loaded font %s, but no TextureManager is assigned to the font manager, so the texture could not be registered", c_name);
    }
}

void FontManager::create_placeholder(String name, String path) {
    Font * font = (Font *) malloc(sizeof(Font));

    font->name      = name;
    font->full_path = path;

    this->table.add(name, font);
}

void FontManager::reload_or_create_asset(String full_path, String file_name) {
    Asset * asset = this->table.find(file_name);

    if(!asset) {
        create_placeholder(file_name, full_path);
        asset = this->table.find(file_name);
    } else {
        free(full_path.data);
    }

    do_load_font((Font *) asset);
}
