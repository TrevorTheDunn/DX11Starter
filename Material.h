#pragma once

#include <DirectXMath.h>
#include "SimpleShader.h"
#include <memory>

class Material
{
public:
	//Constructor
	Material(DirectX::XMFLOAT4 colorTint,
		std::shared_ptr<SimpleVertexShader> vertexShader,
		std::shared_ptr<SimplePixelShader> pixelShader,
		float roughness);

	//Deconstructor
	~Material();

	//Getters
	DirectX::XMFLOAT4 GetColorTint();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	float GetRoughness();

	//Setters
	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);
	void SetRoughness(float roughness);

private:

	DirectX::XMFLOAT4 colorTint;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
	float roughness;
};