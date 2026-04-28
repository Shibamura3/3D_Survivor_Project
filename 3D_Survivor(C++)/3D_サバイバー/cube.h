/*
	3Dキューブの表示（立方体）：cube.h

	2025/09/09	hibiki sakuma
*/

#ifndef  CUBE_H
#define CUBE_H

#include <d3d11.h>
#include <DirectXMath.h>	
#include "collision.h"

void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Cube_Finalize(void);
void Cube_Update(double elapsed_time);
void Cube_Draw(int texId, const DirectX::XMMATRIX& mtmWorld); // 行列のキューブを描画する
void Cube_DepthDraw(int texId, const DirectX::XMMATRIX& mtmWorld);

AABB Cube_GetAABB(const DirectX::XMFLOAT3& position);

#endif // ! CUBE_H
