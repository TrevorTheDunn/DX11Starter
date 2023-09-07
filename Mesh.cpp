#include "Mesh.h"

Mesh::Mesh(Vertex vertices[], int numVertices, unsigned int indices[], int numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> devContext)
{
    this->context = devContext;
    this->meshBufferIndices = numIndices;

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * 3;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    vbd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initialVertexData = {};
    initialVertexData.pSysMem = vertices;

    device->CreateBuffer(&vbd, &initialVertexData, vertexBuffer.GetAddressOf());

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(unsigned int) * 3;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    ibd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initialIndexData = {};
    initialIndexData.pSysMem = indices;

    device->CreateBuffer(&ibd, &initialIndexData, indexBuffer.GetAddressOf());
}

Mesh::~Mesh()
{

}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer()
{
    return vertexBuffer;
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer()
{
    return indexBuffer;
}

int Mesh::GetIndexCount()
{
    return meshBufferIndices;
}

void Mesh::Draw()
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, this->GetVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(this->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

    context->DrawIndexed(this->GetIndexCount(), 0, 0);
}
