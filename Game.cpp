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
	vertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader.cso").c_str());
	customPixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"CustomPS.cso").c_str());
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

	// Imports all of the different 3D meshes
	std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device);
	std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device);
	std::shared_ptr<Mesh> helixMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str(), device);
	std::shared_ptr<Mesh> cylinderMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str(), device);
	std::shared_ptr<Mesh> torusMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str(), device);
	std::shared_ptr<Mesh> quadMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad.obj").c_str(), device);
	std::shared_ptr<Mesh> quadDSMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str(), device);

	// Creates the different materials and adds them to a vector of materials 
	materials.push_back(std::make_shared<Material>(red, vertexShader, pixelShader));
	materials.push_back(std::make_shared<Material>(green, vertexShader, pixelShader));
	materials.push_back(std::make_shared<Material>(blue, vertexShader, pixelShader));
	materials.push_back(std::make_shared<Material>(black, vertexShader, pixelShader));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader));

	std::shared_ptr<Material> customPSMaterial = std::make_shared<Material>(white, vertexShader, customPixelShader);

	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[0]));
	entities.push_back(std::make_shared<Entity>(cubeMesh, materials[1]));
	entities.push_back(std::make_shared<Entity>(helixMesh, materials[2]));
	entities.push_back(std::make_shared<Entity>(cylinderMesh, customPSMaterial));
	entities.push_back(std::make_shared<Entity>(torusMesh, materials[4]));
	entities.push_back(std::make_shared<Entity>(quadMesh, materials[3]));
	entities.push_back(std::make_shared<Entity>(quadDSMesh, materials[1]));

	float x = -9.0f;

	for (int i = 0; i < entities.size(); i++)
	{
		entities[i]->GetTransform()->MoveAbsolute(x, 0, 0);
		x += 3.0f;
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
		ePos.push_back(entities[i]->GetTransform()->GetPosition());
		eRot.push_back(entities[i]->GetTransform()->GetPitchYawRoll());
		eSca.push_back(entities[i]->GetTransform()->GetScale());
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
			entities[i]->GetTransform()->SetPosition(ePos[i]);
			entities[i]->GetTransform()->SetRotation(eRot[i]);
			entities[i]->GetTransform()->SetScale(eSca[i]);
		}
	}

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	activeCamera->Update(deltaTime);
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
		e->Draw(context, activeCamera, deltaTime, XMFLOAT2((float)this->windowWidth, (float)this->windowHeight));
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