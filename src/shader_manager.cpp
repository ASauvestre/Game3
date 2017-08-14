#include "shader_manager.h"
#include "renderer.h"
#include "macros.h"
#include "os/layer.h"

void ShaderManager::init() {
    this->directories.add("shaders/");
}

Shader * ShaderManager::load_shader(char * file_name) {
    Shader * shader = (Shader *) malloc(sizeof(Shader));
    this->init_asset(shader);

    shader->name     = file_name;
    shader->filename = file_name;

    do_load_shader(shader);

    this->table.add(file_name, shader);

    return shader;
}

void ShaderManager::reload_asset(String file_path, String file_name) {
    // @Incomplete, check extension

    char * c_file_name = to_c_string(file_name);
    scope_exit(free(c_file_name));

    Shader * shader = this->table.find(c_file_name);

    if (!shader) {
        log_print("reload_asset", "Shader file %s is not registered in the manager, so we're not reloading it", c_file_name);
        return;
    }

    if((os_specific_get_time() - shader->last_reload_time) < shader->reload_timeout) {
        // We already reloaded it.
        return;
    }

    log_print("reload_asset", "Asset change on file %s caught by the shader manager, previous reload was at time %f", c_file_name, shader->last_reload_time);
    do_load_shader(shader);
}

