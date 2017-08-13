#include "shader_manager.h"
#include "renderer.h"
#include "macros.h"

void ShaderManager::init() {
    this->directories.add("shaders/");
}

Shader * ShaderManager::load_shader(char * file_name) {
    Shader * shader = (Shader *) malloc(sizeof(Shader));

    shader->name = file_name;
    shader->filename = file_name;

    do_load_shader(shader);

    this->table.add(file_name, shader);

    return shader;
}

void ShaderManager::reload_asset(char * file_path, char * file_name, char * extension) {
    log_print("reload_asset", "Asset change on file %s caught by the shader manager", file_name);
    // @Incomplete, check extension

    Shader * shader = this->table.find(file_name);

	if (!shader) {
		log_print("reload_asset", "Shader file %s is not registered in the manager, so we're not reloading it", file_name);
	}

    do_load_shader(shader);
}

