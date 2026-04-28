/*==============================================================================

   インスタンス描画専用シェーダー3D用 [shader3d_instance.h]
														 Author : Hibiki sakuma
														 Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER3D_INSTANCE_H
#define	SHADER3D_INSTANCE_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader3d_Instance_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader3d_Instance_Finalize();

void Shader3d_Instance_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

void Shader3d_Instance_SetColor(const DirectX::XMFLOAT4& color);;

void Shader3d_Instance_Begin();

#endif // SHADER3D_INSTANCE_H