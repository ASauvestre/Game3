#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "texture_manager.h"
#include "macros.h"
#include "os/layer.h"

void TextureManager::init() {
    this->extensions.add("png");
}

Texture * TextureManager::create_texture(String name, unsigned char * data, int width, int height, int bytes_per_pixel) { // Default : bytes_per_pixel = 4

    String empty_string;
    empty_string.data = "";
    empty_string.count = 0;

    create_placeholder(name, empty_string);
    Texture * texture = this->table.find(name);

    texture->bitmap          = data;
    texture->width           = width;
    texture->height          = height;
    texture->bytes_per_pixel = bytes_per_pixel;
    texture->width_in_bytes  = width * bytes_per_pixel;
    texture->num_bytes       = width * height * bytes_per_pixel;

    return texture;
}

void TextureManager::create_placeholder(String name, String path) {
    Texture * texture = (Texture * ) malloc(sizeof(Texture));

    texture->name          = name;
    texture->full_path     = path;
    texture->dirty         = false;
    texture->platform_info = NULL;
    texture->bitmap        = NULL;

    this->table.add(name, texture);
}

// @Think, make this part of AssetManager_Poly ?
void TextureManager::reload_or_create_asset(String full_path, String file_name) {
    Asset * asset = this->table.find(file_name);

    if(!asset) {
        create_placeholder(file_name, full_path);
        asset = this->table.find(file_name);
    } else {
        free(full_path.data);
    }

    do_load_texture((Texture *) asset);
}

// @Incomplete Handle other file types in addition to PNG.
void TextureManager::do_load_texture(Texture * texture) {
    String file_data = os_specific_read_file(texture->full_path);
    scope_exit(free(file_data.data));

    int width, height, bytes_per_pixel;
    unsigned char * bitmap = stbi_load_from_memory((unsigned char *) file_data.data, file_data.count, &width, &height, &bytes_per_pixel, 4);

    if(bitmap == NULL) {
        char * c_name = to_c_string(texture->name);
        scope_exit(free(c_name));
        log_print("do_load_texture", "Failed to load texture \"%s\"", c_name);
        return;
    }

    if(texture->bitmap) {
        free(texture->bitmap);
    }

    texture->bitmap = bitmap;

    if(bytes_per_pixel != 4) {
        char * c_name = to_c_string(texture->name);
        scope_exit(free(c_name));
        log_print("do_load_texture", "Loaded texture \"%s\", it has %d bit depth, please convert to 32 bit depth", c_name, bytes_per_pixel * 8);
        bytes_per_pixel = 4;
    } else {
        // log_print("do_load_texture", "Loaded texture \"%s\"", texture->name);
    }

    texture->width           = width;
    texture->height          = height;
    texture->bytes_per_pixel = bytes_per_pixel;
    texture->width_in_bytes  = width * bytes_per_pixel;
    texture->num_bytes       = width * height * bytes_per_pixel;
    texture->dirty           = true;
}
