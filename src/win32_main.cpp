#include "win32_main.h"

struct Texture {
	int width;
	int height;
	int bytes_per_pixel;
	char * name;
	ID3D11ShaderResourceView * srv;
	ID3D11SamplerState * sampler_state;
};

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

static ID3D11VertexShader * VS;
static ID3D11PixelShader * PS;
static ID3DBlob * VS_bytecode;
static ID3DBlob * PS_bytecode;

static ID3D11InputLayout * vertex_layout;
static D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
};

static ID3D11SamplerState * default_sampler_state;

static std::vector<Texture*> textures;

static bool should_quit = false;

static Keyboard keyboard = {};
static WindowData window_data = {};
static GraphicsBuffer graphics_buffer;

const int MAX_VERTEX_BUFFER_SIZE 	= 1024;
const int MAX_INDEX_BUFFER_SIZE 	= 1024;

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

			if(l_param & 0x40000000) { break; } // Bit 30 "The previous key state. The value is 1 if the key is down 
												// before the message is sent, or it is zero if the key is up." (MSDN)

			switch(w_param) {
				case VK_LEFT : {
					printf("Keyboard Event: Key LEFT was pressed.\n");
					keyboard.key_left = true;
					break;
				}
				case VK_RIGHT : {
					printf("Keyboard Event: Key RIGHT was pressed.\n");
					keyboard.key_right = true;
					break;
				}
				case VK_UP : {
					printf("Keyboard Event: Key UP was pressed.\n");
					keyboard.key_up = true;
					break;
				}
				case VK_DOWN : {
					printf("Keyboard Event: Key DOWN was pressed.\n");
					keyboard.key_down = true;
					break;
				}
				case VK_SHIFT : {
					printf("Keyboard Event: Key SHIFT was pressed.\n");
					break;
				}
				case VK_CONTROL : {
					printf("Keyboard Event: Key CONTROL was pressed.\n");
					break;
				}
				case VK_SPACE : {
					printf("Keyboard Event: Key SPACE was pressed.\n");
					break;
				}
				case VK_ESCAPE : {
					printf("Keyboard Event: Key ESCAPE was pressed.\n");
					break;
				}
				default : {
					printf("Keyboard Event: Unknown key was pressed.\n");
				}
			}
			break;	
		}
		case WM_KEYUP : {
			switch(w_param) {
				case VK_LEFT : {
					printf("Keyboard Event: Key LEFT was released.\n");
					keyboard.key_left = false;
					break;
				}
				case VK_RIGHT : {
					printf("Keyboard Event: Key RIGHT was released.\n");
					keyboard.key_right = false;
					break;
				}
				case VK_UP : {
					printf("Keyboard Event: Key UP was released.\n");
					keyboard.key_up = false;
					break;
				}
				case VK_DOWN : {
					printf("Keyboard Event: Key DOWN was released.\n");
					keyboard.key_down = false;
					break;
				}
				case VK_SHIFT : {
					printf("Keyboard Event: Key SHIFT was released.\n");
					break;
				}
				case VK_CONTROL : {
					printf("Keyboard Event: Key CONTROL was released.\n");
					break;
				}
				case VK_SPACE : {
					printf("Keyboard Event: Key SPACE was released.\n");
					break;
				}
				case VK_ESCAPE : {
					printf("Keyboard Event: Key ESCAPE was released.\n");
					SendMessage(window_handle, WM_QUIT, NULL, NULL);
					break;
				}
				default : {
					printf("Keyboard Event: Unknown key was released.\n");
				}
			}
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
				ShowWindow(handle, 1);
				UpdateWindow(handle);

				}
			}
	}
	return false;
}

void init_d3d() {
	HRESULT error_code;

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

	error_code = D3D11CreateDeviceAndSwapChain(	NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, 
												&swap_chain_desc, &swap_chain, &d3d_device, NULL, &d3d_dc);

	ID3D11Texture2D * backbuffer;
	error_code = swap_chain->GetBuffer(0, __uuidof(backbuffer), (void**) &backbuffer);

	error_code = d3d_device->CreateRenderTargetView(backbuffer, NULL, &render_target_view);
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

	error_code = D3DCompileFromFile(L"shader.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
									"VS", "vs_5_0", 0, 0, &VS_bytecode, NULL);
	error_code = D3DCompileFromFile(L"shader.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
									"PS", "ps_5_0", 0, 0, &PS_bytecode, NULL);

	error_code = d3d_device->CreateVertexShader(VS_bytecode->GetBufferPointer(), VS_bytecode->GetBufferSize(), 
												NULL, &VS);
	error_code = d3d_device->CreatePixelShader(	PS_bytecode->GetBufferPointer(), PS_bytecode->GetBufferSize(), 
												NULL, &PS);
	d3d_dc->VSSetShader(VS, NULL, 0);
	d3d_dc->PSSetShader(PS, NULL, 0);

	// Vectex Buffer
	D3D11_BUFFER_DESC 	vertex_buffer_desc = {};
						vertex_buffer_desc.Usage 			= D3D11_USAGE_DYNAMIC;
						vertex_buffer_desc.ByteWidth 		= sizeof(Vertex) * MAX_VERTEX_BUFFER_SIZE;
						vertex_buffer_desc.BindFlags 		= D3D11_BIND_VERTEX_BUFFER;
						vertex_buffer_desc.CPUAccessFlags 	= D3D11_CPU_ACCESS_WRITE;
						vertex_buffer_desc.MiscFlags 		= 0;

	error_code = d3d_device->CreateBuffer(&vertex_buffer_desc, NULL, &d3d_vertex_buffer_interface);

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

	error_code = d3d_device->CreateBuffer(&index_buffer_desc, NULL, &d3d_index_buffer_interface);

	d3d_dc->IASetIndexBuffer(d3d_index_buffer_interface, DXGI_FORMAT_R32_UINT, 0);

	// Input Layout
	d3d_device->CreateInputLayout(layout_desc, ARRAYSIZE(layout_desc), VS_bytecode->GetBufferPointer(), 
								  VS_bytecode->GetBufferSize(), &vertex_layout);

	d3d_dc->IASetInputLayout(vertex_layout);

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

}

void draw_buffer(int buffer_index) {

	VertexBuffer * vb = &graphics_buffer.vertex_buffers[buffer_index];
	IndexBuffer * ib = &graphics_buffer.index_buffers[buffer_index];
	std::vector<char *> texture_ids = graphics_buffer.texture_ids[buffer_index];

	// Update vertex buffer
	D3D11_MAPPED_SUBRESOURCE resource = {};
	d3d_dc->Map(d3d_vertex_buffer_interface, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, &vb->vertices[0], sizeof(Vertex) * vb->vertices.size());
	d3d_dc->Unmap(d3d_vertex_buffer_interface, 0);

	// Update index buffer
	resource = {};
	d3d_dc->Map(d3d_index_buffer_interface, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, &ib->indices[0],
		   sizeof( int ) *  ib->indices.size());
	d3d_dc->Unmap(d3d_index_buffer_interface, 0);

	// Bind Textures
	std::vector<ID3D11ShaderResourceView *> texture_buffer;
	for(int i = 0; i<texture_ids.size(); i++) {
		// Find texture
		int texture_index = -1;
		for(int j = 0; j<textures.size(); j++) {
			if(strcmp(texture_ids[i], textures[j]->name) == 0) {
				texture_index = j;
				break;
			}
		}
		// @TODO Check for duplicates
		if(texture_index == -1) {
			log_print("draw_buffer", "Unable to find texture %s", texture_ids[i]);
		} else {
			texture_buffer.push_back(textures[texture_index]->srv);
		}
	}

	d3d_dc->PSSetShaderResources(0, 1, &texture_buffer[0]);
	d3d_dc->PSSetSamplers(0, 1, &default_sampler_state);

	d3d_dc->DrawIndexed(graphics_buffer.index_buffers[buffer_index].indices.size(), 0, 0);
}

void draw_frame() {
	// Clear view
	float color_array[4] = {window_data.background_color.r, window_data.background_color.g, 
							window_data.background_color.b, window_data.background_color.a};

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
	graphics_buffer.texture_ids.clear();
}

void create_texture(char name[]) {
	// SRV
	D3D11_SUBRESOURCE_DATA texture_data = {};

	char path[256];
	snprintf(path, 256, "textures/%s", name);
	
	int x,y,n;
	texture_data.pSysMem 			= stbi_load(path, &x, &y, &n, 4);

	if(texture_data.pSysMem == NULL) {
		log_print("create_texture", "Failed to load texture %s", name);
		return;
	}

	if(n != 4) {
		log_print("create_texture", "Loaded texture %s, but it has %d bit depth, we prefer using 32 bit depth textures", name, n*8);
		n = 4;
	}

    texture_data.SysMemPitch 		= x*n;
    texture_data.SysMemSlicePitch 	= x*y*n;

	D3D11_TEXTURE2D_DESC texture_desc;
						 texture_desc.Width 				= x;
						 texture_desc.Height 				= y;
						 texture_desc.MipLevels 			= 1;
						 texture_desc.ArraySize 			= 1;
						 texture_desc.Format 				= DXGI_FORMAT_R8G8B8A8_UNORM;
						 texture_desc.SampleDesc.Count 		= 1;
						 texture_desc.SampleDesc.Quality 	= 0;
						 texture_desc.Usage 				= D3D11_USAGE_DEFAULT;
						 texture_desc.BindFlags 			= D3D11_BIND_SHADER_RESOURCE;
						 texture_desc.CPUAccessFlags 		= 0;
						 texture_desc.MiscFlags 			= 0;

	ID3D11Texture2D * d3d_texture;
    d3d_device->CreateTexture2D(&texture_desc, &texture_data, &d3d_texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    								srv_desc.Format 					= DXGI_FORMAT_R8G8B8A8_UNORM;
    								srv_desc.ViewDimension 				= D3D11_SRV_DIMENSION_TEXTURE2D;
									srv_desc.Texture2D.MostDetailedMip 	= 0;
    								srv_desc.Texture2D.MipLevels 		= 1;

    ID3D11ShaderResourceView * srv;
    d3d_device->CreateShaderResourceView(d3d_texture, &srv_desc, &srv);
    d3d_texture->Release();

	Texture * texture = new Texture();
			  texture->width 			= x;
			  texture->height 			= y;
			  texture->bytes_per_pixel 	= n;
			  texture->srv 				= srv;
			  texture->sampler_state 	= default_sampler_state;

	texture->name = name;

	textures.push_back(texture);
}

void init_textures() {
	create_texture("title_screen_logo.png");
	create_texture("grass.png");
}

void main() {
	window_data.width 	= 1600;
	window_data.height 	= 900;

	char * window_name = "Game3";

	create_window(window_data.width, window_data.height, window_name);
	init_d3d();

	init_textures();

	RECT rc;
	float frame_time = 0.0f;
	LARGE_INTEGER start_time, end_time;
	LARGE_INTEGER frequency;

	while(!should_quit) {
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&start_time);

		update_window_events();
		game(&window_data, &keyboard, &graphics_buffer);
		draw_frame();

		QueryPerformanceCounter(&end_time);
		frame_time = ((float)(end_time.QuadPart - start_time.QuadPart)*1000/(float)frequency.QuadPart);
	}
}