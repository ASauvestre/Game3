#include "shader_manager.h"
#include "renderer.h"
#include "macros.h"
#include "os/layer.h"

void ShaderManager::init() {
    this->extensions.add("hlsl");
}

Shader * ShaderManager::load_shader(char * file_name) {
    create_placeholder(file_name);

    Shader * shader = this->table.find(file_name);

    do_load_shader(shader);

    return shader;
}

void ShaderManager::create_placeholder(char * name) {
    Shader * shader = (Shader * ) malloc(sizeof(Shader));
    this->init_asset(shader);

    shader->name     = name;
    shader->filename = name;

    this->table.add(name, shader);
}

// @Think, make this part of AssetManager_Poly ?
void ShaderManager::reload_or_create_asset(String file_path, String file_name) {
    char * c_file_name = to_c_string(file_name);

    Asset * asset = this->table.find(c_file_name);

    if(!asset) {
        create_placeholder(c_file_name);
        Asset * asset = this->table.find(c_file_name);
    }

    if((os_specific_get_time() - asset->last_reload_time) < asset->reload_timeout) {
        return;
    }

    do_load_shader((Shader *) asset);
}
