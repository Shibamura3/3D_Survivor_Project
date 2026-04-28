/*
	メッシュフィールドの表示　：meshfield.h

	2025/09/19	hibiki sakuma
*/

#ifndef MASHFIELD_H
#define MASHFIELD_H

#include <d3d11.h>
#include "collision.h"
#include <DirectXMath.h>	

void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void MeshField_Finalize(void);
void MeshField_Update(double elapsed_time);
void MeshField_Draw(); // 行列のキューブを描画する

AABB MeshField_GetAABB();
#endif // ! MASHFIELD_H