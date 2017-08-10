#include "asset_manager.h"

enum ShaderInputMode {
    NONE,
    POS,
    POS_COL,
    POS_UV
};

// Lots of void * because this is going to be used by multiple Graphics APIs
struct Shader : Asset {
    char * filename;

    void * VS;
    void * PS;

    void * input_layout; // @Temporary, I don't like this very much, I wonder if we could have handle input layouts in D3D the same way OGL does them (ie, not shader bound)

    ShaderInputMode input_mode;
};

struct ShaderManager : AssetManager_Poly<Shader> {
    void init();

    void reload_asset(char * file_path, char * file_name, char * extension);

    Shader * load_shader(char * file_name);
};
