/*
	ÉâÉCÉgÇÃê›íËÅFlight.h

	2025/09/30	hibiki sakuma
*/

#ifndef LIGHT_H
#define LIGHT_H

#include <d3d11.h>
#include <DirectXMath.h>	

void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Light_Finalize();

void Light_SetAmbient(const DirectX::XMFLOAT3& color);
void Light_SetDirectionalWorld(const DirectX::XMFLOAT4& world_directional, const  DirectX::XMFLOAT4& color);
void Light_SetSpecularWorld(const DirectX::XMFLOAT3 camera_position, const float power, const DirectX::XMFLOAT4& color);
void Light_SetPointLightCount(int count);
void Light_SetPointWorld(int n, const DirectX::XMFLOAT3 position, const float range, const DirectX::XMFLOAT3& color);


#endif // !LIGHT_H
