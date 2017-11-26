#include "shader_manager.h"
#include "renderer.h"
#include "macros.h"

void ShaderManager::init() {
    this->extensions.add("shader");
}

void ShaderManager::create_placeholder(String name, String path) {
    Shader * shader = (Shader * ) malloc(sizeof(Shader));

    shader->name      = name;
    shader->full_path = path;

    this->table.add(name, shader);
}

// @Think, make this part of AssetManager_Poly ?
void ShaderManager::reload_or_create_asset(String full_path, String file_name) {
    Asset * asset = this->table.find(file_name);

    if(!asset) {
        create_placeholder(file_name, full_path);
        asset = this->table.find(file_name);
    } else{
        free(full_path.data);
    }

    do_load_shader((Shader *) asset);
}
