#include <stdio.h>
#include <D3Dcompiler.h>
#include <d3d11.h>
#include <assert.h>

#include "d3d_renderer.h"

#include "texture_manager.h"
#include "shader_manager.h"
#include "graphics_buffer.h"

struct PlatformTextureInfo {
    ID3D11ShaderResourceView * srv;
};

struct InputLayoutDesc {
    D3D11_INPUT_ELEMENT_DESC * desc;
    int num_inputs;
};

///////////////////////////////////////////////
// @Temporary, will be infered from file
InputLayoutDesc pos_layout_desc;
InputLayoutDesc pos_col_layout_desc;
InputLayoutDesc pos_uv_layout_desc;
///////////////////////////////////////////////

// Private functions
static void bind_srv_to_texture(Texture * texture);

static bool create_shader(char * filename, ShaderInputMode input_mode, Shader * shader);

static void draw_buffer(GraphicsBuffer * graphics_buffer, int buffer_index, TextureManager * texture_manager);

static void switch_to_shader(Shader * shader);

// Constants
const int MAX_VERTEX_BUFFER_SIZE = 8192;
const int MAX_INDEX_BUFFER_SIZE  = 8192;

// Globals
static IDXGISwapChain      * swap_chain;
static ID3D11Device        * d3d_device;
static ID3D11DeviceContext * d3d_dc;

static ID3D11RenderTargetView * render_target_view;

static ID3D11DepthStencilView * depth_stencil_view;
static ID3D11Texture2D        * depth_stencil_buffer;

// static ID3D11Buffer * d3d_vertex_buffer_interface;
static ID3D11Buffer * positions_vertex_buffer;
static ID3D11Buffer * colors_vertex_buffer;
static ID3D11Buffer * texture_uvs_vertex_buffer;

static ID3D11Buffer * d3d_index_buffer_interface;

static ID3D11SamplerState * default_sampler_state;

static ID3D11BlendState * transparent_blend_state;

static Shader * dummy_shader;
static Shader * last_shader_set;

static Shader d3d_shader;

static char * last_texture_set;

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
                            swap_chain_desc.BufferDesc         = buffer_desc;
                            swap_chain_desc.SampleDesc.Count   = 1;
                            swap_chain_desc.SampleDesc.Quality = 0;
                            swap_chain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                            swap_chain_desc.BufferCount        = 1;
                            swap_chain_desc.OutputWindow       = (HWND) handle;
                            swap_chain_desc.Windowed           = true;
                            swap_chain_desc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
                            swap_chain_desc.Flags              = 0;

    D3D11CreateDeviceAndSwapChain(  NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
                                    &swap_chain_desc, &swap_chain, &d3d_device, NULL, &d3d_dc);

    ID3D11Texture2D * backbuffer;
    swap_chain->GetBuffer(0, __uuidof(backbuffer), (void**) &backbuffer);

    d3d_device->CreateRenderTargetView(backbuffer, NULL, &render_target_view);
    backbuffer->Release();

    // Depth/Stencil Buffer
    D3D11_TEXTURE2D_DESC    depth_stencil_desc;
                            depth_stencil_desc.Width              = rendering_resolution.width;
                            depth_stencil_desc.Height             = rendering_resolution.height;
                            depth_stencil_desc.MipLevels          = 1;
                            depth_stencil_desc.ArraySize          = 1;
                            depth_stencil_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
                            depth_stencil_desc.SampleDesc.Count   = 1;
                            depth_stencil_desc.SampleDesc.Quality = 0;
                            depth_stencil_desc.Usage              = D3D11_USAGE_DEFAULT;
                            depth_stencil_desc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
                            depth_stencil_desc.CPUAccessFlags     = 0;
                            depth_stencil_desc.MiscFlags          = 0;

    d3d_device->CreateTexture2D(&depth_stencil_desc, NULL, &depth_stencil_buffer);

    d3d_device->CreateDepthStencilView(depth_stencil_buffer, NULL, &depth_stencil_view);

    d3d_dc->OMSetRenderTargets( 1, &render_target_view, depth_stencil_view);

    // Vertex Buffers
    D3D11_BUFFER_DESC   positions_vertex_buffer_desc                  = {};
                        positions_vertex_buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
                        positions_vertex_buffer_desc.ByteWidth        = sizeof(Vector3f) * MAX_VERTEX_BUFFER_SIZE;
                        positions_vertex_buffer_desc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
                        positions_vertex_buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
                        positions_vertex_buffer_desc.MiscFlags        = 0;

    D3D11_BUFFER_DESC   colors_vertex_buffer_desc                     = {};
                        colors_vertex_buffer_desc.Usage               = D3D11_USAGE_DYNAMIC;
                        colors_vertex_buffer_desc.ByteWidth           = sizeof(Vector3f) * MAX_VERTEX_BUFFER_SIZE;
                        colors_vertex_buffer_desc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
                        colors_vertex_buffer_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
                        colors_vertex_buffer_desc.MiscFlags           = 0;

    D3D11_BUFFER_DESC   texture_uvs_vertex_buffer_desc                = {};
                        texture_uvs_vertex_buffer_desc.Usage          = D3D11_USAGE_DYNAMIC;
                        texture_uvs_vertex_buffer_desc.ByteWidth      = sizeof(Vector3f) * MAX_VERTEX_BUFFER_SIZE;
                        texture_uvs_vertex_buffer_desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
                        texture_uvs_vertex_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                        texture_uvs_vertex_buffer_desc.MiscFlags      = 0;

    d3d_device->CreateBuffer(&positions_vertex_buffer_desc,   NULL, &positions_vertex_buffer);
    d3d_device->CreateBuffer(&colors_vertex_buffer_desc,      NULL, &colors_vertex_buffer);
    d3d_device->CreateBuffer(&texture_uvs_vertex_buffer_desc, NULL, &texture_uvs_vertex_buffer);

    ID3D11Buffer * vertex_buffers[] = {positions_vertex_buffer, colors_vertex_buffer, texture_uvs_vertex_buffer};
    UINT strides[]                  = {sizeof(Vector3f),        sizeof(Color4f),      sizeof(Vector2f)};
    UINT offsets[]                  = {0,                       0,                    0};

    d3d_dc->IASetVertexBuffers(0, 3, vertex_buffers, strides, offsets);

    // Index Buffer
    D3D11_BUFFER_DESC   index_buffer_desc = {};
                        index_buffer_desc.Usage          = D3D11_USAGE_DYNAMIC;
                        index_buffer_desc.ByteWidth      = sizeof(int) * MAX_INDEX_BUFFER_SIZE;
                        index_buffer_desc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
                        index_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                        index_buffer_desc.MiscFlags      = 0;

    d3d_device->CreateBuffer(&index_buffer_desc, NULL, &d3d_index_buffer_interface);

    d3d_dc->IASetIndexBuffer(d3d_index_buffer_interface, DXGI_FORMAT_R32_UINT, 0);

    // Input Layout
    d3d_dc->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Viewport
    D3D11_VIEWPORT  viewport;
                    viewport.TopLeftX = 0;
                    viewport.TopLeftY = 0;
                    viewport.Width    = rendering_resolution.width;
                    viewport.Height   = rendering_resolution.height;
                    viewport.MinDepth = 0.0f;
                    viewport.MaxDepth = 1.0f;

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

    // Blending
    D3D11_RENDER_TARGET_BLEND_DESC render_target_blend_desc;

    render_target_blend_desc.BlendEnable           = true;
    render_target_blend_desc.SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    render_target_blend_desc.DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    render_target_blend_desc.BlendOp               = D3D11_BLEND_OP_ADD;
    render_target_blend_desc.SrcBlendAlpha         = D3D11_BLEND_ONE;
    render_target_blend_desc.DestBlendAlpha        = D3D11_BLEND_ZERO;
    render_target_blend_desc.BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    render_target_blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    D3D11_BLEND_DESC blend_desc;
    blend_desc.AlphaToCoverageEnable  = false;
    blend_desc.IndependentBlendEnable = false;
    blend_desc.RenderTarget[0]        = render_target_blend_desc;

    d3d_device->CreateBlendState(&blend_desc, &transparent_blend_state);

    d3d_dc->OMSetBlendState(transparent_blend_state, NULL, 0xffffffff);

    d3d_dc->PSSetSamplers(0, 1, &default_sampler_state);

    // Init shader layouts @Temporary
    pos_layout_desc.num_inputs     = 1;
	pos_layout_desc.desc           = (D3D11_INPUT_ELEMENT_DESC * ) malloc(sizeof(D3D11_INPUT_ELEMENT_DESC) * pos_layout_desc.num_inputs);
	pos_layout_desc.desc[0]        = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	pos_uv_layout_desc.num_inputs  = 2;
	pos_uv_layout_desc.desc        = (D3D11_INPUT_ELEMENT_DESC * ) malloc(sizeof(D3D11_INPUT_ELEMENT_DESC) * pos_uv_layout_desc.num_inputs);
    pos_uv_layout_desc.desc[0]     = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 };
    pos_uv_layout_desc.desc[1]     = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

    pos_col_layout_desc.num_inputs = 2;
	pos_col_layout_desc.desc       = (D3D11_INPUT_ELEMENT_DESC * ) malloc(sizeof(D3D11_INPUT_ELEMENT_DESC) * pos_col_layout_desc.num_inputs);
    pos_col_layout_desc.desc[0]    = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 };
    pos_col_layout_desc.desc[1]    = { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

    dummy_shader = (Shader *) malloc(sizeof(Shader));

    create_shader("dummy_shader.hlsl", POS, dummy_shader);
}

void draw_frame(GraphicsBuffer * graphics_buffer, int num_buffers, TextureManager * texture_manager) {
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

static void update_buffers(DrawBatch * batch, ShaderInputMode mode) {
    // Update vertex buffers
    D3D11_MAPPED_SUBRESOURCE resource = {};
    d3d_dc->Map(positions_vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    memcpy(resource.pData, batch->positions.data, sizeof(Vector3f) * batch->positions.count);
    d3d_dc->Unmap(positions_vertex_buffer, 0);

    if(mode == POS_COL) {
        resource = {};
        d3d_dc->Map(colors_vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        memcpy(resource.pData, batch->colors.data, sizeof(Color4f) * batch->colors.count);
        d3d_dc->Unmap(colors_vertex_buffer, 0);
    }

    if(mode == POS_UV) {
        resource = {};
        d3d_dc->Map(texture_uvs_vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
        memcpy(resource.pData, batch->texture_uvs.data, sizeof(Vector2f) * batch->texture_uvs.count);
        d3d_dc->Unmap(texture_uvs_vertex_buffer, 0);
    }

    // Update index buffer
    resource = {};
    d3d_dc->Map(d3d_index_buffer_interface, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    memcpy(resource.pData, batch->indices.data, sizeof( int ) *  batch->indices.count);
    d3d_dc->Unmap(d3d_index_buffer_interface, 0);
}

// @Rewrite This is really ugly, rewrite and refactor things
static void draw_buffer(GraphicsBuffer * graphics_buffer, int buffer_index, TextureManager * texture_manager) {

    DrawBatch * batch = &graphics_buffer->batches.data[buffer_index];

	Shader * shader = batch->info.shader;

	if (shader != last_shader_set) {
		switch_to_shader(shader);
		last_shader_set = shader;
	}

    update_buffers(batch, shader->input_mode);

    if(d3d_shader.input_mode == POS_UV) {

        char * texture_name = batch->info.texture;

        Texture * texture = (Texture *) texture_manager->table.find(texture_name);

        if(texture == NULL) {
            log_print("draw_buffer", "Texture %s was not found in the catalog", texture_name);
            return;
        }

        // This texture was modified, so let's reset its SRV. @Incomplete @Speed, we can probably
        // just remap the data if the size and bit depth stay the same.
        if(texture->dirty) {
            texture->platform_info->srv->Release(); // Release the D3D interface
            free(texture->platform_info);
        }


        // This texture doesn't have its platform info, let's give it one
        if(texture->platform_info == NULL) {
            texture->platform_info = (PlatformTextureInfo *) malloc(sizeof(PlatformTextureInfo));
            bind_srv_to_texture(texture);
        }

        // Send texture to shader if it's different from last time
		if (last_texture_set == NULL || !(strcmp(last_texture_set, texture_name) == 0)) {
			d3d_dc->PSSetShaderResources(0, 1, &texture->platform_info->srv);
			last_texture_set = texture_name;
		}

    } else if(d3d_shader.input_mode == POS_COL) {
        // Nothing to do

    } else {
        // @Temporary, we need to fix dummy shader so that it can accept any input.
        if(d3d_shader.input_mode != NONE) {
            log_print("draw_buffer", "Unsupported shader input mode");
        }
        return;
    }

    d3d_dc->DrawIndexed(batch->indices.count, 0, 0);
}

static void bind_srv_to_texture(Texture * texture) {
    D3D11_TEXTURE2D_DESC texture_desc;
                         texture_desc.Width              = texture->width;
                         texture_desc.Height             = texture->height;
                         texture_desc.MipLevels          = 1;
                         texture_desc.ArraySize          = 1;
                         texture_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
                         texture_desc.SampleDesc.Count   = 1;
                         texture_desc.SampleDesc.Quality = 0;
                         texture_desc.Usage              = D3D11_USAGE_DEFAULT;
                         texture_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
                         texture_desc.CPUAccessFlags     = 0;
                         texture_desc.MiscFlags          = 0;

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
                                    srv_desc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
                                    srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                                    srv_desc.Texture2D.MostDetailedMip = 0;
                                    srv_desc.Texture2D.MipLevels       = 1;

    // Our fonts are greyscale, so let's make a special case for those
    if(texture->bytes_per_pixel == 1) {
        srv_desc.Format = DXGI_FORMAT_A8_UNORM;
    }

    d3d_device->CreateShaderResourceView(d3d_texture, &srv_desc, &texture->platform_info->srv);
}

struct String {
    char * data  = NULL;
    int    count = 0;

    inline char& operator[](int i) {
        return this->data[i];
    }

    String& operator=(String & string) {
        this->count = string.count;
        this->data  = string.data;

        return *this;
    }
};

char * to_c_string(String string) { // @Incomplete take pointer here ?
    char * c = (char *) malloc(string.count + 1);
    memcpy(c, string.data, string.count);
    c[string.count] = 0;

    return c;
}

void push(String * string, int amount = 1) {
    string->data  += amount;
    string->count -= amount;
}

inline void cut_spaces(String * string) {
    while((*string)[0] == ' ') push(string);
}

String bump_to_next_line(String * string) {
    cut_spaces(string);

    String line;
    line.data  = string->data;
    line.count = 0;

    // Empty line / EOF check.
    {
        if((*string)[0] == '\0') {
            line.count = -1; // @Temporary, this signals that we should stop getting new lines from the buffer, we should probably find a better way to do it.
            return line; // EOF
        }

        if((*string)[0] == '\r') {
            push(string, 2); // CLRF
            return line;
        }

        if((*string)[0] == '\n') {
            push(string);
            return line;
        }
    }

    while(true) {

        line.count  += 1;
        push(string);

        // End of line/file check
        {
            if((*string)[0] == '\0') {
                break; // EOF
            }

            if((*string)[0] == '\n') {
                push(string);
                break;
            }

            if((*string)[0] == '\r') {
                push(string, 2);
                break;
            }
        }
    }

    return line;
}


// Right-side inclusive
String cut_until_char(char c, String * string) {
    String left;
    left.data  = string->data;
    left.count = 0;

    while(string->count > 0) {
        if((*string)[0] == c) break;
        push(string);
        left.count += 1;
    }

    return left;
}

// @Temporary: Make this use cut_until_char?
String cut_until_space(String * string) {

    String left = cut_until_char(' ', string);

    cut_spaces(string);

    return left;

}

void parse_input_layout_from_file(char * file_name, char * c_file_data, Shader * shader) {
    printf("Parsing shader file %s\n\n", file_name);

    String file_data;
    file_data.data = c_file_data;
    file_data.count = strlen(c_file_data);

    String line = bump_to_next_line(&file_data);

    while(line.count >= 0) {
           // printf("    %s\n", c_string);

        {
            String left = cut_until_space(&line);

            char * c_left = to_c_string(left);
            scope_exit(free(c_left));

            if(strcmp(c_left, "VS_FUNC") == 0) { // @Incomplete @Speed Use a memcmp here, no need to malloc a string.
                char * c_string = to_c_string(line);
                scope_exit(free(c_string));
                printf("----- Found VS function : %s\n", c_string);

                cut_until_char('(', &line);
                String args = cut_until_char(')', &line);
                int arg_index = 0;
                while(args.count > 0) { // Go until end of line
                    cut_until_char(':', &args);

                    push(&args); // We don't want the ':'
                    cut_spaces(&args);

                    String token = cut_until_space(&args);

                    if(token[token.count -1] == ',') token.count -= 1; // If we have a ',' or a ')' after the input type, cut it.
                    if(token.count == 0) continue; // Token was just ',' so we move on to the next one.

                    char * c_token = to_c_string(token); // @Incomplete @Speed Use a memcmp here, no need to malloc a string.
                    scope_exit(free(c_token));

                    if     (strcmp("POSITION", c_token) == 0) shader->position_index = arg_index;
                    else if(strcmp("COLOR",    c_token) == 0) shader->color_index    = arg_index;
                    else if(strcmp("TEXCOORD", c_token) == 0) shader->uv_index       = arg_index;

                    arg_index += 1;
                }


            }
        }

        line = bump_to_next_line(&file_data);
    }

    printf("--------> Parsed inputs:\n", shader->position_index, shader->color_index, shader->uv_index);

    if(shader->position_index >= 0) printf("                POSITION @ index %d\n", shader->position_index);
    if(shader->color_index    >= 0) printf("                COLOR    @ index %d\n", shader->color_index);
    if(shader->uv_index       >= 0) printf("                TEXCOORD @ index %d\n", shader->uv_index);


	printf("\n");
}

bool compile_shader(Shader * shader) {
    shader->VS = NULL;
    shader->PS = NULL;

    shader->position_index = -1;
    shader->color_index    = -1;
    shader->uv_index       = -1;

    ID3DBlob * VS_bytecode;
    ID3DBlob * PS_bytecode;

    ID3DBlob * error = NULL;


    char path[512];
    snprintf(path, 512, "data/shaders/%s", shader->filename);
    HANDLE file_handle = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    int file_size = GetFileSize(file_handle, NULL);

    char * file_data = (char *) malloc(file_size);
    scope_exit(free(file_data));

    DWORD shader_size = 0;

    if(!ReadFile(file_handle, file_data, file_size, &shader_size, NULL)) {
        log_print("compile_shader", "An error occured while reading the file \"%s\", could not compile the shaders. Error code is 0x%x",
                  shader->filename, GetLastError());

        return false;
    }

    HRESULT error_code = D3DCompile(file_data, file_size, NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                       "VS", "vs_5_0", 0, 0, &VS_bytecode, &error);

    if(error) {
        log_print("compile_shader", "Vertex shader in %s failed to compile, error given is : \n---------\n%s---------", shader->filename, (char *)error->GetBufferPointer());

        error->Release();

        return false;
    }

    if (error_code > S_FALSE) {
        log_print("compile_shader",
                  "An error occured while compiling the file \"%s\", could not compile the shaders. Error code is 0x%x",
                  shader->filename, error_code);

        return false;
    }

    D3DCompile(file_data, file_size, NULL, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                       "PS", "ps_5_0", 0, 0, &PS_bytecode, &error);

    if(error) {
        log_print("compile_shader", "Pixel shader in %s failed to compile, error given is : \n---------\n%s---------", shader->filename, (char *)error->GetBufferPointer());

        error->Release();
        VS_bytecode->Release();

        return false;
    }

    // @Temporary, @Hack, @ThisIsTerrible

    if(strcmp(shader->filename, "dummy_shader.hlsl")    == 0) { shader->input_mode = POS; }
    if(strcmp(shader->filename, "textured_shader.hlsl") == 0) { shader->input_mode = POS_UV; }
    if(strcmp(shader->filename, "font_shader.hlsl")     == 0) { shader->input_mode = POS_UV; }
    if(strcmp(shader->filename, "colored_shader.hlsl")  == 0) { shader->input_mode = POS_COL; }

    parse_input_layout_from_file(shader->filename, file_data, shader);

	InputLayoutDesc * layout = NULL;

	switch (shader->input_mode) {
		case POS: {
			layout = &pos_layout_desc;
			break;
		} case POS_UV: {
			layout = &pos_uv_layout_desc;
			break;
		} case POS_COL: {
			layout = &pos_col_layout_desc;
			break;
		}
	}

	if (layout) {
		d3d_device->CreateInputLayout(layout->desc, layout->num_inputs, VS_bytecode->GetBufferPointer(),
			VS_bytecode->GetBufferSize(), (ID3D11InputLayout **)&shader->input_layout);

		d3d_device->CreateVertexShader(VS_bytecode->GetBufferPointer(), VS_bytecode->GetBufferSize(),
			NULL, (ID3D11VertexShader **)&shader->VS);

		d3d_device->CreatePixelShader(PS_bytecode->GetBufferPointer(), PS_bytecode->GetBufferSize(),
			NULL, (ID3D11PixelShader **)&shader->PS);
	} else {
		log_print("compile_shader", "Could not compile shader %s, the input mode was unknown (value was %d)", shader->filename, shader->input_mode);
	}

    VS_bytecode->Release();
    PS_bytecode->Release();

    return true;
}

static bool create_shader(char * filename, ShaderInputMode input_mode, Shader * shader){
    shader->filename = filename;

    shader->input_mode = input_mode;

    char path[512];;
    snprintf(path, 512, "data/shaders/%s", shader->filename);

    return compile_shader(shader);
}

static void switch_to_shader(Shader * shader) {
    if(shader->VS == NULL) {
        d3d_shader.VS           = dummy_shader->VS;
		d3d_shader.input_layout = dummy_shader->input_layout;
        d3d_shader.input_mode   = dummy_shader->input_mode;
    } else {
        d3d_shader.VS           = shader->VS;
		d3d_shader.input_layout = shader->input_layout;
        d3d_shader.input_mode   = shader->input_mode;
    }

    if(shader->PS == NULL) {
        d3d_shader.PS = dummy_shader->PS;
    } else {
        d3d_shader.PS = shader->PS;
    }

    d3d_dc->IASetInputLayout((ID3D11InputLayout *) d3d_shader.input_layout);
    d3d_dc->VSSetShader((ID3D11VertexShader *)     d3d_shader.VS, NULL, 0);
    d3d_dc->PSSetShader((ID3D11PixelShader *)      d3d_shader.PS, NULL, 0);
}
