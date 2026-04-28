/*
	メッシュフィールドの表示：meshfiled.cpp

	2025/09/09	hibiki sakuma
*/

#include "meshfield.h"
#include <random>
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "shaderfield.h"
#include "debug_ostream.h"
#include "texture.h"
#include "resource_manager.h"
#include "camera.h"

// 定数宣言
static constexpr int MASS_SIZE = 1;
static constexpr int MASS_V_COUNT = 100; // 縦Yマス
static constexpr int MASS_H_COUNT = 100; // 横Xマス

static constexpr int MESH_V_SIZE = MASS_SIZE * MASS_V_COUNT;
static constexpr int MESH_H_SIZE = MASS_SIZE * MASS_H_COUNT;

static constexpr int MASS_H_POINT = MASS_H_COUNT + 1; // 横の頂点数 
static constexpr int MASS_V_POINT = MASS_V_COUNT + 1; // 縦の頂点数

static constexpr int NUM_VERTEX = MASS_H_POINT * MASS_V_POINT; // 全体の頂点数 = 横の頂点数 × 縦の頂点数
static constexpr int NUM_INDEX = NUM_VERTEX * 6;

static constexpr int INTERVAL = 15; // 10マス間隔
static constexpr int ROAD_HALF_WIDTH = 1; // 道の太さ（中心から1マスずつ＝合計2〜3マス幅）

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr; // インデックスバッファ


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

static Vertex3d g_MeshVertex[NUM_VERTEX]{}; // 計算結果を入れる

static unsigned short g_MeshIndex[NUM_INDEX]{};

void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext){	
	// 頂点座標の算出
	// マップの中心座標を計算（頂点数ベース）
	int centerX = MASS_H_POINT / 2;
	int centerZ = MASS_V_POINT / 2;

	for (int z = 0; z < MASS_V_POINT; z++) {
		for (int x = 0; x < MASS_H_POINT; x++) {
			int index = x + MASS_H_POINT * z;

			// 頂点の基本設定
			g_MeshVertex[index].position = { (float)(x * MASS_SIZE), 0.0f, (float)(z * MASS_SIZE) };
			g_MeshVertex[index].normal = { 0.0f, 1.0f, 0.0f };
			g_MeshVertex[index].texcoord = { (float)x * 1.0f, (float)z * 1.0f };

			// --- 格子状の道ロジック ---
			// 中心からの距離を計算
			int dx = abs(x - centerX);
			int dz = abs(z - centerZ);

			// 10で割った余りが「0」なら、そこは10マスごとのライン上
			// ROAD_HALF_WIDTH以下であれば道として判定する
			bool isRoadX = (dx % INTERVAL) <= ROAD_HALF_WIDTH;
			bool isRoadZ = (dz % INTERVAL) <= ROAD_HALF_WIDTH;

			float blendR = (isRoadX || isRoadZ) ? 1.0f : 0.0f;

			// 色を設定 (R:道、G:土)
			g_MeshVertex[index].color = { blendR, 1.0f - blendR, 0.0f, 1.0f };
		}
	}

	int i = 0;
	for (int z = 0; z < MASS_V_COUNT; z++) {
		for (int x = 0; x < MASS_H_COUNT; x++) {
			g_MeshIndex[i + 0] = x     + (z       * MASS_H_POINT);
			g_MeshIndex[i + 1] = x + 1 + ((z + 1) * MASS_H_POINT) ;
			g_MeshIndex[i + 2] = x + 1 + (z       * MASS_H_POINT) ;
			g_MeshIndex[i + 3] = x     + (z       * MASS_H_POINT) ;
			g_MeshIndex[i + 4] = x     + ((z + 1) * MASS_H_POINT) ;
			g_MeshIndex[i + 5] = x + 1 + ((z + 1) * MASS_H_POINT) ;

			i += 6;
		}
	}


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
	sd.pSysMem = g_MeshVertex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	// インデックスバッファ作成
	bd.Usage = D3D11_USAGE_DEFAULT; // Usage:使い方　DYNAMIC:書き換えて使う
	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0; // 1度作成したら動かさない 読み書き不可

	sd.pSysMem = g_MeshIndex;

	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);

	ShaderField_Initialize(pDevice, pContext); // 本来のシェーダーはmain こちらはフィールドでのみ使用するためココで初期化
}

void MeshField_Finalize(void){
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

void MeshField_Update(double elapsed_time){

}

void MeshField_Draw(){
	// シェーダーを描画パイプラインに設定
	ShaderField_Begin();
	// テクスチャの設定
	Texture_SetTexture(Resouce_Manager_GetTexId(Color_Ground1), 0);
	Texture_SetTexture(Resouce_Manager_GetTexId(Color_Ground2), 1);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// インデックスバッファを描画パイプラインに設定
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0); // unsined short = R16 unsined int = R32
	
	// ワールド座標変換行列の作成
	XMMATRIX mtmWorld = XMMatrixIdentity(); // 単位行列

	float offset_x = MESH_H_SIZE * 0.5f;
	float offset_z = MESH_V_SIZE * 0.5f;

	XMMATRIX mtmTrans = XMMatrixTranslation(-offset_x, 0.0f, -offset_z);
	mtmWorld *= mtmTrans;

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	// 頂点シェーダーにワールド座標変換行列を設定
	ShaderField_SetWorldMatrix(mtmWorld);

	ShaderField_SetColor({ 1.0f,1.0f,1.0f,1.0f });

	// ポリゴン描画命令発行
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

AABB MeshField_GetAABB()
{
	return { { -(float)(MASS_V_COUNT) * 0.5f, -2.0f , -(float)(MASS_H_COUNT) * 0.5f},
			 {  (float)(MASS_V_COUNT) * 0.5f, -0.01f,  (float)(MASS_H_COUNT) * 0.5f} };
}

