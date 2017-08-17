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
    char path[512];
    snprintf(path, 512, "data/fonts/%s", font->name); // @Temporary

    FILE * font_file = fopen(path, "rb");

    if(font_file == NULL) {
        log_print("load_font", "Font %s not found at %s", font->name, path)
        free(font);

        return;
    }

    int font_file_size = get_file_size(font_file);

    unsigned char * font_file_buffer = (unsigned char *) malloc(font_file_size);
    scope_exit(free(font_file_buffer));

    // log_print("font_loading", "Font file for %s is %d bytes long", my_font->name, font_file_size);

    fread(font_file_buffer, 1, font_file_size, font_file);

    // We no longer need the file
    fclose(font_file);

    unsigned char * bitmap = (unsigned char *) malloc(256 * 256 * 4); // Our bitmap is 512x512 pixels and each pixel takes 4 bytes @Robustness, make sure the bitmap is big enough for the font

    int result = stbtt_BakeFontBitmap(font_file_buffer, 0, 16.0, bitmap, 512, 512, 32, 96, font->char_data); // From stb_truetype.h : "no guarantee this fits!""

    if(result <= 0) {
        log_print("load_font", "The font %s could not be loaded, it is too large to fit in a 512x512 bitmap", font->name);
        return;
    }
    if(tm) {
        font->texture = tm->create_texture(font->name, bitmap, 512, 512, 1);
        log_print("load_font", "Loaded font %s", font->name);
    } else {
        log_print("load_font", "Loaded font %s, but no TextureManager is assigned to the font manager, so the texture could not be registered", font->name);
    }
}

void FontManager::create_placeholder(char * name, char * path) {
    Font * font = (Font *) malloc(sizeof(Font));
    this->init_asset(font);

    font->name      = name;
    font->full_path = path;

    this->table.add(name, font);
}

void FontManager::reload_or_create_asset(String full_path, String file_name) {
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

    if((os_specific_get_time() - asset->last_reload_time) < asset->reload_timeout) {
        return;
    }

    do_load_font((Font *) asset);
}
