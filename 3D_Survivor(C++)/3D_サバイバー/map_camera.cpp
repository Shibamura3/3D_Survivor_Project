/*
	マップ上方向からのカメラ制御：map_camera.cpp

	2025/12/10	hibiki sakuma
*/

#include "map_camera.h"
using namespace DirectX;

// 定数宣言
static constexpr XMVECTOR DOWN_LOOK = { 0.0f,-1.0f,0.0f }; // 下方向を見る

// グローバル変数宣言
static XMFLOAT3 g_Position{};
static XMFLOAT3 g_Front{};

void MapCamera_Initialize(){
}

void MapCamera_Finalize(){
}

void MapCamera_SetPosition(const DirectX::XMFLOAT3& position){
	g_Position = position;
}

void MapCamera_SetFront(const DirectX::XMFLOAT3& front){
	g_Front = front;
}

const DirectX::XMFLOAT4X4& MapCamera_GetViewMatrix(){
	XMFLOAT4X4 mtxView;

	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&g_Position), DOWN_LOOK, XMLoadFloat3(&g_Front));

	XMStoreFloat4x4(&mtxView, view);

	return mtxView;
}

const DirectX::XMFLOAT4X4& MapCamera_GetPerspectiveMatrix(){
	XMFLOAT4X4 mtxProje;

	//XMMATRIX proj = XMMatrixOrthographicOffCenterLH(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 1000.0f);
	XMMATRIX proj = XMMatrixOrthographicOffCenterLH(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 10000.0f);

	XMStoreFloat4x4(&mtxProje, proj);

	return mtxProje;
}
