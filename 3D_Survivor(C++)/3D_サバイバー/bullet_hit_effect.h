/*
	弾丸の衝突エフェクトの表示：bullet_hit_effect.h

	2025/11/19	hibiki sakuma
*/
#ifndef BULLET_HIT_EFFECT_H
#define BULLET_HIT_EFFECT_H

#include <DirectXMath.h>

// C++のポリモーフィズムでエフェクトを一括管理できる
void Bullet_HitEffect_Initialize();
void Bullet_HitEffect_Finalize();
void Bullet_HitEffect_Updata();
void Bullet_HitEffect_Draw();

void Bullet_HitEffect_Create(const DirectX::XMFLOAT3& position);

#endif // !BULLET_HIT_EFFECT_H

