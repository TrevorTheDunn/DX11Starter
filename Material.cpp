#include "Material.h"
#include "Transform.h"
#include "Camera.h"

Material::Material(DirectX::XMFLOAT3 colorTint, 
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

DirectX::XMFLOAT3 Material::GetColorTint() { return colorTint; }

std::shared_ptr<SimpleVertexShader> Material::GetVertexShader() { return vertexShader; }

std::shared_ptr<SimplePixelShader> Material::GetPixelShader() { return pixelShader; }

float Material::GetRoughness() { return roughness; }

void Material::SetColorTint(DirectX::XMFLOAT3 colorTint) { this->colorTint = colorTint; }

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader) { this->vertexShader = vertexShader; }

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader) { this->pixelShader = pixelShader; }

void Material::SetRoughness(float roughness) { this->roughness = roughness; }

void Material::AddTextureSRV(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { textureSRVs.insert({ shaderName, srv }); }

void Material::AddSampler(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler) { samplers.insert({ shaderName, sampler }); }

void Material::SetResources(std::shared_ptr<Transform> transform, std::shared_ptr<Camera> camera)
{
    pixelShader->SetShader();
    vertexShader->SetShader();

    vertexShader->SetMatrix4x4("world", transform->GetWorldMatrix());
    vertexShader->SetMatrix4x4("view", camera->GetView());
    vertexShader->SetMatrix4x4("proj", camera->GetProjection());
    vertexShader->SetMatrix4x4("worldInvTranspose", transform->GetWorldInverseTransposeMatrix());
    vertexShader->CopyAllBufferData();

    pixelShader->SetFloat3("colorTint", colorTint);
    pixelShader->SetFloat("roughness", roughness);
    pixelShader->SetFloat3("cameraPos", camera->GetTransform()->GetPosition());
    pixelShader->CopyAllBufferData();

    for (auto& t : textureSRVs) { pixelShader->SetShaderResourceView(t.first.c_str(), t.second.Get()); }
    for (auto& s : samplers) { pixelShader->SetSamplerState(s.first.c_str(), s.second.Get()); }
}
