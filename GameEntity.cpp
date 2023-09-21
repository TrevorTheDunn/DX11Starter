#include "GameEntity.h"
#include "BufferStructs.h"

using namespace std;

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh)
{
    meshObject = mesh;
}

GameEntity::~GameEntity()
{
}

std::shared_ptr<Mesh> GameEntity::GetMesh() {return meshObject;}

std::shared_ptr<Transform> GameEntity::GetTransform() {return transformObject;}

void GameEntity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer,
    VertexShaderExternalData vsData)
{
    //Set the correct Constant Buffer resource for the Vertex Shader stage

    //Collects data for the current entity in a C++ struct
    VertexShaderExternalData vData;
    //vData.colorTint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vData.worldMatrix = this->GetTransform()->GetWorldMatrix();

    //Map / memcpy / unmap constant buffer resource
    D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
    context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);

    memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));

    context->Unmap(constantBuffer.Get(), 0);

    context->VSGetConstantBuffers(
        0,
        1,
        constantBuffer.GetAddressOf());

    //Sets the correct vertex and index buffers
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, meshObject->GetVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(meshObject->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

    //Tell D3D to render the currently bound resources
    context->DrawIndexed(meshObject->GetIndexCount(), 0, 0);
}
