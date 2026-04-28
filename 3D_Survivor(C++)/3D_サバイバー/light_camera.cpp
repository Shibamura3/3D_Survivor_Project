/*
	ライト方向からのカメラ制御：light_camera.cpp

	2025/12/10	hibiki sakuma
*/
#include "light_camera.h"

using namespace DirectX;

static XMFLOAT3 g_Position{};
static XMFLOAT3 g_Front{};

void LightCamera_Initialize(const DirectX::XMFLOAT3& world_directional, const DirectX::XMFLOAT3& position){
	g_Front = world_directional;
	g_Position = position;
}

void LightCamera_Finalize(){

}

void LightCamera_SetPosition(const DirectX::XMFLOAT3& position){
	g_Position = position;
}

void LightCamera_SetFront(const DirectX::XMFLOAT3& front){
	g_Front = front;
}

//const DirectX::XMFLOAT4X4& LightCamera_GetViewMatrix(){
//	XMFLOAT4X4 mtxView;
//
//	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&g_Position), XMVECTOR{ 0.0f, -1.0f,0.0f }, XMLoadFloat3(&g_Front));
//
//	XMStoreFloat4x4(&mtxView, view);
//
//	return mtxView;
//}

//const DirectX::XMFLOAT4X4& LightCamera_GetPerspectiveMatrix(){
//	XMFLOAT4X4 mtxpers;
//
//	float value = 40.0f;
//
//	XMMATRIX pers = XMMatrixOrthographicOffCenterLH(-value, value, -value, value, 0.1f, 10000.0f);
//
//	XMStoreFloat4x4(&mtxpers, pers);
//
//	//return mtxpers;
//	
//}

const DirectX::XMMATRIX LightCamera_GetViewMatrix() {
	return XMMatrixLookToLH(XMLoadFloat3(&g_Position), XMVECTOR{ 0.0f, -1.0f,0.0f }, XMLoadFloat3(&g_Front));
}

const DirectX::XMMATRIX LightCamera_GetPerspectiveMatrix() {
	float range = 60.0f; // 影を表示する範囲（広すぎると影がボケます）
	return XMMatrixOrthographicOffCenterLH(-range, range, -range, range, 0.1f, 500.0f);
}
