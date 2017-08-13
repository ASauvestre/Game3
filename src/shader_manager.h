#include "asset_manager.h"

// Lots of void * because this is going to be used by multiple Graphics APIs
struct Shader : Asset {
    char * filename;

    void * VS;
    void * PS;

    void * input_layout = NULL;

    // Indices of input types in the Vertex Shader
    int position_index = -1;
    int color_index    = -1;
    int uv_index       = -1;
};

struct ShaderManager : AssetManager_Poly<Shader> {
    void init();

    void reload_asset(char * file_path, char * file_name, char * extension);

    Shader * load_shader(char * file_name);
};
