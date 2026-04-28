/*
	敵の弾丸の管理：bullet_enemy.h

	2026/01/11	hibiki sakuma
*/

#ifndef BULLET_ENEMY_H
#define BULLET_ENEMY_H

#include <DirectXMath.h>
#include "collision.h"

struct BulletEnemyRay
{
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
};

void Bullet_Enemy_Initialize();
void Bullet_Enemy_Finalize();
void Bullet_Enemy_Update(double elapsed_time);
void Bullet_Enemy_Draw();

void Bullet_Enemy_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity);

// ゲッター
int Bullet_Enemy_GetBulletCount();
const  DirectX::XMFLOAT3& Bullet_Enemy_GetPosition(int index);
bool Bullet_Enemy_IsActive(int index);      // 指定したインデックスの弾が生きているか
// セッター
void Bullet_Enemy_Deactivate(int index);    // 指定したインデックスの弾を無効化する

// 当たり判定
BulletEnemyRay Bullet_Enemy_GetRay(int index);
AABB Bullet_Enemy_GetAABB(int index);
Sphere Bullet_Enemy_GetSphere(int index);


#endif // !BULLET_ENEMY_H
