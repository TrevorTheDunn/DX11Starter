#pragma once

#include <d3d11.h> //Used for Direct3D "stuff"
#include <wrl/client.h> //Used for ComPtr
#include "Vertex.h" //Used for custom Vertex struct

class Mesh
{
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer; //vertex buffer of this mesh
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer; //index buffer of this mesh
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context; //used for issuing draw commands
	int meshBufferIndices; //specifies how many indices are in the mesh's index buffer, used when drawing

public:
	//A constructor that creates the two buffers from the appropriate arrays.
	//You should copy, paste, and adjust the code from the CreateBasicGeometry()
	//method as necessary
	Mesh(Vertex vertices[], int numVertices, unsigned int indices[], int numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> devContext);

	//Since we're using smart pointers, your destructor won't have much to do (it'll be empty)
	//Properly cleaning up Direct3D objects is your responsibility
	//Smart pointers will do this for you if you're using them correctly
	~Mesh();

	//method to return the pointer to the vertex buffer object
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();

	//method to return the pointer for the index buffer object
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();

	//returns the number of indices the mesh contains
	int GetIndexCount(); 

	//sets the buffers and tells DirectX to draw the correct number of indices
	void Draw(); 
};

