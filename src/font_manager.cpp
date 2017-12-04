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

SpecificFont * FontManager::get_font_at_size(Font * font, int size) {
    // @Speed use a hashmap here ? Most likely unecessary.
    for_array(font->specific_fonts.data, font->specific_fonts.count) {
        if((*it)->size == size) return *it;
    }

    return load_font_at_specific_size(font, size);

}

SpecificFont * FontManager::load_font_at_specific_size(Font * font, int size) {
    SpecificFont * specific_font = (SpecificFont *) malloc(sizeof(SpecificFont));
    specific_font->size = size;

    String file_data = os_specific_read_file(font->full_path);

    if(!file_data.data) return NULL;

    scope_exit(free(file_data.data));

    unsigned char * c_file_data = (unsigned char *) to_c_string(file_data);
    scope_exit(free(c_file_data));

    unsigned char * bitmap = (unsigned char *) malloc(512 * 512 * 4); // Our bitmap is 512x512 pixels and each pixel takes 4 bytes @Robustness, make sure the bitmap is big enough for the font
    int result = stbtt_BakeFontBitmap(c_file_data, 0, size, bitmap, 512, 512, 32, 96, specific_font->char_data); // @Robustness From stb_truetype.h : "no guarantee this fits!""

    char * c_name = to_c_string(font->name);
    scope_exit(free(c_name));

    if(result <= 0) {

        log_print("load_font", "The font %s could not be loaded for size %d, it is too large to fit in a 512x512 bitmap", c_name, size);
        return NULL;
    }

    if(tm) {
        char texture_name_buffer[128];
        snprintf(texture_name_buffer, 128, "%s%d", c_name, size);

        String texture_name = to_string(strdup(texture_name_buffer));
        specific_font->texture = tm->table.find(texture_name);
        if(specific_font->texture) {
            free(texture_name.data);
            // We already made a texture for that font size, we'll just update the bitmap.
            free(specific_font->texture->bitmap);
            specific_font->texture->bitmap = bitmap;
            specific_font->texture->dirty  = true;
        } else {
            specific_font->texture = tm->create_texture(texture_name, bitmap, 512, 512, 1);
        }

        font->specific_fonts.add(specific_font);

        // log_print("load_font", "Loaded font %s at size %d", c_name, size);

        return specific_font;
    } else {
        log_print("load_font", "Tried creating font %s with size %d, but no TextureManager is assigned to the font manager, so the texture could not be registered", c_name, size);
    }

    return NULL;
}

void FontManager::do_load_font(Font * font) {
    for_array(font->specific_fonts.data, font->specific_fonts.count) {
        load_font_at_specific_size(font, (*it)->size);
        font->specific_fonts.remove_by_index(it_index);
        free(it);
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
