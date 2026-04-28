/*==============================================================================

   シェーダー3Dフィールド用 [shaderfield.h]
														 Author : Youhei Sato
														 Date   : 2025/09/26
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADERFIELD_H
#define	SHADERFIELD_H

#include <d3d11.h>
#include <DirectXMath.h>

bool ShaderField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ShaderField_Finalize();
		  
void ShaderField_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void ShaderField_SetLightMatrix(const DirectX::XMMATRIX& matrix);
void ShaderField_SetColor(const DirectX::XMFLOAT4& color);
void ShaderField_Begin();

#endif // SHADERFIELD_H