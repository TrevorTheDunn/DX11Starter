#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "SimpleShader.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

//For Constant Buffer(?)
#include "BufferStructs.h"

#include "Entity.h"

#include "Material.h"

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	//ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();

	materials.push_back(std::make_shared<Material>(XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), vertexShader, pixelShader));
	materials.push_back(std::make_shared<Material>(XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), vertexShader, pixelShader));
	materials.push_back(std::make_shared<Material>(XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), vertexShader, pixelShader));

	CreateGeometry();

	//Create the cameras and sets the active one to the first camera
	cameras.push_back(std::make_shared<Camera>(
		0.0f, 0.0f, -5.0f,
		5.0f,
		1.0f,
		XM_PIDIV4,
		(float)this->windowWidth / this->windowHeight));
	cameras.push_back(std::make_shared<Camera>(
		-5.0f, 5.0f, -10.0f,
		5.0f,
		1.0f,
		XM_PIDIV2,
		(float)this->windowWidth / this->windowHeight));
	cameras.push_back(std::make_shared<Camera>(
		5.0f, -5.0f, -7.5f,
		5.0f,
		1.0f,
		XM_PIDIV4/2.0f,
		(float)this->windowWidth / this->windowHeight));
	activeCamera = cameras[0];
	
	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		//context->IASetInputLayout(inputLayout.Get());

		// Set the active vertex and pixel shaders
		//  - Once you start applying different shaders to different objects,
		//    these calls will need to happen multiple times per frame
		//context->VSSetShader(vertexShader.Get(), 0, 0);
		//context->PSSetShader(pixelShader.Get(), 0, 0);
	}

	//Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	//Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	//Get size as the next multiple of 16 (rather than hardcoding a size)
	unsigned int size = sizeof(VertexShaderExternalData);
	size = (size + 15) / 16 * 16; //Will work even if the struct size changes

	//Describe the constant buffer
	//D3D11_BUFFER_DESC cbDesc = {}; //Sets struct to all zeroes
	//cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//cbDesc.ByteWidth = size; //Must be multiple of 16
	//cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//cbDesc.Usage = D3D11_USAGE_DYNAMIC;

	//Creates the buffer
	//device->CreateBuffer(&cbDesc, 0, vsConstantBuffer.GetAddressOf());

	offset = XMFLOAT3(0.0f, 0.0f, 0.0f);
	tint = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	/*
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}

	// Create an input layout 
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}*/

	vertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader.cso").c_str());
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red	= XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green	= XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue	= XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMFLOAT4 black = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 white = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// Set up the vertices of the triangle we would like to draw
	// - We're going to copy this array, exactly as it exists in CPU memory
	//    over to a Direct3D-controlled data structure on the GPU (the vertex buffer)
	// - Note: Since we don't have a camera or really any concept of
	//    a "3d world" yet, we're simply describing positions within the
	//    bounds of how the rasterizer sees our screen: [-1 to +1] on X and Y
	// - This means (0,0) is at the very center of the screen.
	// - These are known as "Normalized Device Coordinates" or "Homogeneous 
	//    Screen Coords", which are ways to describe a position without
	//    knowing the exact size (in pixels) of the image/window/etc.  
	// - Long story short: Resizing the window also resizes the triangle,
	//    since we're describing the triangle in terms of the window itself
	Vertex vertices[] =
	{
		{ XMFLOAT3(+0.0f, +0.5f, +0.0f), red },
		{ XMFLOAT3(+0.5f, -0.5f, +0.0f), blue },
		{ XMFLOAT3(-0.5f, -0.5f, +0.0f), green },
	};

	// Set up indices, which tell us which vertices to use and in which order
	// - This is redundant for just 3 vertices, but will be more useful later
	// - Indices are technically not required if the vertices are in the buffer 
	//    in the correct order and each one will be used exactly once
	// - But just to see how it's done...
	unsigned int indices[] = { 0, 1, 2 };

	{
		mesh1 = std::make_shared<Mesh>(vertices, ARRAYSIZE(vertices), indices, 3, device, context);

		Vertex vertices2[] =
		{
			{ XMFLOAT3(-0.75f, +0.75f, +0.0f), red },
			{ XMFLOAT3(-0.65f, +0.55f, +0.0f), blue},
			{ XMFLOAT3(-0.65f, +0.25f, +0.0f), green},
			{ XMFLOAT3(-0.85f, +0.25f, +0.0f), white},
			{ XMFLOAT3(-0.85f, +0.55f, +0.0f), black},
			{ XMFLOAT3(-0.75f, +0.5f, +0.0f), white},
		};

		unsigned int indices2[] = {0, 1, 5, 1, 2, 5, 2, 3, 5, 3, 4, 5, 4, 0, 5};

		mesh2 = std::make_shared<Mesh>(vertices2, ARRAYSIZE(vertices2), indices2, ARRAYSIZE(indices2), device, context);

		Vertex vertices3[] =
		{
			{XMFLOAT3(+0.7f, -0.5f, +0.0f), red},
			{XMFLOAT3(+0.9f, -0.5f, +0.0f), black},
			{XMFLOAT3(+0.9f, -0.8f, +0.0f), black},
			{XMFLOAT3(+0.7f, -0.8f, +0.0f), red},
		};

		unsigned int indices3[] = {0, 1, 2, 2, 3, 0};

		mesh3 = std::make_shared<Mesh>(vertices3, ARRAYSIZE(vertices3), indices3, ARRAYSIZE(indices3), device, context);

		meshes.push_back(mesh1);
		meshes.push_back(mesh2);
		meshes.push_back(mesh3);

		Entity e1 = Entity(mesh1, materials[0]);
		Entity e2 = Entity(mesh1, materials[1]);
		Entity e3 = Entity(mesh1, materials[2]);
		Entity e4 = Entity(mesh2, materials[0]);
		Entity e5 = Entity(mesh3, materials[1]);

		entities.push_back(e1);
		entities.push_back(e2);
		entities.push_back(e3);
		entities.push_back(e4);
		entities.push_back(e5);
	}
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	activeCamera->UpdateProjectionMatrix(XM_PIDIV4,
		(float)this->windowWidth / this->windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	//Feed fresh input data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;

	//Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//Determine new input capture
	Input& input = Input::GetInstance();
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);

	//Show the demo window
	//ImGui::ShowDemoWindow();

	int value = 0;

	std::vector<XMFLOAT3> ePos;
	std::vector<XMFLOAT3> eRot;
	std::vector<XMFLOAT3> eSca;

	for (int i = 0; i < entities.size(); i++)
	{
		ePos.push_back(entities[i].GetTransform()->GetPosition());
		eRot.push_back(entities[i].GetTransform()->GetPitchYawRoll());
		eSca.push_back(entities[i].GetTransform()->GetScale());
	}

	//Creation of custom ImGui Window
	{
		ImGui::SetNextWindowSize(ImVec2(500, 400));

		//Header Text for the Window
		ImGui::Begin("Assignment 5 Window");

		//Dropdown menu for app details
		if (ImGui::TreeNode("App Details"))
		{
			//Tracker for the framerate
			ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

			//Tracker for the Window Dimensions
			ImGui::Text("Window Dimensions: %i x %i", this->windowWidth, this->windowHeight);

			ImGui::TreePop();
		}

		//TASK 7 - OFFSET AND COLOR CHANGE
		//You'll probably need variables for these pieces of data, as they need to persist across frames

		if (ImGui::TreeNode("Meshes"))
		{
			//OFFSET - To edit a 3 component vector with ImGui, use the DragFloat3() function
			ImGui::DragFloat3("Offset", &offset.x);

			//COLOR - To edit a 4-component color with ImGui, use the ColorEdit4() function
			ImGui::ColorEdit4("4 component RGBA Color Editor", &tint.x);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Scene Entities"))
		{
			if (ImGui::TreeNode("Entity 1"))
			{
				ImGui::DragFloat3("Position##1", &ePos[0].x);
				ImGui::DragFloat3("Rotation (Radians)##1", &eRot[0].x);
				ImGui::DragFloat3("Scale##1", &eSca[0].x);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 2"))
			{
				ImGui::DragFloat3("Position##2", &ePos[1].x);
				ImGui::DragFloat3("Rotation (Radians)##2", &eRot[1].x);
				ImGui::DragFloat3("Scale##2", &eSca[1].x);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 3"))
			{
				ImGui::DragFloat3("Position##3", &ePos[2].x);
				ImGui::DragFloat3("Rotation (Radians)##3", &eRot[2].x);
				ImGui::DragFloat3("Scale##3", &eSca[2].x);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 4"))
			{
				ImGui::DragFloat3("Position##4", &ePos[3].x);
				ImGui::DragFloat3("Rotation (Radians)##4", &eRot[3].x);
				ImGui::DragFloat3("Scale##4", &eSca[3].x);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 5"))
			{
				ImGui::DragFloat3("Position##5", &ePos[4].x);
				ImGui::DragFloat3("Rotation (Radians)##5", &eRot[4].x);
				ImGui::DragFloat3("Scale##5", &eSca[4].x);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Cameras"))
		{
			if (ImGui::TreeNode("Camera 1"))
			{
				XMFLOAT3 c1Pos = cameras[0]->GetTransform()->GetPosition();
				ImGui::Text("Camera 1 Position: X - %f Y - %f Z - %f", c1Pos.x, c1Pos.y, c1Pos.z);
				ImGui::Text("Camera 1 FOV: %f", XM_PIDIV2);
				if (ImGui::Button("Set As Current Camera"))
				{
					if (activeCamera == cameras[0]) return;
					activeCamera = cameras[0];
				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Camera 2"))
			{
				XMFLOAT3 c2Pos = cameras[1]->GetTransform()->GetPosition();
				ImGui::Text("Camera 2 Position: X - %f Y - %f Z - %f", c2Pos.x, c2Pos.y, c2Pos.z);
				ImGui::Text("Camera 2 FOV: %f", XM_PIDIV4);
				if (ImGui::Button("Set As Current Camera"))
				{
					if (activeCamera == cameras[1]) return;
					activeCamera = cameras[1];
				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Camera 3"))
			{
				XMFLOAT3 c3Pos = cameras[2]->GetTransform()->GetPosition();
				ImGui::Text("Camera 3 Position: X - %f Y - %f Z - %f", c3Pos.x, c3Pos.y, c3Pos.z);
				ImGui::Text("Camera 3 FOV: %f", XM_PIDIV4/2.0f);
				if (ImGui::Button("Set As Current Camera"))
				{
					if (activeCamera == cameras[2]) return;
					activeCamera = cameras[2];
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		ImGui::End();

		for (int i = 0; i < entities.size(); i++)
		{
			entities[i].GetTransform()->SetPosition(ePos[i]);
			entities[i].GetTransform()->SetRotation(eRot[i]);
			entities[i].GetTransform()->SetScale(eSca[i]);
		}
	}

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	activeCamera->Update(deltaTime);

	entities[4].GetTransform()->MoveAbsolute(-0.1f * deltaTime, 0.0f * deltaTime, 0.0f * deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	//Creates a local variable of the struct and fills out its variables
	//VertexShaderExternalData vsData;
	//vsData.colorTint = tint;

	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	//Draws each of the entities
	for (auto& e : entities)
	{
		e.Draw(context, activeCamera);
	}

	//ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}