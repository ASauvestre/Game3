#include "asset_manager.h"

// Lots of void * because this is going to be used by multiple Graphics APIs
struct Shader : Asset {
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

    void reload_or_create_asset(String file_path, String file_name);
    void create_placeholder(char * name, char * path);
};
