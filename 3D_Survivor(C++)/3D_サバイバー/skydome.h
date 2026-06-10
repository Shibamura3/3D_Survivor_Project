/*
	ãÛÇÃï`âÊÅ@

	2025/11/21	hibiki sakuma
*/

#ifndef SKYDOME_H
#define SKYDOME_H

#include <DirectXMath.h>

void Skydome_Initialize();
void Skydome_Finalize();
void Skydonm_SetPosition(const DirectX::XMFLOAT3& position);
void Skydome_Draw();

#endif // !SKYDOME_H
