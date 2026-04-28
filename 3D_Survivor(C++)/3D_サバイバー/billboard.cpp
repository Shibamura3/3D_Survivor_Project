/*
	ビルボードの描画：billboard.h

	2025/11/14	hibiki sakuma
*/

#include "billboard.h"
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "sheder_billboard.h"
#include "texture.h"
#include "player_camera.h"


static constexpr int NUM_VERTEX = 4; // 頂点数

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

static XMFLOAT4X4 g_mtxView{}; // ビュー行列の平行移動成分をカットした行列

// 頂点構造体
struct Vertex3d
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT4 color;    // 色　
	XMFLOAT2 texcoord; // UV
};


void Billboard_Initialize(){
	ShaderBillboard_Initialize();

	static Vertex3d Vertex[]{
		// 足元が中心になって回転
		{{ -0.5f,  1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 0.0f, 0.0f}},
		{{  0.5f,  1.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 1.0f, 0.0f}},
		{{ -0.5f,  0.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 0.0f, 1.0f}},
		{{  0.5f,  0.0f, 0.0f},{1.0f, 1.0f, 1.0f, 1.0f},{ 1.0f, 1.0f}}
	};

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT; // Usage:使い方　DYNAMIC:書き換えて使う
	bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX; // sizeof(g_CubeVertex)配列なのでこれでもおｋ
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0; // 1度作成したら動かさない 読み書き不可

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = Vertex;

	Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
}

void Billboard_Finalize(void){
	SAFE_RELEASE(g_pVertexBuffer);
	ShaderBillboard_Finalize();
}

void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view){
	// カメラの回転分だけ逆行列を作る
	g_mtxView = view;
	g_mtxView._41 = g_mtxView._42 = g_mtxView._43 = 0.0f;
}

void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4 color, const DirectX::XMFLOAT2& pivot){
	ShaderBillboard_SetUVParameter({ {1.0f,1.0f}, {0.0f,0.0f} });	

	ShaderBillboard_Begin();

	ShaderBillboard_SetColor(color);
	//
	Texture_SetTexture(texId);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 行列の演算
	// カメラ行列の回転だけ逆行列を作る
	XMFLOAT4X4 mtxCamera = Player_Camera_GetViewMatrix();
	mtxCamera._41 = mtxCamera._42 = mtxCamera._43 = 0.0f;
	// XMMATRIX iv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mtxCamera)); // インヴァースビュー 逆行列の生成が一番負担が大きい
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&g_mtxView)); // 転置行列 上と同じ結果になる

	// 回転までのオフセット行列
	XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);

	XMMATRIX scal = XMMatrixScaling(scale.x, scale.y, 1.0f);
	XMMATRIX trans = XMMatrixTranslation(position.x + pivot.x, position.y + pivot.y, position.z);

	// 頂点シェーダにワールド座標変換行列に設定
	ShaderBillboard_SetWorldMatrix(scal * pivot_offset * iv * trans); // offsetを中心に変換を行う
	// ポリゴン描画命令発行
	Direct3D_GetContext()->Draw(NUM_VERTEX, 0);

}

void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMUINT4 tex_cut, const DirectX::XMFLOAT4 color, const DirectX::XMFLOAT2& pivot) {
	float texW = (float)(Texture_Width(texId));
	float texH = (float)(Texture_Height(texId));
	
	float uv_x = (float)tex_cut.x / texW; // uvの幅の計算
	float uv_y = (float)tex_cut.y / texH; // uvの高さの計算
	float uv_w = (float)tex_cut.z / texW; // uvの幅の計算
	float uv_h = (float)tex_cut.w / texH; // uvの高さの計算

	ShaderBillboard_SetUVParameter({ {uv_w,uv_h} ,{uv_x,uv_y}});

	ShaderBillboard_Begin();

	ShaderBillboard_SetColor(color);
	//
	Texture_SetTexture(texId);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	Direct3D_GetContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// インデックスバッファを描画パイプラインに設定
	Direct3D_GetContext()->IASetIndexBuffer(nullptr/*使用しない宣言*/, DXGI_FORMAT_R16_UINT, 0); // unsined short = R16 unsined int = R32

	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 行列の演算
	// XMMATRIX iv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mtxCamera)); // インヴァースビュー 逆行列の生成が一番負担が大きい
	XMMATRIX iv = XMMatrixTranspose(XMLoadFloat4x4(&g_mtxView)); // 転置行列 上と同じ結果になる

	// 回転までのオフセット行列
	XMMATRIX pivot_offset = XMMatrixTranslation(-pivot.x, -pivot.y, 0.0f);

	XMMATRIX scal = XMMatrixScaling(scale.x, scale.y, 1.0f);
	XMMATRIX trans = XMMatrixTranslation(position.x + pivot.x, position.y + pivot.y, position.z);

	// 頂点シェーダにワールド座標変換行列に設定
	ShaderBillboard_SetWorldMatrix(scal * pivot_offset * iv * trans); // offsetを中心に変換を行う
	// ポリゴン描画命令発行
	Direct3D_GetContext()->Draw(NUM_VERTEX, 0);
}

