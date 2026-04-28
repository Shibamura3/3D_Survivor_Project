/*
	マップ上方向からのカメラ制御：map_camera.h

	2025/12/10	hibiki sakuma
*/
#ifndef Map_CAMERA_H
#define Map_CAMERA_H

#include <DirectXMath.h>

void MapCamera_Initialize();
void MapCamera_Finalize();

void MapCamera_SetPosition(const DirectX::XMFLOAT3& position);
void MapCamera_SetFront(const DirectX::XMFLOAT3& front);

// カメラ行列をシェーダーに入れる用
const DirectX::XMFLOAT4X4& MapCamera_GetViewMatrix();
const DirectX::XMFLOAT4X4& MapCamera_GetPerspectiveMatrix();

#endif // !Map_CAMERA_H
