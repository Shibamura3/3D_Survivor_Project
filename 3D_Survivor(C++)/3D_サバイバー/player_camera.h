/*
	プレイヤーカメラ制御：player_camera.h

	2025/10/31	hibiki sakuma
*/

#ifndef PLAYER_CAMERA_H
#define PLAYER_CAMERA_H

#include <DirectXMath.h>
void Player_Camera_Initialize();
void Player_Camera_Finalize();

void Player_Camera_Update(double elapsed_time);

// カメラ行列をシェーダーに入れる用
const DirectX::XMFLOAT3& Player_Camera_GetPosition();
const DirectX::XMFLOAT3& Player_Camera_GetFront(); // カメラがどこを向いているか
const DirectX::XMFLOAT4X4& Player_Camera_GetViewMatrix();
const DirectX::XMFLOAT4X4& Player_Camera_GetPerspectiveMatrix();

#endif // !PLAYER_CAMERA_H
