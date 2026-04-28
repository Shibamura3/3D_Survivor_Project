/*
	‹ó‚Ì•`‰æ@

	2025/11/21	hibiki sakuma
*/

#ifndef SKYDOME_H
#define SKYDOME_H

#include <DirectXMath.h>

void Skydome_Initialize();
void Skydome_Finalize();
//void Skydome_Update(double elapsed_time); // ‰_‚ğ“®‚©‚·ê‡‚Í‚Q–‡—pˆÓ‚µ‚Ä‰_‚¾‚¯‚ğ‰ñ‚·
void Skydone_SetPosition(const DirectX::XMFLOAT3& position);
void Skydome_Draw();

#endif // !SKYDOME_H
