/*
	弾丸の管理：bullet.h

	2025/11/12	hibiki sakuma
*/

#ifndef BULLET_H
#define BULLET_H

#include <DirectXMath.h>
#include "collision.h"

struct BulletRay
{
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
};

void Bullet_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front);
void Bullet_Finalize();
void Bullet_Update(double elapsed_time);
void Bullet_Draw();

void Bullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity);

// ゲッター
int Bullet_GetBulletCount();
const  DirectX::XMFLOAT3& Bullet_GetPosition(int index);
bool Bullet_IsActive(int index);      // 指定したインデックスの弾が生きているか
// セッター
void Bullet_Deactivate(int index);    // 指定したインデックスの弾を無効化する

// 当たり判定
BulletRay Bullet_GetRay(int index);
AABB Bullet_GetAABB(int index);
Sphere Bullet_GetSphere(int index);


#endif // !BULLET_H
