#pragma once

#include <DirectXMath.h>
#include "SimpleShader.h"
#include <memory>

#include <unordered_map>

#include "Transform.h"
#include "Camera.h"

class Material
{
public:
	//Constructor
	Material(DirectX::XMFLOAT3 colorTint,
		std::shared_ptr<SimpleVertexShader> vertexShader,
		std::shared_ptr<SimplePixelShader> pixelShader,
		float roughness);

	//Deconstructor
	~Material();

	//Getters
	DirectX::XMFLOAT3 GetColorTint();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	float GetRoughness();

	//Setters
	void SetColorTint(DirectX::XMFLOAT3 colorTint);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);
	void SetRoughness(float roughness);

	void AddTextureSRV(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	void AddSampler(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);

	void SetResources(std::shared_ptr<Transform>transform, std::shared_ptr<Camera> camera);

private:

	DirectX::XMFLOAT3 colorTint;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
	float roughness;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;
};