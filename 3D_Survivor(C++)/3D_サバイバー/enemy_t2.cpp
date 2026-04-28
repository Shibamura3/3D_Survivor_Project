/*
	敵(タイプ2)の制御：enemy_t2.cpp

	2026/01/11	hibiki sakuma
*/

#include "enemy_t2.h"
#include "collision.h"
#include "player.h"
#include "cube.h"
#include "shader3d.h"
#include <cmath>
using namespace DirectX;
MODEL* Enemy_T2::s_pModel = nullptr;

static constexpr float ENEMY_SPEED = 3.0f;

void Enemy_T2::LoadModel() {
	if (!s_pModel) s_pModel = ModelLoad("resuce/3Dmodel/enemy/enemy_ghost.fbx", 0.5f);
}

void Enemy_T2::UnloadModel() {
	ModelRelease(s_pModel);
	s_pModel = nullptr;
}

void Enemy_T2::Activate(const DirectX::XMFLOAT3& pos) {
	m_position = pos;
	m_hp = 10;
	m_stateTimer = 0.0f;
	m_isActive = true; // ここで「生きている」状態にする
}

void Enemy_T2::Update(double elapsed_time) {
	if (!m_isActive) return;

	// プレイヤーに向かって移動する処理（これまでの Chase 処理）
		// プレイヤーに向かって進む
	using namespace DirectX;
	// 移動計算
	XMVECTOR vPos = XMLoadFloat3(&m_position);
	XMVECTOR vPlayerPos = XMLoadFloat3(&GetPlayer()->GetPosition());
	XMVECTOR vDir = XMVector3Normalize(XMVectorSetY(vPlayerPos - vPos, 0.0f));

	XMFLOAT3 dir;
	XMStoreFloat3(&dir, vDir);
	m_angle = atan2f(dir.x, dir.z);

	XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + vDir * ENEMY_SPEED * (float)elapsed_time);

	if (m_position.y < .0f) m_position.y = 1.0f;

	if (m_hp <= 0) m_isActive = false;
}
