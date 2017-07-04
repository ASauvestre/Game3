#include <stdio.h>
#include <D3Dcompiler.h>
#include <assert.h>

#define STB_TRUETYPE_IMPLEMENTATION
//#include "stb_truetype.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "d3d_renderer.h"
#include "macros.h"

struct ShaderInputFormat {
    ID3D11InputLayout *layout;
    D3D11_INPUT_ELEMENT_DESC * layout_desc;
    int num_inputs;
};

enum ShaderInputMode {
    NONE,
    POS_COL,
    POS_TEXCOORD
};

struct Shader {
    HANDLE file_handle;

    FILETIME last_modified;

    char * filename;
    char * vs_name;
    char * ps_name;

    ID3D11VertexShader * VS;
    ID3D11PixelShader * PS;

    ShaderInputFormat input_format;
    ShaderInputMode input_mode;
};

struct GraphicsBuffer {
    GraphicsBuffer() {};
    ~GraphicsBuffer() {};

    std::vector<Shader> shaders;
    std::vector<VertexBuffer> vertex_buffers;
    std::vector<IndexBuffer> index_buffers;
    std::vector<char *> texture_id_buffer;
};

// Constants
const int MAX_VERTEX_BUFFER_SIZE     = 4096;
const int MAX_INDEX_BUFFER_SIZE      = 4096;
const int MAX_TEXTURE_INDEX_MAP_SIZE = 2048;

// Extern Globals
Shader * font_shader;
Shader * textured_shader;
Shader * colored_shader;
Shader * dummy_shader;

HWND handle;

Font * my_font;

// Globals
static IDXGISwapChain * swap_chain;
static ID3D11Device * d3d_device;
static ID3D11DeviceContext * d3d_dc;

static ID3D11RenderTargetView * render_target_view;

static ID3D11DepthStencilView * depth_stencil_view;
static ID3D11Texture2D * depth_stencil_buffer;

static ID3D11Buffer * d3d_vertex_buffer_interface;
static ID3D11Buffer * d3d_index_buffer_interface;

static ID3D11SamplerState * default_sampler_state;

static ID3D11Texture2D * d3d_texture_array;
static ID3D11ShaderResourceView * d3d_texture_array_srv;
static ID3D11Buffer * d3d_texture_index_map_buffer;

static ID3D11BlendState * transparent_blend_state;

static TextureManager texture_manager;

// State variables
static GraphicsBuffer graphics_buffer;
static int num_buffers;

static Vector2f rendering_resolution;

static BufferMode current_buffer_mode = QUADS;
static Shader current_shader;
static char * current_texture;

static VertexBuffer current_vb;
static IndexBuffer current_ib;

static bool buffering = false;

// Interface
void init_renderer(int width, int height) {
    rendering_resolution.width = width;
    rendering_resolution.height = height;

    init_d3d();
    init_shaders();
    init_textures();
    init_fonts();
}

void check_shader_files_modification() {
    
    check_specific_shader_file_modification(dummy_shader);
    check_specific_shader_file_modification(textured_shader);
    check_specific_shader_file_modification(colored_shader);
    check_specific_shader_file_modification(font_shader);
}

void set_shader(Shader * shader) {
    current_shader = *shader;
}

void set_texture(char * texture) {
    current_texture = texture; // @Temporary, find texture here ? At least check it exists.
}

void set_buffer_mode(BufferMode buffer_mode) {
    assert(buffering == false); // Can't change buffer_mode while we're buffering
    current_buffer_mode = buffer_mode;
}

bool create_shader(char * filename, char * vs_name, char * ps_name, D3D11_INPUT_ELEMENT_DESC * layout_desc, int num_shader_inputs, ShaderInputMode input_mode, Shader * shader){
    shader->filename = filename;
    shader->vs_name = vs_name;
    shader->ps_name = ps_name;

    shader->input_format.num_inputs = num_shader_inputs;
    shader->input_format.layout_desc = (D3D11_INPUT_ELEMENT_DESC *) malloc(sizeof(D3D11_INPUT_ELEMENT_DESC) * num_shader_inputs);
    
    shader->input_mode = input_mode;

    memcpy(shader->input_format.layout_desc, layout_desc, sizeof(D3D11_INPUT_ELEMENT_DESC) * num_shader_inputs);

    char path[512];;
    snprintf(path, 512, "shaders/%s", shader->filename);

    shader->file_handle = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    GetFileTime(shader->file_handle, NULL, NULL, &shader->last_modified);

    return compile_shader(shader);
}

Texture * create_texture(char * name, unsigned char * data, int width, int height, int bytes_per_pixel) {
    Texture * texture = texture_manager.get_new_texture_slot();

    texture->name               = name;
    texture->bitmap             = data;
    texture->width              = width;
    texture->height             = height;
    texture->bytes_per_pixel    = bytes_per_pixel;
    texture->width_in_bytes     = texture->width * texture->bytes_per_pixel;
    texture->num_bytes          = texture->width_in_bytes * texture->height;

    texture_manager.register_texture(texture);

    return texture;
}

Texture * create_texture(char * name, unsigned char * data, int width, int height) {
    return create_texture(name, data, width, height, 4); // Default is 32-bit bitmap.
}

void draw_frame() {
    
    // Clear view
    float color_array[4] = {0.29f, 0.84f, 0.93f, 1.0f}; // Sky blue @Temporary, move the sky background buffering to game_main.

    d3d_dc->ClearRenderTargetView(render_target_view, color_array);
    d3d_dc->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    switch_to_shader(graphics_buffer.shaders[0]);

    for(int i = 0; i < num_buffers; i++) {
        // Draw buffer
        draw_buffer(i);
    }

    swap_chain->Present(0, 0);  

    clear_buffers();
}

void check_specific_shader_file_modification(Shader * shader) {
    FILETIME new_last_modified;

    GetFileTime(shader->file_handle, NULL, NULL, &new_last_modified);

    if((new_last_modified.dwLowDateTime  != shader->last_modified.dwLowDateTime)  || 
       (new_last_modified.dwHighDateTime != shader->last_modified.dwHighDateTime)) {

        // log_print("recompile_shaders", "Previous time : %u %u, New time : %u %u",
        //        shader->last_modified.dwHighDateTime, shader->last_modified.dwLowDateTime,
        //        new_last_modified.dwHighDateTime, new_last_modified.dwLowDateTime);

        // Get new handle
        char path[512];;
        snprintf(path, 512, "shaders/%s", shader->filename);

        shader->file_handle = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        log_print("recompile_shaders", "Detected file modification, recompiling shader \"%s\"", shader->filename);

        compile_shader(shader);
        
        shader->last_modified = new_last_modified;
    }
}

void start_buffer() {
    assert(buffering == false);
    buffering = true;
}

void add_to_buffer(Vertex vertex) {
    assert(buffering == true);

    current_vb.vertices.push_back(vertex);
}

void end_buffer() {
    assert(buffering == true);

    // Fill the index buffer
    if(current_buffer_mode == QUADS) {

        assert(current_vb.vertices.size() % 4 == 0); // Assert we actually have Quads

        int first_index = 0;

        for(int i = 0; i < current_vb.vertices.size() / 4; i++) {
            current_ib.indices.push_back(first_index);
            current_ib.indices.push_back(first_index+1);
            current_ib.indices.push_back(first_index+2);
            current_ib.indices.push_back(first_index);
            current_ib.indices.push_back(first_index+3);
            current_ib.indices.push_back(first_index+1);

            first_index += 4;
        } 
    }

    // Add buffers
    graphics_buffer.shaders.push_back(current_shader);
    graphics_buffer.texture_id_buffer.push_back(current_texture);
    graphics_buffer.vertex_buffers.push_back(current_vb);
    graphics_buffer.index_buffers.push_back(current_ib);

    // Reset buffers
    current_vb.vertices.clear();
    current_ib.indices.clear();

	buffering = false;
    num_buffers++;
}

// Functions
void init_textures() {
    load_texture("grass1.png");
    load_texture("grass2.png");
    load_texture("grass3.png");
    load_texture("dirt_road.png");
    load_texture("dirt_road_bottom.png");
    load_texture("dirt_road_top.png");
    load_texture("megaperson.png");
    load_texture("tree.png");
}

void init_fonts() {
    my_font = load_font("Inconsolata.ttf");
}

void init_d3d() {

    DXGI_MODE_DESC  buffer_desc = {};
                    buffer_desc.Width                   = rendering_resolution.width;
                    buffer_desc.Height                  = rendering_resolution.height;
                    buffer_desc.RefreshRate.Numerator   = 60;
                    buffer_desc.RefreshRate.Denominator = 1;
                    buffer_desc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
                    buffer_desc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                    buffer_desc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SWAP_CHAIN_DESC    swap_chain_desc = {};
                            swap_chain_desc.BufferDesc          = buffer_desc;
                            swap_chain_desc.SampleDesc.Count    = 1;
                            swap_chain_desc.SampleDesc.Quality  = 0;
                            swap_chain_desc.BufferUsage         = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                            swap_chain_desc.BufferCount         = 1;
                            swap_chain_desc.OutputWindow        = handle;
                            swap_chain_desc.Windowed            = true;
                            swap_chain_desc.SwapEffect          = DXGI_SWAP_EFFECT_DISCARD;
                            swap_chain_desc.Flags               = 0;

    D3D11CreateDeviceAndSwapChain(  NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, 
                                    &swap_chain_desc, &swap_chain, &d3d_device, NULL, &d3d_dc);

    ID3D11Texture2D * backbuffer;
    swap_chain->GetBuffer(0, __uuidof(backbuffer), (void**) &backbuffer);

    d3d_device->CreateRenderTargetView(backbuffer, NULL, &render_target_view);
    backbuffer->Release();

    // Depth/Stencil Buffer
    D3D11_TEXTURE2D_DESC    depth_stencil_desc;
                            depth_stencil_desc.Width                = rendering_resolution.width;
                            depth_stencil_desc.Height               = rendering_resolution.height;
                            depth_stencil_desc.MipLevels            = 1;
                            depth_stencil_desc.ArraySize            = 1;
                            depth_stencil_desc.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
                            depth_stencil_desc.SampleDesc.Count     = 1;
                            depth_stencil_desc.SampleDesc.Quality   = 0;
                            depth_stencil_desc.Usage                = D3D11_USAGE_DEFAULT;
                            depth_stencil_desc.BindFlags            = D3D11_BIND_DEPTH_STENCIL;
                            depth_stencil_desc.CPUAccessFlags       = 0;
                            depth_stencil_desc.MiscFlags            = 0;

    d3d_device->CreateTexture2D(&depth_stencil_desc, NULL, &depth_stencil_buffer);

    d3d_device->CreateDepthStencilView(depth_stencil_buffer, NULL, &depth_stencil_view);

    d3d_dc->OMSetRenderTargets( 1, &render_target_view, depth_stencil_view);

    // Vectex Buffer
    D3D11_BUFFER_DESC   vertex_buffer_desc = {};
                        vertex_buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
                        vertex_buffer_desc.ByteWidth        = sizeof(Vertex) * MAX_VERTEX_BUFFER_SIZE;
                        vertex_buffer_desc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
                        vertex_buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
                        vertex_buffer_desc.MiscFlags        = 0;

    d3d_device->CreateBuffer(&vertex_buffer_desc, NULL, &d3d_vertex_buffer_interface);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    d3d_dc->IASetVertexBuffers(0, 1, &d3d_vertex_buffer_interface, &stride, &offset);

    // Index Buffer
    D3D11_BUFFER_DESC   index_buffer_desc = {};
                        index_buffer_desc.Usage             = D3D11_USAGE_DYNAMIC;
                        index_buffer_desc.ByteWidth         = sizeof(int) * MAX_INDEX_BUFFER_SIZE;
                        index_buffer_desc.BindFlags         = D3D11_BIND_INDEX_BUFFER;
                        index_buffer_desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
                        index_buffer_desc.MiscFlags         = 0;

    d3d_device->CreateBuffer(&index_buffer_desc, NULL, &d3d_index_buffer_interface);

    d3d_dc->IASetIndexBuffer(d3d_index_buffer_interface, DXGI_FORMAT_R32_UINT, 0);

    // Default shader
    dummy_shader = (Shader *)malloc(sizeof(Shader));

    D3D11_INPUT_ELEMENT_DESC dummy_shader_layout_desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
    };

    create_shader("dummy_shader.hlsl", "VS", "PS", dummy_shader_layout_desc, ARRAYSIZE(dummy_shader_layout_desc), NONE, dummy_shader);

    // Input Layout
    d3d_dc->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Viewport
    D3D11_VIEWPORT  viewport;
                    viewport.TopLeftX   = 0;
                    viewport.TopLeftY   = 0;
                    viewport.Width      = rendering_resolution.width;
                    viewport.Height     = rendering_resolution.height;
                    viewport.MinDepth   = 0.0f;
                    viewport.MaxDepth   = 1.0f;

    d3d_dc->RSSetViewports(1, &viewport);

    // Sampler State
    D3D11_SAMPLER_DESC  sampler_desc;
                        sampler_desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                        sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
                        sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
                        sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
                        sampler_desc.MipLODBias     = 0.0f;
                        sampler_desc.MaxAnisotropy  = 0;
                        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
                        sampler_desc.BorderColor[4] = {};
                        sampler_desc.MinLOD         = 0;
                        sampler_desc.MaxLOD         = D3D11_FLOAT32_MAX;

    d3d_device->CreateSamplerState(&sampler_desc, &default_sampler_state);

    // Texture index map buffer
    D3D11_BUFFER_DESC texture_map_index_buffer_desc;
    texture_map_index_buffer_desc.ByteWidth             = sizeof(int) * MAX_TEXTURE_INDEX_MAP_SIZE;
    texture_map_index_buffer_desc.Usage                 = D3D11_USAGE_DYNAMIC;
    texture_map_index_buffer_desc.BindFlags             = D3D11_BIND_CONSTANT_BUFFER;
    texture_map_index_buffer_desc.CPUAccessFlags        = D3D11_CPU_ACCESS_WRITE;
    texture_map_index_buffer_desc.MiscFlags             = 0;
    texture_map_index_buffer_desc.StructureByteStride   = 0;

    d3d_device->CreateBuffer( &texture_map_index_buffer_desc, NULL, &d3d_texture_index_map_buffer );

    // Blending 
    D3D11_RENDER_TARGET_BLEND_DESC render_target_blend_desc;

    render_target_blend_desc.BlendEnable            = true;
    render_target_blend_desc.SrcBlend               = D3D11_BLEND_SRC_ALPHA;
    render_target_blend_desc.DestBlend              = D3D11_BLEND_INV_SRC_ALPHA;
    render_target_blend_desc.BlendOp                = D3D11_BLEND_OP_ADD;
    render_target_blend_desc.SrcBlendAlpha          = D3D11_BLEND_ONE;
    render_target_blend_desc.DestBlendAlpha         = D3D11_BLEND_ZERO;
    render_target_blend_desc.BlendOpAlpha           = D3D11_BLEND_OP_ADD;
    render_target_blend_desc.RenderTargetWriteMask  = D3D11_COLOR_WRITE_ENABLE_ALL;

    D3D11_BLEND_DESC blend_desc;
    blend_desc.AlphaToCoverageEnable    = false;
    blend_desc.IndependentBlendEnable   = false;
    blend_desc.RenderTarget[0]          = render_target_blend_desc;

    d3d_device->CreateBlendState(&blend_desc, &transparent_blend_state);

    d3d_dc->OMSetBlendState(transparent_blend_state, NULL, 0xffffffff);
}

void init_shaders() {
    font_shader = (Shader *) malloc(sizeof(Shader));
    textured_shader = (Shader *) malloc(sizeof(Shader));
    colored_shader = (Shader *) malloc(sizeof(Shader));

    // Pixel formats
    D3D11_INPUT_ELEMENT_DESC font_shader_layout_desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
    };

    D3D11_INPUT_ELEMENT_DESC textured_shader_layout_desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
    };

    D3D11_INPUT_ELEMENT_DESC colored_shader_layout_desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
    };

    create_shader("font_shader.hlsl", "VS", "PS", font_shader_layout_desc, ARRAYSIZE(font_shader_layout_desc), POS_TEXCOORD, font_shader);
    create_shader("textured_shader.hlsl", "VS", "PS", textured_shader_layout_desc, ARRAYSIZE(textured_shader_layout_desc), POS_TEXCOORD, textured_shader);
    create_shader("colored_shader.hlsl", "VS", "PS", colored_shader_layout_desc, ARRAYSIZE(colored_shader_layout_desc), POS_COL, colored_shader);
}

Font * load_font(char * name) {
    Font * font = (Font *) malloc(sizeof(Font));

    font->name = name;

    char path[512];
    snprintf(path, 512, "fonts/%s", font->name);

    FILE * font_file = fopen(path, "rb");

    if(font_file == NULL) {
        log_print("load_font", "Font %s not found at %s", name, path)
        free(font);
        return NULL;
    }

    int font_file_size = get_file_size(font_file);

    unsigned char * font_file_buffer = (unsigned char *) malloc(font_file_size);

    // log_print("font_loading", "Font file for %s is %d bytes long", my_font->name, font_file_size);

    fread(font_file_buffer, 1, font_file_size, font_file);

    // We no longer need the file
    fclose(font_file);

    unsigned char * bitmap = (unsigned char *) malloc(256*256*4); // Our bitmap is 512x512 pixels and each pixel takes 4 bytes @Robustness, make sure the bitmap is big enough for the font

    int result = stbtt_BakeFontBitmap(font_file_buffer, 0, 16.0, bitmap, 512, 512, 32, 96, font->char_data); // From stb_truetype.h : "no guarantee this fits!""

    if(result <= 0) {
        log_print("load_font", "The font %s could not be loaded, it is too large to fit in a 512x512 bitmap", name);
    }

    font->texture = create_texture(font->name, bitmap, 512, 512, 1);

    free(font_file_buffer);

    log_print("load_font", "Loaded font %s", font->name);

    return font;
}

int get_file_size(FILE * file) {
    fseek (file , 0 , SEEK_END);
    int size = ftell (file);
    fseek (file , 0 , SEEK_SET);
    return size;
}

void load_texture(char * name) {
    Texture * texture = texture_manager.get_new_texture_slot();

    texture->name = name;

    do_load_texture(texture);

    texture_manager.register_texture(texture);
}

void do_load_texture(Texture * texture) {

    char path[512];
    snprintf(path, 512, "textures/%s", texture->name);
    
    int x,y,n;
    texture->bitmap = stbi_load(path, &x, &y, &n, 4);

    if(texture->bitmap == NULL) {
        log_print("do_load_texture", "Failed to load texture \"%s\"", texture->name);
        return;
    }

    if(n != 4) {
        log_print("do_load_texture", "Loaded texture \"%s\", it has %d bit depth, please convert to 32 bit depth", texture->name, n*8);
        n = 4;
    } else {
        log_print("do_load_texture", "Loaded texture \"%s\"", texture->name);
    }

    texture->width              = x;
    texture->height             = y;
    texture->bytes_per_pixel    = n;
    texture->width_in_bytes     = x * n;
    texture->num_bytes          = texture->width_in_bytes * y;
}

bool compile_shader(Shader * shader) {
    shader->VS = NULL;
    shader->PS = NULL;

    ID3DBlob * VS_bytecode;
    ID3DBlob * PS_bytecode;

    ID3DBlob * error = NULL;

    int file_size = GetFileSize(shader->file_handle, NULL);

    void * file_data = malloc(file_size);

    DWORD shader_size = 0;

    if(!ReadFile(shader->file_handle, file_data, file_size, &shader_size, NULL)) {
        log_print("compile_shader", "An error occured while reading the file \"%s\", could not compile the shaders. Error code is 0x%x", shader->filename, GetLastError());
        free(file_data);
        return false;
    }

    HRESULT error_code = D3DCompile(file_data, file_size, NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                       shader->vs_name, "vs_5_0", 0, 0, &VS_bytecode, &error);

    if(error) {
        log_print("compile_shader", "Vertex shader %s in %s failed to compile, error given is : \n---------\n%s---------", 
                   shader->vs_name, shader->filename, (char *)error->GetBufferPointer());

        error->Release();
        free(file_data);

        return false;
    }

    if (error_code > S_FALSE) {
        log_print("compile_shader", 
                  "An error occured while compiling the file \"%s\", could not compile the shaders. Error code is 0x%x", 
                  shader->filename, error_code);

        free(file_data);
        return false;
    }

    D3DCompile(file_data, file_size, NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                       shader->ps_name, "ps_5_0", 0, 0, &PS_bytecode, &error);

    if(error) {
        log_print("compile_shader", "Pixel shader %s in %s failed to compile, error given is : \n---------\n%s---------", 
                   shader->ps_name, shader->filename, (char *)error->GetBufferPointer());

        error->Release();
        VS_bytecode->Release();
        free(file_data);

        return false;
    }

    d3d_device->CreateVertexShader(VS_bytecode->GetBufferPointer(), VS_bytecode->GetBufferSize(), 
                                   NULL, &shader->VS);
    d3d_device->CreatePixelShader(PS_bytecode->GetBufferPointer(), PS_bytecode->GetBufferSize(), 
                                  NULL, &shader->PS);

    d3d_device->CreateInputLayout(shader->input_format.layout_desc, shader->input_format.num_inputs, VS_bytecode->GetBufferPointer(), 
                                  VS_bytecode->GetBufferSize(), &shader->input_format.layout);

    VS_bytecode->Release();
    PS_bytecode->Release();
    free(file_data);

    return true;
}

void recompile_shaders() {
    compile_shader(font_shader);
    compile_shader(textured_shader);
    compile_shader(colored_shader);
}

void bind_srv_to_texture(Texture * texture) {
    D3D11_TEXTURE2D_DESC texture_desc;
                         texture_desc.Width                 = texture->width;
                         texture_desc.Height                = texture->height;
                         texture_desc.MipLevels             = 1;
                         texture_desc.ArraySize             = 1;
                         texture_desc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
                         texture_desc.SampleDesc.Count      = 1;
                         texture_desc.SampleDesc.Quality    = 0;
                         texture_desc.Usage                 = D3D11_USAGE_DEFAULT;
                         texture_desc.BindFlags             = D3D11_BIND_SHADER_RESOURCE;
                         texture_desc.CPUAccessFlags        = 0;
                         texture_desc.MiscFlags             = 0;

    // Our fonts are greyscale, so let's make a special case for those
    if(texture->bytes_per_pixel == 1) {
        texture_desc.Format = DXGI_FORMAT_A8_UNORM;
    }

    D3D11_SUBRESOURCE_DATA texture_subresource;
    texture_subresource.pSysMem             = texture->bitmap;
    texture_subresource.SysMemPitch         = texture->width_in_bytes;
    texture_subresource.SysMemSlicePitch    = texture->num_bytes;

    ID3D11Texture2D * d3d_texture;

    d3d_device->CreateTexture2D(&texture_desc, &texture_subresource, &d3d_texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
                                    srv_desc.Format                         = DXGI_FORMAT_R8G8B8A8_UNORM;
                                    srv_desc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2D;
                                    srv_desc.Texture2D.MostDetailedMip      = 0;
                                    srv_desc.Texture2D.MipLevels            = 1;

    // Our fonts are greyscale, so let's make a special case for those
    if(texture->bytes_per_pixel == 1) {
        srv_desc.Format = DXGI_FORMAT_A8_UNORM;
    }

    d3d_device->CreateShaderResourceView(d3d_texture, &srv_desc, (ID3D11ShaderResourceView **) &texture->platform_info->srv);
}

void switch_to_shader(Shader shader) {
    if(shader.VS == NULL) {
        current_shader.input_format.layout = dummy_shader->input_format.layout;
        current_shader.VS = dummy_shader->VS;
        current_shader.input_mode = dummy_shader->input_mode;
    } else {
        current_shader.input_format.layout = shader.input_format.layout;
        current_shader.VS = shader.VS;
        current_shader.input_mode = shader.input_mode;
    }

    if(shader.PS == NULL) {
        current_shader.PS = dummy_shader->PS;
    } else {
        current_shader.PS = shader.PS;
    }

    d3d_dc->IASetInputLayout(current_shader.input_format.layout);
    d3d_dc->VSSetShader(current_shader.VS, NULL, 0);    
    d3d_dc->PSSetShader(current_shader.PS, NULL, 0);
}

void draw_buffer(int buffer_index) {

    switch_to_shader(graphics_buffer.shaders[buffer_index]);

    if(current_shader.input_mode == POS_TEXCOORD) {

        VertexBuffer * vb = &graphics_buffer.vertex_buffers[buffer_index];
        IndexBuffer * ib = &graphics_buffer.index_buffers[buffer_index];

        // Update vertex buffer
        D3D11_MAPPED_SUBRESOURCE resource = {};
        d3d_dc->Map(d3d_vertex_buffer_interface, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        memcpy(resource.pData, &vb->vertices[0], sizeof(Vertex) * vb->vertices.size());
        d3d_dc->Unmap(d3d_vertex_buffer_interface, 0);

        // Update index buffer
        resource = {};
        d3d_dc->Map(d3d_index_buffer_interface, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        memcpy(resource.pData, &ib->indices[0], sizeof( int ) *  ib->indices.size());
        d3d_dc->Unmap(d3d_index_buffer_interface, 0);

        char * texture_name = graphics_buffer.texture_id_buffer[buffer_index];

        int index = texture_manager.find_texture_index(texture_name);

        if(index == -1) {
            log_print("draw_buffer", "Texture %s was not found in the catalog", texture_name);
            return;
        }
        Texture * texture = &texture_manager.textures[index];
        
        // This texture doesn't have its platform info, likely because it was create by the game, let's give it one
        if(texture->platform_info == NULL) {
            texture->platform_info = (PlatformTextureInfo *) malloc(sizeof(PlatformTextureInfo));
            bind_srv_to_texture(texture);
        }

        ID3D11ShaderResourceView * srv = (ID3D11ShaderResourceView *) texture->platform_info->srv;

        d3d_dc->PSSetShaderResources(0, 1, &srv);

        d3d_dc->PSSetSamplers(0, 1, &default_sampler_state);

    } else if(current_shader.input_mode == POS_COL) {

        VertexBuffer * vb = &graphics_buffer.vertex_buffers[buffer_index];
        IndexBuffer * ib = &graphics_buffer.index_buffers[buffer_index];

        // Update vertex buffer
        D3D11_MAPPED_SUBRESOURCE resource = {};
        d3d_dc->Map(d3d_vertex_buffer_interface, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        memcpy(resource.pData, &vb->vertices[0], sizeof(Vertex) * vb->vertices.size());
        d3d_dc->Unmap(d3d_vertex_buffer_interface, 0);

        // Update index buffer
        resource = {};
        d3d_dc->Map(d3d_index_buffer_interface, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        memcpy(resource.pData, &ib->indices[0], sizeof( int ) *  ib->indices.size());
        d3d_dc->Unmap(d3d_index_buffer_interface, 0);

        d3d_dc->PSSetSamplers(0, 1, &default_sampler_state);

    } else {
        log_print("draw_buffer", "Unsupported shader input mode");
        return;
    }

    
    d3d_dc->DrawIndexed(graphics_buffer.index_buffers[buffer_index].indices.size(), 0, 0);
}

void clear_buffers() {
    // Clear buffer;
    graphics_buffer.vertex_buffers.clear();
    graphics_buffer.index_buffers.clear();
    graphics_buffer.texture_id_buffer.clear();
    graphics_buffer.shaders.clear();

    num_buffers = 0;
}