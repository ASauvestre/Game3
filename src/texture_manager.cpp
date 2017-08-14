#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "texture_manager.h"
#include "macros.h"
#include "os/layer.h"

void TextureManager::init() {
    this->directories.add("textures/");
}

Texture * TextureManager::create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel) { // Default : bytes_per_pixel = 4
    Texture * texture = (Texture * ) malloc(sizeof(Texture));
    init_asset(texture);

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

// @Bug this doesn't work and leaks memory for reloads. We need a separate function.
// @Bug this doesn't work and leaks memory for reloads. We need a separate function.
// @Bug this doesn't work and leaks memory for reloads. We need a separate function.
// @Bug this doesn't work and leaks memory for reloads. We need a separate function.
void TextureManager::load_texture(char * name) {
    Texture * texture = (Texture * ) malloc(sizeof(Texture));
    this->init_asset(texture);

    texture->name = name;

    do_load_texture(texture);

    this->table.add(name, texture);
}

void TextureManager::reload_asset(String file_path, String file_name) {

    char * c_file_name = to_c_string(file_name);
    scope_exit(free(c_file_name));

    Texture * texture = this->table.find(c_file_name);

    if((os_specific_get_time() - texture->last_reload_time) < texture->reload_timeout) {
        // We already reloaded it.
        return;
    }

    String extension = find_char_from_left('.', file_name);


    if(memcmp(".png", extension.data, extension.count) == 0) {
        log_print("reload_asset", "Asset change on file %s caught by the texture manager", c_file_name);
        load_texture(c_file_name);
    }

}
