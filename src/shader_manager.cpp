#include "shader_manager.h"
#include "renderer.h"
#include "macros.h"

void ShaderManager::init() {
    this->extensions.add("shader");
}

void ShaderManager::create_placeholder(char * name, char * path) {
    Shader * shader = (Shader * ) malloc(sizeof(Shader));

    shader->name      = name;
    shader->full_path = path;

    this->table.add(name, shader);
}

// @Think, make this part of AssetManager_Poly ?
void ShaderManager::reload_or_create_asset(String full_path, String file_name) {
    char * c_file_name = to_c_string(file_name);
    char * c_full_path = to_c_string(full_path);

    Asset * asset = this->table.find(c_file_name);

    if(!asset) {
        create_placeholder(c_file_name, c_full_path);
        asset = this->table.find(c_file_name);
    } else{
        free(c_file_name);
        free(c_full_path);
    }

    do_load_shader((Shader *) asset);
}
