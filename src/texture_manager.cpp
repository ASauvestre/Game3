#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "texture_manager.h"
#include "macros.h"

void TextureManager::init() {
    this->directories.add("textures/");
}

Texture * TextureManager::create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel) { // Default : bytes_per_pixel = 4
    Texture * texture = (Texture * ) malloc(sizeof(Texture));

    texture->name            = name;
    texture->bitmap          = data;
    texture->width           = width;
    texture->height          = height;
    texture->bytes_per_pixel = bytes_per_pixel;
    texture->width_in_bytes  = width * bytes_per_pixel;
    texture->num_bytes       = width * height * bytes_per_pixel;

    // These should be default values, but since we're allocating textures on
    // the heap by default, those aren't set, so we do it here.
    texture->dirty         = false;
    texture->platform_info = NULL;

    this->table.add(name, texture);

    return texture;
}

// @Incomplete Handle other file types in addition to PNG.
void TextureManager::do_load_texture(Texture * texture) {

    char path[512];
    snprintf(path, 512, "data/textures/%s", texture->name);

    int width, height, bytes_per_pixel;

    texture->bitmap = stbi_load(path, &width, &height, &bytes_per_pixel, 4);

    if(texture->bitmap == NULL) {
        log_print("do_load_texture", "Failed to load texture \"%s\"", texture->name);
        return;
    }

    if(bytes_per_pixel != 4) {
        log_print("do_load_texture", "Loaded texture \"%s\", it has %d bit depth, please convert to 32 bit depth", texture->name, bytes_per_pixel * 8);
        bytes_per_pixel = 4;
    } else {
        log_print("do_load_texture", "Loaded texture \"%s\"", texture->name);
    }

    texture->width           = width;
    texture->height          = height;
    texture->bytes_per_pixel = bytes_per_pixel;
    texture->width_in_bytes  = width * bytes_per_pixel;
    texture->num_bytes       = width * height * bytes_per_pixel;
    texture->dirty           = false;
    texture->platform_info   = NULL;
}

void TextureManager::load_texture(char * name) {
    Texture * texture = (Texture * ) malloc(sizeof(Texture));

    texture->name = name;

    do_load_texture(texture);

    this->table.add(name, texture);
}

void TextureManager::reload_asset(char * file_path, char * file_name, char * extension) {
    log_print("reload_asset", "Asset change on file %s caught by the texture manager", file_name);
    // @Incomplete, check extension
    load_texture(file_name);

}
