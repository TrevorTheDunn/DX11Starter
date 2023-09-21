#pragma once
#include "Transform.h"
#include "Mesh.h"
#include <memory>
#include "BufferStructs.h"

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> mesh);

	~GameEntity();

	std::shared_ptr<Mesh> GetMesh();

	std::shared_ptr<Transform> GetTransform();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer, VertexShaderExternalData vsData);

private:
	std::shared_ptr<Transform> transformObject;
	std::shared_ptr<Mesh> meshObject;
};