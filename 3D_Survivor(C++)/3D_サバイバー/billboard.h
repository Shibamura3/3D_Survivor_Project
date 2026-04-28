/*
	ビルボードの描画：billboard.h

	2025/11/14	hibiki sakuma
*/
#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <d3d11.h>
#include <DirectXMath.h>	

void Billboard_Initialize();
void Billboard_Finalize(void);

void Billboard_SetViewMatrix(const DirectX::XMFLOAT4X4& view);

void Billboard_Draw(int texId, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0 }, const DirectX::XMFLOAT2& pivot = {0.0f,0.0f});
void Billboard_Draw(int texId,
					const DirectX::XMFLOAT3& position,
					const DirectX::XMFLOAT2& scale,
					const DirectX::XMUINT4 tex_cut ,// 切り取り始めと切り取りサイズ
					const DirectX::XMFLOAT4 color = {1.0f,1.0f,1.0f,1.0},
					const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }
);

#endif // !BILLBOARD_H
