/*
	敵(タイプ3)の制御：enemy_t3.cpp

	2026/01/11	hibiki sakuma
*/

#include "enemy_t3.h"
#include "collision.h"
#include "player.h"
#include "bullet_enemy.h"
#include "shader3d.h"
#include <cmath>
using namespace DirectX;
MODEL* Enemy_T3::s_pModel = nullptr;

static constexpr float ENEMY_SPEED = 1.5f;
static constexpr double SHOOT_INTERVAL = 5.0;
static constexpr float SHOOT_RANGE_SQ = 225.0f; // 15*15

void Enemy_T3::LoadModel() {
	if (!s_pModel) s_pModel = ModelLoad("resuce/3Dmodel/enemy/enemy_stopshoot.fbx", 0.5f);
}

void Enemy_T3::UnloadModel() {
	ModelRelease(s_pModel);
	s_pModel = nullptr;
}

void Enemy_T3::Activate(const DirectX::XMFLOAT3& pos) {
	m_position = pos;
	m_hp = 10;
	m_stateTimer = 0.0f;
	m_isActive = true; // ここで「生きている」状態にする
}

void Enemy_T3::Update(double elapsed_time) {
	if (!m_isActive) return;

	// --- 1. プレイヤーの方向を向く（回転のみ） ---
	DirectX::XMFLOAT3 playerPos = GetPlayer()->GetPosition();

	// プレイヤーへの方向と向きを計算
	float dx = playerPos.x - m_position.x;
	float dz = playerPos.z - m_position.z;
	m_angle = atan2f(dx, dz);
	float distSq = dx * dx + dz * dz;

	// 射程範囲内ならタイマー更新と発射処理
	if (distSq <= SHOOT_RANGE_SQ) {
		// 弾の発射タイマー更新
		m_shotTimer += elapsed_time;
		if (m_shotTimer >= SHOOT_INTERVAL) {
			m_shotTimer = 0.0;

			// 弾の速度ベクトルを計算（プレイヤーの方向へ）
			XMVECTOR vPos = XMLoadFloat3(&m_position);
			XMVECTOR vTarget = XMLoadFloat3(&playerPos);
			XMVECTOR vDir = XMVector3Normalize(vTarget - vPos);

			float bulletSpeed = 10.0f; // 弾の速さ
			XMFLOAT3 velocity;
			XMStoreFloat3(&velocity, vDir * bulletSpeed);

			// 少し高い位置（砲口のイメージ）から発射
			XMFLOAT3 muzzlePos = m_position;
			muzzlePos.y += 0.5f;

			Bullet_Enemy_Create(muzzlePos, velocity);
		}
	} else {
		m_shotTimer = 0.0; // タイマーをリセットしておく
	}

	// 動かない敵のため地面に固定
	m_position.y = 1.0f;

	if (m_hp <= 0) m_isActive = false;
}
