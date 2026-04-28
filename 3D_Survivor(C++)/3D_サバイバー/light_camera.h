/*
	ライト方向からのカメラ制御：light_camera.h

	2025/12/10	hibiki sakuma
*/
#ifndef LIGHT_CAMERA_H
#define LIGHT_CAMERA_H

#include <DirectXMath.h>

void LightCamera_Initialize(const DirectX::XMFLOAT3& world_directional, const DirectX::XMFLOAT3& position);
void LightCamera_Finalize();
// 時間によって動かす場合に使用
void LightCamera_SetPosition(const DirectX::XMFLOAT3& position);
void LightCamera_SetFront(const DirectX::XMFLOAT3& front); 

// カメラ行列をシェーダーに入れる用
//const DirectX::XMFLOAT4X4& LightCamera_GetViewMatrix();
//const DirectX::XMFLOAT4X4& LightCamera_GetPerspectiveMatrix();
const DirectX::XMMATRIX LightCamera_GetViewMatrix();
const DirectX::XMMATRIX LightCamera_GetPerspectiveMatrix();

#endif // !LIGHT_CAMERA_H
