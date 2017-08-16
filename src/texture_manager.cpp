#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "texture_manager.h"
#include "macros.h"
#include "os/layer.h"

void TextureManager::init() {
    this->extensions.add("png");
}

Texture * TextureManager::create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel) { // Default : bytes_per_pixel = 4
    create_placeholder(name);
    Texture * texture = this->table.find(name);

    texture->bitmap          = data;
    texture->width           = width;
    texture->height          = height;
    texture->bytes_per_pixel = bytes_per_pixel;
    texture->width_in_bytes  = width * bytes_per_pixel;
    texture->num_bytes       = width * height * bytes_per_pixel;

    return texture;
}

void TextureManager::load_texture(char * name) {
    Texture * texture = this->table.find(name);

    if(!texture) {
        create_placeholder(name);
		texture = this->table.find(name);
    }

    do_load_texture(texture);
}

void TextureManager::create_placeholder(char * name) {
    Texture * texture = (Texture * ) malloc(sizeof(Texture));
    this->init_asset(texture);

    texture->name          = name;
    texture->dirty         = false;
    texture->platform_info = NULL;

    this->table.add(name, texture);
}

// @Think, make this part of AssetManager_Poly ?
void TextureManager::reload_or_create_asset(String file_path, String file_name) {
    char * c_file_name = to_c_string(file_name);

    Asset * asset = this->table.find(c_file_name);

    if(!asset) {
        create_placeholder(c_file_name);
        asset = this->table.find(c_file_name);
    }

    if((os_specific_get_time() - asset->last_reload_time) < asset->reload_timeout) {
        return;
    }

    do_load_texture((Texture *) asset);
}

// @Incomplete Handle other file types in addition to PNG.
void TextureManager::do_load_texture(Texture * texture) {
    char path[512];
    snprintf(path, 512, "data/textures/%s", texture->name);

    int width, height, bytes_per_pixel;

    unsigned char * bitmap = stbi_load(path, &width, &height, &bytes_per_pixel, 4);

    if(bitmap == NULL) {
        log_print("do_load_texture", "Failed to load texture \"%s\"", texture->name);
        return;
    }

    if(texture->dirty) {
        free(texture->bitmap);
    }

    texture->bitmap = bitmap;

    if(bytes_per_pixel != 4) {
        log_print("do_load_texture", "Loaded texture \"%s\", it has %d bit depth, please convert to 32 bit depth", texture->name, bytes_per_pixel * 8);
        bytes_per_pixel = 4;
    } else {
        log_print("do_load_texture", "Loaded texture \"%s\"", texture->name);
    }

    texture->width            = width;
    texture->height           = height;
    texture->bytes_per_pixel  = bytes_per_pixel;
    texture->width_in_bytes   = width * bytes_per_pixel;
    texture->num_bytes        = width * height * bytes_per_pixel;

    texture->last_reload_time = os_specific_get_time();
    texture->dirty = true;
}
