/*
	ライトの設定：light.cpp

	2025/09/30	hibiki sakuma
*/

#include "light.h"
#include "direct3d.h"

using namespace DirectX;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11Buffer* g_pPSConstantBuffer1 = nullptr; // 定数バッファb1
static ID3D11Buffer* g_pPSConstantBuffer2 = nullptr; // 定数バッファb2
static ID3D11Buffer* g_pPSConstantBuffer3 = nullptr; // 定数バッファb3
static ID3D11Buffer* g_pPSConstantBuffer4 = nullptr; // 定数バッファb3

// 構造体
struct DirectionalLight { // 並行光源　拡散反射光
	XMFLOAT4 Directional; 
	XMFLOAT4 color;
};

struct SpecularLight { // 鏡面反射光
	XMFLOAT3 CameraPosition;
	float Power;
	XMFLOAT4 Color;
};

struct PointLight { // 点光源
	XMFLOAT3 Position;
	float Range;
	XMFLOAT4 Color;
};
struct PointLightList {
	PointLight light[4];
	int count;
	float dummy;
};

// スポットライト

// グローバル変数
static PointLightList g_PointLights{};

void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext){
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;
	
	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer1); // ambient

	buffer_desc.ByteWidth = sizeof(DirectionalLight); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer2); // direction
	
	buffer_desc.ByteWidth = sizeof(SpecularLight); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer3); // specular

	buffer_desc.ByteWidth = sizeof(PointLight); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer4); // point

	
}

void Light_Finalize(){
	SAFE_RELEASE(g_pPSConstantBuffer4);
	SAFE_RELEASE(g_pPSConstantBuffer3);
	SAFE_RELEASE(g_pPSConstantBuffer2);
	SAFE_RELEASE(g_pPSConstantBuffer1);
}

void Light_SetAmbient(const DirectX::XMFLOAT3& color){

	// 定数バッファにambient行列をセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer1, 0, nullptr, &color, 0, 0);
	g_pContext->PSSetConstantBuffers(1, 1, &g_pPSConstantBuffer1);

}

void Light_SetDirectionalWorld(const DirectX::XMFLOAT4& world_directional, const DirectX::XMFLOAT4& color){
	DirectionalLight light{
		world_directional,
		color,
	};

	// 定数バッファにambient行列をセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer2, 0, nullptr, &light, 0, 0);
	g_pContext->PSSetConstantBuffers(2, 1, &g_pPSConstantBuffer2);

}

void Light_SetSpecularWorld(const DirectX::XMFLOAT3 camera_position, const float power, const DirectX::XMFLOAT4& color){
	SpecularLight light{
		camera_position,
		power,
		color
	};

	// 定数バッファにambient行列をセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer3, 0, nullptr, &light, 0, 0);
	g_pContext->PSSetConstantBuffers(3, 1, &g_pPSConstantBuffer3);

}

void Light_SetPointLightCount(int count){
	g_PointLights.count = count;
	
	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_PointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);

}

void Light_SetPointWorld(int n,const DirectX::XMFLOAT3 position, const float range, const DirectX::XMFLOAT3& color) {
	g_PointLights.light[n].Position = position;
	g_PointLights.light[n].Range = range;
	g_PointLights.light[n].Color = { color.x,color.y,color.z,1.0f };

	// 定数バッファにambient行列をセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_PointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);

}

