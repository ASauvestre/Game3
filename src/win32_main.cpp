#include "win32_main.h"

// From "windowsx.h" 
// Gets Low short and high short from LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

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

// Extern Globals
Shader * font_shader;
Shader * textured_shader;
Shader * colored_shader;
Shader * dummy_shader;

// Globals
static HWND handle;
static HDC device_context;

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

// static ID3D11BlendState* transparent_blend_state;

static bool should_quit = false;

static Keyboard keyboard = {};
static WindowData window_data = {};
static GraphicsBuffer graphics_buffer;

static TextureManager texture_manager;

const int MAX_VERTEX_BUFFER_SIZE 	 = 4096;
const int MAX_INDEX_BUFFER_SIZE 	 = 4096;
const int MAX_TEXTURE_INDEX_MAP_SIZE = 2048;

// Perf counters
LARGE_INTEGER start_time, end_time;
LARGE_INTEGER frequency;

void update_window_events() {
	MSG message;
	while(PeekMessage(&message, handle, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

LRESULT CALLBACK WndProc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
	switch(message) {
		case WM_QUIT :
		case WM_CLOSE :
		case WM_DESTROY : {
			should_quit = true;
			break;
		}
		case WM_KEYDOWN : {

			// if(l_param & 0x40000000) { break; } // Bit 30 "The previous key state. The value is 1 if the key is down 
												// before the message is sent, or it is zero if the key is up." (MSDN)

			switch(w_param) {
				case VK_LEFT : {
					keyboard.key_left = true;
					break;
				}
				case VK_RIGHT : {
					keyboard.key_right = true;
					break;
				}
				case VK_UP : {
					keyboard.key_up = true;
					break;
				}
				case VK_DOWN : {
					keyboard.key_down = true;
					break;
				}
				case VK_SHIFT : {
					keyboard.key_shift = true;
					break;
				}
				case VK_CONTROL : {
					break;
				}
				case VK_SPACE : {
					keyboard.key_space = true;
					break;
				}
				case VK_F1 : {
					keyboard.key_F1 = true;
					break;
				}
				case VK_F2 : {
					keyboard.key_F2 = true;
					break;
				}
				case VK_F3 : {
					keyboard.key_F3 = true;
					break;
				}
				case VK_ESCAPE : {
					break;
				}
				default : {
					// printf("Keyboard Event: Unknown key was pressed.\n");
				}
			}
			break;	
		}
		case WM_KEYUP : {
			switch(w_param) {
				case VK_LEFT : {
					keyboard.key_left = false;
					break;
				}
				case VK_RIGHT : {
					keyboard.key_right = false;
					break;
				}
				case VK_UP : {
					keyboard.key_up = false;
					break;
				}
				case VK_DOWN : {
					keyboard.key_down = false;
					break;
				}
				case VK_SHIFT : {
					keyboard.key_shift = false;
					break;
				}
				case VK_CONTROL : {
					break;
				}
				case VK_SPACE : {
					keyboard.key_space = false;
					break;
				}
				case VK_F1 : {
					keyboard.key_F1 = false;
					break;
				}
				case VK_F2 : {
					keyboard.key_F2 = false;
					break;
				}
				case VK_F3 : {
					keyboard.key_F3 = false;
					break;
				}
				case VK_ESCAPE : {
					SendMessage(window_handle, WM_QUIT, NULL, NULL);
					break;
				}
				default : {
					// printf("Keyboard Event: Unknown key was released.\n");
				}
			}
			break;
		}	
		case WM_MOUSEMOVE : {
			if(window_data.width == 0) {
				keyboard.mouse_position.x = 0.0f;
			} else {
				keyboard.mouse_position.x = (float)GET_X_LPARAM(l_param) / window_data.width;
			}

			if(window_data.height == 0) {
				keyboard.mouse_position.y = 0.0f;
			} else {
				keyboard.mouse_position.y = (float)GET_Y_LPARAM(l_param) / window_data.height;
			}

			break;
		}
		case WM_LBUTTONDOWN : {
			keyboard.mouse_left = true;

			// SetCapture allows us to detect mouse up if the cursor leaves the window.
			SetCapture(window_handle);


			// Store the position at which the button was pressed @Pasted
			if(window_data.width == 0) {
				keyboard.mouse_left_pressed_position.x = 0.0f;
			} else {
				keyboard.mouse_left_pressed_position.x = (float)GET_X_LPARAM(l_param) / window_data.width;
			}

			if(window_data.height == 0) {
				keyboard.mouse_left_pressed_position.y = 0.0f;
			} else {
				keyboard.mouse_left_pressed_position.y = (float)GET_Y_LPARAM(l_param) / window_data.height;
			}

			break;
		}
		case WM_LBUTTONUP : {
			keyboard.mouse_left = false;
			ReleaseCapture();
			break;
		}
		case WM_RBUTTONDOWN : {
			keyboard.mouse_right = true;

			// SetCapture allows us to detect mouse up if the cursor leaves the window.
			SetCapture(window_handle);

			// Store the position at which the button was pressed @Pasted
			if(window_data.width == 0) {
				keyboard.mouse_right_pressed_position.x = 0.0f;
			} else {
				keyboard.mouse_right_pressed_position.x = (float)GET_X_LPARAM(l_param) / window_data.width;
			}

			if(window_data.height == 0) {
				keyboard.mouse_right_pressed_position.y = 0.0f;
			} else {
				keyboard.mouse_right_pressed_position.y = (float)GET_Y_LPARAM(l_param) / window_data.height;
			}

			break;
		}
		case WM_RBUTTONUP : {
			keyboard.mouse_right= false;
			ReleaseCapture();
			break;
		}
	}
	return DefWindowProc(window_handle, message, w_param, l_param);
}

bool create_window(int width, int height, char * name) {
	HINSTANCE instance = GetModuleHandle(NULL);
	WNDCLASSEX 	window_class = {};
				window_class.cbSize 		= sizeof(WNDCLASSEX);
				window_class.style  		= CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
				window_class.hCursor 		= LoadCursor(NULL, IDC_ARROW);
				window_class.lpfnWndProc 	= WndProc;
				window_class.hInstance 		= instance;
				window_class.lpszClassName 	= "Win32WindowClass";

	if(RegisterClassEx(&window_class)) {

		RECT window_dimensions = {};
		window_dimensions.right = width;
		window_dimensions.bottom = height;

		int window_style = WS_CAPTION | WS_SIZEBOX | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

		AdjustWindowRect(&window_dimensions, window_style, false);

		int centered_x = (GetSystemMetrics(SM_CXSCREEN)-width)/2;
		int centered_y = (GetSystemMetrics(SM_CYSCREEN)-height)/2;

		handle = CreateWindowEx(0, window_class.lpszClassName, (LPCTSTR) name, window_style, centered_x, 
								centered_y, window_dimensions.right - window_dimensions.left, 
								window_dimensions.bottom - window_dimensions.top, NULL, NULL, instance, NULL);

		if(handle) {
			device_context = GetDC(handle);
			if(device_context) {
				//ShowCursor(false);
				ShowWindow(handle, 1);
				UpdateWindow(handle);

			}
		}
	}
	return false;
}

Shader current_shader;

void set_shader(Shader * shader) {
	if(shader->VS == NULL) {
		current_shader.input_format.layout = dummy_shader->input_format.layout;
		current_shader.VS = dummy_shader->VS;
		current_shader.input_mode = dummy_shader->input_mode;
	} else {
		current_shader.input_format.layout = shader->input_format.layout;
		current_shader.VS = shader->VS;
		current_shader.input_mode = shader->input_mode;
	}

	if(shader->PS == NULL) {
		current_shader.PS = dummy_shader->PS;
	} else {
		current_shader.PS = shader->PS;
	}

	d3d_dc->IASetInputLayout(current_shader.input_format.layout);
	d3d_dc->VSSetShader(current_shader.VS, NULL, 0);	
	d3d_dc->PSSetShader(current_shader.PS, NULL, 0);
}

ID3D11BlendState * transparent_blend_state;

void init_d3d() {

	DXGI_MODE_DESC 	buffer_desc = {};
					buffer_desc.Width 					= window_data.width;
					buffer_desc.Height 					= window_data.height;
					buffer_desc.RefreshRate.Numerator 	= 60;
					buffer_desc.RefreshRate.Denominator = 1;
					buffer_desc.Format 					= DXGI_FORMAT_R8G8B8A8_UNORM;
					buffer_desc.ScanlineOrdering 		= DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
					buffer_desc.Scaling 				= DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SWAP_CHAIN_DESC 	swap_chain_desc = {};
							swap_chain_desc.BufferDesc 			= buffer_desc;
							swap_chain_desc.SampleDesc.Count 	= 1;
							swap_chain_desc.SampleDesc.Quality 	= 0;
							swap_chain_desc.BufferUsage 		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
							swap_chain_desc.BufferCount 		= 1;
							swap_chain_desc.OutputWindow 		= handle;
							swap_chain_desc.Windowed 			= true;
							swap_chain_desc.SwapEffect 			= DXGI_SWAP_EFFECT_DISCARD;
							swap_chain_desc.Flags 				= 0;

	D3D11CreateDeviceAndSwapChain(	NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, 
									&swap_chain_desc, &swap_chain, &d3d_device, NULL, &d3d_dc);

	ID3D11Texture2D * backbuffer;
	swap_chain->GetBuffer(0, __uuidof(backbuffer), (void**) &backbuffer);

	d3d_device->CreateRenderTargetView(backbuffer, NULL, &render_target_view);
	backbuffer->Release();

	// Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC 	depth_stencil_desc;
							depth_stencil_desc.Width 				= window_data.width;
							depth_stencil_desc.Height 				= window_data.height;
							depth_stencil_desc.MipLevels 			= 1;
							depth_stencil_desc.ArraySize 			= 1;
							depth_stencil_desc.Format 				= DXGI_FORMAT_D24_UNORM_S8_UINT;
							depth_stencil_desc.SampleDesc.Count 	= 1;
							depth_stencil_desc.SampleDesc.Quality 	= 0;
							depth_stencil_desc.Usage 				= D3D11_USAGE_DEFAULT;
							depth_stencil_desc.BindFlags 			= D3D11_BIND_DEPTH_STENCIL;
							depth_stencil_desc.CPUAccessFlags 		= 0;
							depth_stencil_desc.MiscFlags 			= 0;

	d3d_device->CreateTexture2D(&depth_stencil_desc, NULL, &depth_stencil_buffer);

	d3d_device->CreateDepthStencilView(depth_stencil_buffer, NULL, &depth_stencil_view);

	d3d_dc->OMSetRenderTargets( 1, &render_target_view, depth_stencil_view);

	// Vectex Buffer
	D3D11_BUFFER_DESC 	vertex_buffer_desc = {};
						vertex_buffer_desc.Usage 			= D3D11_USAGE_DYNAMIC;
						vertex_buffer_desc.ByteWidth 		= sizeof(Vertex) * MAX_VERTEX_BUFFER_SIZE;
						vertex_buffer_desc.BindFlags 		= D3D11_BIND_VERTEX_BUFFER;
						vertex_buffer_desc.CPUAccessFlags 	= D3D11_CPU_ACCESS_WRITE;
						vertex_buffer_desc.MiscFlags 		= 0;

	d3d_device->CreateBuffer(&vertex_buffer_desc, NULL, &d3d_vertex_buffer_interface);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3d_dc->IASetVertexBuffers(0, 1, &d3d_vertex_buffer_interface, &stride, &offset);

	// Index Buffer
	D3D11_BUFFER_DESC 	index_buffer_desc = {};
						index_buffer_desc.Usage 			= D3D11_USAGE_DYNAMIC;
						index_buffer_desc.ByteWidth 		= sizeof(int) * MAX_INDEX_BUFFER_SIZE;
						index_buffer_desc.BindFlags 		= D3D11_BIND_INDEX_BUFFER;
						index_buffer_desc.CPUAccessFlags 	= D3D11_CPU_ACCESS_WRITE;
						index_buffer_desc.MiscFlags 		= 0;

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
	D3D11_VIEWPORT 	viewport;
					viewport.TopLeftX 	= 0;
					viewport.TopLeftY 	= 0;
					viewport.Width 		= window_data.width;
					viewport.Height 	= window_data.height;
					viewport.MinDepth 	= 0.0f;
					viewport.MaxDepth 	= 1.0f;

	d3d_dc->RSSetViewports(1, &viewport);

	// Sampler State
	D3D11_SAMPLER_DESC 	sampler_desc;
						sampler_desc.Filter 		= D3D11_FILTER_MIN_MAG_MIP_LINEAR;
						sampler_desc.AddressU 		= D3D11_TEXTURE_ADDRESS_WRAP;
						sampler_desc.AddressV 		= D3D11_TEXTURE_ADDRESS_WRAP;
						sampler_desc.AddressW 		= D3D11_TEXTURE_ADDRESS_WRAP;
						sampler_desc.MipLODBias 	= 0.0f;
						sampler_desc.MaxAnisotropy 	= 0;
						sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
						sampler_desc.BorderColor[4] = {};
						sampler_desc.MinLOD 		= 0;
						sampler_desc.MaxLOD 		= D3D11_FLOAT32_MAX;

	d3d_device->CreateSamplerState(&sampler_desc, &default_sampler_state);

	// Texture index map buffer
	D3D11_BUFFER_DESC texture_map_index_buffer_desc;
	texture_map_index_buffer_desc.ByteWidth 			= sizeof(int) * MAX_TEXTURE_INDEX_MAP_SIZE;
	texture_map_index_buffer_desc.Usage 				= D3D11_USAGE_DYNAMIC;
	texture_map_index_buffer_desc.BindFlags 			= D3D11_BIND_CONSTANT_BUFFER;
	texture_map_index_buffer_desc.CPUAccessFlags 		= D3D11_CPU_ACCESS_WRITE;
	texture_map_index_buffer_desc.MiscFlags 			= 0;
	texture_map_index_buffer_desc.StructureByteStride 	= 0;

	d3d_device->CreateBuffer( &texture_map_index_buffer_desc, NULL, &d3d_texture_index_map_buffer );

	// Blending	
	D3D11_RENDER_TARGET_BLEND_DESC render_target_blend_desc;

	render_target_blend_desc.BlendEnable 			= true;
	render_target_blend_desc.SrcBlend 				= D3D11_BLEND_SRC_ALPHA;
	render_target_blend_desc.DestBlend 				= D3D11_BLEND_INV_SRC_ALPHA;
	render_target_blend_desc.BlendOp 				= D3D11_BLEND_OP_ADD;
	render_target_blend_desc.SrcBlendAlpha 			= D3D11_BLEND_ONE;
	render_target_blend_desc.DestBlendAlpha 		= D3D11_BLEND_ZERO;
	render_target_blend_desc.BlendOpAlpha 			= D3D11_BLEND_OP_ADD;
	render_target_blend_desc.RenderTargetWriteMask 	= D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC blend_desc;
	blend_desc.AlphaToCoverageEnable 	= false;
	blend_desc.IndependentBlendEnable 	= false;
	blend_desc.RenderTarget[0]			= render_target_blend_desc;

	d3d_device->CreateBlendState(&blend_desc, &transparent_blend_state);

	d3d_dc->OMSetBlendState(transparent_blend_state, NULL, 0xffffffff);
}

void draw_buffer(int buffer_index) {

	set_shader(graphics_buffer.shaders[buffer_index]);

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

void draw_frame() {
	
	// Clear view
	float color_array[4] = {0.0f, 0.0f, 0.1f, 1.0f};

	d3d_dc->ClearRenderTargetView(render_target_view, color_array);
	d3d_dc->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	for(int i = 0; i < graphics_buffer.vertex_buffers.size(); i++) {
		// Draw buffer
		draw_buffer(i);

	}

	swap_chain->Present(0, 0);  

	// Clear buffer;
	graphics_buffer.vertex_buffers.clear();
	graphics_buffer.index_buffers.clear();
	graphics_buffer.texture_id_buffer.clear();
	graphics_buffer.shaders.clear();
}

void bind_srv_to_texture(Texture * texture) {
	D3D11_TEXTURE2D_DESC texture_desc;
						 texture_desc.Width 				= texture->width;
						 texture_desc.Height 				= texture->height;
						 texture_desc.MipLevels 			= 1;
						 texture_desc.ArraySize 			= 1;
						 texture_desc.Format 				= DXGI_FORMAT_R8G8B8A8_UNORM;
						 texture_desc.SampleDesc.Count 		= 1;
						 texture_desc.SampleDesc.Quality 	= 0;
						 texture_desc.Usage 				= D3D11_USAGE_DEFAULT;
						 texture_desc.BindFlags 			= D3D11_BIND_SHADER_RESOURCE;
						 texture_desc.CPUAccessFlags 		= 0;
						 texture_desc.MiscFlags 			= 0;

	// Our fonts are greyscale, so let's make a special case for those
	if(texture->bytes_per_pixel == 1) {
		texture_desc.Format = DXGI_FORMAT_A8_UNORM;
	}

	D3D11_SUBRESOURCE_DATA texture_subresource;
	texture_subresource.pSysMem 			= texture->bitmap;
	texture_subresource.SysMemPitch			= texture->width_in_bytes;
	texture_subresource.SysMemSlicePitch	= texture->num_bytes;

	ID3D11Texture2D * d3d_texture;

	d3d_device->CreateTexture2D(&texture_desc, &texture_subresource, &d3d_texture);

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
									srv_desc.Format 						= DXGI_FORMAT_R8G8B8A8_UNORM;
									srv_desc.ViewDimension 					= D3D11_SRV_DIMENSION_TEXTURE2D;
									srv_desc.Texture2D.MostDetailedMip 		= 0;
									srv_desc.Texture2D.MipLevels 			= 1;

	// Our fonts are greyscale, so let's make a special case for those
	if(texture->bytes_per_pixel == 1) {
		srv_desc.Format = DXGI_FORMAT_A8_UNORM;
	}

	d3d_device->CreateShaderResourceView(d3d_texture, &srv_desc, (ID3D11ShaderResourceView **) &texture->platform_info->srv);

}

// Shader code
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

void recompile_shaders() {
	compile_shader(font_shader);
	compile_shader(textured_shader);
	compile_shader(colored_shader);
}
void check_specific_shader_file_modification(Shader * shader) {
	FILETIME new_last_modified;

	GetFileTime(shader->file_handle, NULL, NULL, &new_last_modified);

	if((new_last_modified.dwLowDateTime  != shader->last_modified.dwLowDateTime)  || 
	   (new_last_modified.dwHighDateTime != shader->last_modified.dwHighDateTime)) {

		// log_print("recompile_shaders", "Previous time : %u %u, New time : %u %u",
		// 		  shader->last_modified.dwHighDateTime, shader->last_modified.dwLowDateTime,
		// 		  new_last_modified.dwHighDateTime, new_last_modified.dwLowDateTime);

		// Get new handle
		char path[512];;
		snprintf(path, 512, "shaders/%s", shader->filename);

		shader->file_handle = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		log_print("recompile_shaders", "Detected file modification, recompiling shader \"%s\"", shader->filename);

		compile_shader(shader);
		
		shader->last_modified = new_last_modified;
	}
}

void check_shader_files_modification() {
	
	check_specific_shader_file_modification(dummy_shader);
	check_specific_shader_file_modification(textured_shader);
	check_specific_shader_file_modification(colored_shader);
	check_specific_shader_file_modification(font_shader);
}

const int TARGET_FPS = 60;

void main() {
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start_time);

	window_data.width 	= 1280;
	window_data.height 	= 720;

	// window_data.width 	= 1920;
	// window_data.height 	= 1080;

	window_data.aspect_ratio = (float) window_data.width/window_data.height;
	char * window_name = "Game3";

	create_window(window_data.width, window_data.height, window_name);

	init_d3d();

	init_shaders();

	init_game(&texture_manager);

	Keyboard previous_keyboard;

	QueryPerformanceCounter(&end_time);
	float frame_time = ((float)(end_time.QuadPart - start_time.QuadPart)*1000/(float)frequency.QuadPart);
	log_print("perf_counter", "Startup time : %.3f seconds", frame_time/1000.f);

	while(!should_quit) {
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&start_time);

		check_shader_files_modification();

		update_window_events();

		game(&window_data, &keyboard, &previous_keyboard, &graphics_buffer, &texture_manager);
		draw_frame();

		previous_keyboard = keyboard; // @Framerate

		QueryPerformanceCounter(&end_time);
		
		frame_time = ((float)(end_time.QuadPart - start_time.QuadPart)*1000/(float)frequency.QuadPart);

		if (window_data.locked_fps) {
			while (frame_time < 1000.0f / TARGET_FPS) {
				Sleep(0);

				QueryPerformanceCounter(&end_time);

				frame_time = ((float)(end_time.QuadPart - start_time.QuadPart) * 1000 / (float)frequency.QuadPart);
			}
		}
		window_data.frame_time = frame_time;
		// log_print("perf_counter", "Frame time : %f", frame_time);
	}
}