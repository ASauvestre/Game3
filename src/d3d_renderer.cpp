#include <stdio.h>
#include <D3Dcompiler.h>
#include <d3d11.h>
#include <assert.h>

#include "d3d_texture_manager.h"
#include "graphics_buffer.h"

#define COMMON_TYPES_IMPLEMENTATION
#include "common_types.h"

#include "d3d_renderer.h"

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

// Private functions
static void check_shader_files_modification(); // @Temporary, move to hotloader.

static void bind_srv_to_texture(Texture * texture);

static bool create_shader(char * filename, char * vs_name, char * ps_name, D3D11_INPUT_ELEMENT_DESC * layout_desc, int num_shader_inputs, ShaderInputMode input_mode, Shader * shader);

static bool compile_shader(Shader * shader);
static void check_specific_shader_file_modification(Shader * shader);

static void draw_buffer(GraphicsBuffer * graphics_buffer, int buffer_index, TextureManager * texture_manager);

static void switch_to_shader(Shader * shader);

// Constants
const int MAX_VERTEX_BUFFER_SIZE     = 4096;
const int MAX_INDEX_BUFFER_SIZE      = 4096;
const int MAX_TEXTURE_INDEX_MAP_SIZE = 2048;

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

static Shader * dummy_shader;

// State variables
static GraphicsBuffer graphics_buffer;
static int num_buffers;

static Shader d3d_shader;

void init_platform_renderer(Vector2f rendering_resolution, void * handle) {

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
                            swap_chain_desc.OutputWindow        = (HWND) handle;
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

void draw_frame(GraphicsBuffer * graphics_buffer, int num_buffers, TextureManager * texture_manager) {

    // Check if shaders have changed @Temporary move to hotloader
    check_shader_files_modification();

    // Clear view
    float color_array[4] = {0.29f, 0.84f, 0.93f, 1.0f}; // Sky blue @Temporary, move the sky background buffering to game_main.

    d3d_dc->ClearRenderTargetView(render_target_view, color_array);
    d3d_dc->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    for(int i = 0; i < num_buffers; i++) {
        // Draw buffer
        draw_buffer(graphics_buffer, i, texture_manager);
    }

    swap_chain->Present(0, 0);  
}

static void draw_buffer(GraphicsBuffer * graphics_buffer, int buffer_index, TextureManager * texture_manager) {

    switch_to_shader(*graphics_buffer->shaders[buffer_index]);

    if(d3d_shader.input_mode == POS_TEXCOORD) {

        VertexBuffer * vb = &graphics_buffer->vertex_buffers[buffer_index];
        IndexBuffer * ib = &graphics_buffer->index_buffers[buffer_index];

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

        char * texture_name = graphics_buffer->texture_id_buffer[buffer_index];

        int index = texture_manager->find_texture_index(texture_name);

        if(index == -1) {
            log_print("draw_buffer", "Texture %s was not found in the catalog", texture_name);
            return;
        }
        Texture * texture = &texture_manager->textures[index];
        
        // This texture doesn't have its platform info, likely because it was create by the game, let's give it one
        if(texture->platform_info == NULL) {
            texture->platform_info = (PlatformTextureInfo *) malloc(sizeof(PlatformTextureInfo));
            bind_srv_to_texture(texture);
        }

        ID3D11ShaderResourceView * srv = (ID3D11ShaderResourceView *) texture->platform_info->srv;

        d3d_dc->PSSetShaderResources(0, 1, &srv);

        d3d_dc->PSSetSamplers(0, 1, &default_sampler_state);

    } else if(d3d_shader.input_mode == POS_COL) {

        VertexBuffer * vb = &graphics_buffer->vertex_buffers[buffer_index];
        IndexBuffer * ib = &graphics_buffer->index_buffers[buffer_index];

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

    
    d3d_dc->DrawIndexed(graphics_buffer->index_buffers[buffer_index].indices.size(), 0, 0);
}

static void bind_srv_to_texture(Texture * texture) {
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

// Shaders
void init_platform_shaders() {
    font_shader     = (Shader *) malloc(sizeof(Shader));
    textured_shader = (Shader *) malloc(sizeof(Shader));
    colored_shader  = (Shader *) malloc(sizeof(Shader));

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

static void check_shader_files_modification() { 
    check_specific_shader_file_modification(dummy_shader);
    check_specific_shader_file_modification(textured_shader);
    check_specific_shader_file_modification(colored_shader);
    check_specific_shader_file_modification(font_shader);
}

static bool compile_shader(Shader * shader) {
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

static void recompile_shaders() {
    compile_shader(font_shader);
    compile_shader(textured_shader);
    compile_shader(colored_shader);
}

static bool create_shader(char * filename, char * vs_name, char * ps_name, D3D11_INPUT_ELEMENT_DESC * layout_desc, int num_shader_inputs, ShaderInputMode input_mode, Shader * shader){
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

static void check_specific_shader_file_modification(Shader * shader) {
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

static void switch_to_shader(Shader * shader) {
    if(shader->VS == NULL) {
        d3d_shader.input_format.layout = dummy_shader->input_format.layout;
        d3d_shader.VS = dummy_shader->VS;
        d3d_shader.input_mode = dummy_shader->input_mode;
    } else {
        d3d_shader.input_format.layout = shader->input_format.layout;
        d3d_shader.VS = shader->VS;
        d3d_shader.input_mode = shader->input_mode;
    }

    if(shader->PS == NULL) {
        d3d_shader.PS = dummy_shader->PS;
    } else {
        d3d_shader.PS = shader->PS;
    }

    d3d_dc->IASetInputLayout(d3d_shader.input_format.layout);
    d3d_dc->VSSetShader(d3d_shader.VS, NULL, 0);    
    d3d_dc->PSSetShader(d3d_shader.PS, NULL, 0);
}
