#include "Material.h"

Material::Material(DirectX::XMFLOAT4 colorTint, 
    std::shared_ptr<SimpleVertexShader> vertexShader, 
    std::shared_ptr<SimplePixelShader> pixelShader,
    float roughness) :
    colorTint(colorTint),
    vertexShader(vertexShader),
    pixelShader(pixelShader),
    roughness(roughness)
{
}

Material::~Material() { }

DirectX::XMFLOAT4 Material::GetColorTint() { return colorTint; }

std::shared_ptr<SimpleVertexShader> Material::GetVertexShader() { return vertexShader; }

std::shared_ptr<SimplePixelShader> Material::GetPixelShader() { return pixelShader; }

float Material::GetRoughness() { return roughness; }

void Material::SetColorTint(DirectX::XMFLOAT4 colorTint) { this->colorTint = colorTint; }

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader) { this->vertexShader = vertexShader; }

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader) { this->pixelShader = pixelShader; }

void Material::SetRoughness(float roughness) { this->roughness = roughness; }
