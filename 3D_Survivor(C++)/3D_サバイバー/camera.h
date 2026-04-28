/*
	カメラ制御：camera.h

	2025/09/09	hibiki sakuma
*/

#ifndef CAMERA_H
#define CAMERA_H

#include <DirectXMath.h>

void Camera_Initialize(const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& front,
	const DirectX::XMFLOAT3& right);

void Camera_Initialize();

void Camera_Finalize();
void Camera_Update(double elapsed_time);

// カメラ行列をシェーダーに入れる用
const DirectX::XMFLOAT4X4& Camera_GetViewMatrix();
const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix();

const  DirectX::XMFLOAT3& Camera_GetFront();
const  DirectX::XMFLOAT3& Camera_GetPosition();

float Camera_GetFov();

void Camera_SetMatrix(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection);
void Camera_DebugDraw();

#endif // !CAMERA_H
