/*
	3Dキューブの表示（立方体）：cube.cpp

	2025/09/09	hibiki sakuma
*/

#include "cube.h"

#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "shader3d.h"
#include "shader_depth.h"
#include "debug_ostream.h"
#include "texture.h"
#include "resource_manager.h"


static constexpr int NUM_INDEX = 36;
static constexpr int NUM_VERTEX = 24; // 頂点数 1面に三角形2枚　1面：6頂点　6面：36頂点 - 重複頂点：８

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr; // インデックスバッファ
//static ID3D11ShaderResourceView* g_pTexture = nullptr; // テクスチャ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 頂点構造体
struct Vertex3d
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT3 normal;   // 法線
	XMFLOAT4 color;    // 色　
	XMFLOAT2 texcoord; // UV
};

static Vertex3d g_CubeVertex[24]{
	// 右　
	{{ 0.5f,  0.5f, -0.5f},{ 1.0f, 0.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 0.0f, 0.0f}},
	{{ 0.5f, -0.5f,  0.5f},{ 1.0f, 0.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.25f,0.25f}},
	{{ 0.5f, -0.5f, -0.5f},{ 1.0f, 0.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 0.0f,0.25f}},
	{{ 0.5f,  0.5f,  0.5f},{ 1.0f, 0.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.25f, 0.0f}},
	// 左　
	{{-0.5f, -0.5f,  0.5f},{-1.0f, 0.0f, 0.0f},{ 1.0f, 1.0f, 1.0f, 1.0f},{0.25f, 0.0f} },
	{{-0.5f,  0.5f, -0.5f},{-1.0f, 0.0f, 0.0f},{ 1.0f, 1.0f, 1.0f, 1.0f},{ 0.5f,0.25f }},
	{{-0.5f, -0.5f, -0.5f},{-1.0f, 0.0f, 0.0f},{ 1.0f, 1.0f, 1.0f, 1.0f},{0.25f,0.25f }},
	{{-0.5f,  0.5f,  0.5f},{-1.0f, 0.0f, 0.0f},{ 1.0f, 1.0f, 1.0f, 1.0f},{ 0.5f, 0.0f }},

	// 前　
	{{-0.5f,  0.5f, -0.5f},{ 0.0f, 0.0f, 1.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 0.5f, 0.0f}},
	{{ 0.5f, -0.5f, -0.5f},{ 0.0f, 0.0f, 1.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.75f,0.25f}},
	{{-0.5f, -0.5f, -0.5f},{ 0.0f, 0.0f, 1.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 0.5f,0.25f}},
	{{ 0.5f,  0.5f, -0.5f},{ 0.0f, 0.0f, 1.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.75f, 0.0f}},
	// 後　
	{{ 0.5f,  0.5f,  0.5f},{ 0.0f, 0.0f,-1.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.75f, 0.0f }},
	{{-0.5f, -0.5f,  0.5f},{ 0.0f, 0.0f,-1.0f},{1.0f, 1.0f, 1.0f, 1.0f},{1.0f ,0.25f }},
	{{ 0.5f, -0.5f,  0.5f},{ 0.0f, 0.0f,-1.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.75f,0.25f }},
	{{-0.5f,  0.5f,  0.5f},{ 0.0f, 0.0f,-1.0f},{1.0f, 1.0f, 1.0f, 1.0f},{1.0f, 0.0f }},

	// 下　
	{{-0.5f, -0.5f,  0.5f},{ 0.0f,-1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.0f, 0.25f}},
	{{-0.5f, -0.5f, -0.5f},{ 0.0f,-1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.0f,0.5f }},
	{{ 0.5f, -0.5f, -0.5f},{ 0.0f,-1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.25f,0.5f }},
	{{ 0.5f, -0.5f,  0.5f},{ 0.0f,-1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.25f, 0.25}},
	// 上　
	{{-0.5f,  0.5f,  0.5f},{ 0.0f, 1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.25f,0.25f}},
	{{ 0.5f,  0.5f, -0.5f},{ 0.0f, 1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 0.5f,0.5f }},
	{{-0.5f,  0.5f, -0.5f},{ 0.0f, 1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{0.25f,0.5f }},
	{{ 0.5f,  0.5f,  0.5f},{ 0.0f, 1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 0.5f,0.25f}},

};

static Vertex3d g_DebagCubeVertex[24]{};


static unsigned short g_CubeIndex[36]{ // unsigned short 0-65535 最大頂点数 int　の方がハイポリだが容量多
	 0, 1, 2, 0, 3, 1, // 1
	 4, 5, 6, 4, 7, 5, // 2
	 8, 9,10, 8,11, 9, // 3
	12,13,14,12,15,13, // 4
	16,17,18,16,18,19, // 5 **
	20,21,22,20,23,21  // 6
};

void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext){
	
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT; // Usage:使い方　DYNAMIC:書き換えて使う
	bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX; // sizeof(g_CubeVertex)配列なのでこれでもおｋ
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0; // 1度作成したら動かさない 読み書き不可

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_CubeVertex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	// インデックスバッファ作成
	bd.Usage = D3D11_USAGE_DEFAULT; // Usage:使い方　DYNAMIC:書き換えて使う
	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0; // 1度作成したら動かさない 読み書き不可

	sd.pSysMem = g_CubeIndex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);

}

void Cube_Finalize(void){
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

void Cube_Update(double elapsed_time){
}

void Cube_Draw(int texId, const DirectX::XMMATRIX& mtmWorld){
	// シェーダーを描画パイプラインに設定
	Shader3d_Begin();

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	Shader3d_SetColor({ 1.0f,1.0f,1.0f,1.0f });

	Texture_SetTexture(Resouce_Manager_GetTexId(Cube));

	// インデックスバッファを描画パイプラインに設定
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT,0); // unsined short = R16 unsined int = R32

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	// 頂点シェーダにワールド座標変換行列に設定
	Shader3d_SetWorldMatrix(mtmWorld);

	// ポリゴン描画命令発行
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

void Cube_DepthDraw(int texId, const DirectX::XMMATRIX& mtmWorld){
	// シェーダーを描画パイプラインに設定
	ShaderDepth_Begin();
	
	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// インデックスバッファを描画パイプラインに設定
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0); // unsined short = R16 unsined int = R32

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点シェーダにワールド座標変換行列に設定
	ShaderDepth_SetWorldMatrix(mtmWorld);
	// ポリゴン描画命令発行
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

AABB Cube_GetAABB(const DirectX::XMFLOAT3& position){
	return {
		{position.x - 0.5f, position.y - 0.5f, position.z - 0.5f},
		{position.x + 0.5f, position.y + 0.5f, position.z + 0.5f}
	};
}
