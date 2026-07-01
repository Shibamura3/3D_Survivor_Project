/*
	敵(タイプ2)の制御：enemy_t2.cpp

	2026/01/11	hibiki sakuma
*/

#include "enemy_t2.h"
#include "collision.h"
#include "player.h"
#include "map.h"
#include "cube.h"
#include "shader3d.h"
#include "resource_manager.h"
#include <cmath>
using namespace DirectX;
MODEL* Enemy_T2::s_pModel = nullptr;

static constexpr float ENEMY_SPEED = 3.0f;
static constexpr float OFFSET_Y = 1.0f;

void Enemy_T2::LoadModel() {
	if (!s_pModel) s_pModel = Resouce_Manager_GetModelId(EnemyModel_T2);
}

void Enemy_T2::UnloadModel() {
	s_pModel = nullptr;
}

void Enemy_T2::Activate(const DirectX::XMFLOAT3& pos) {
	m_position = pos;
	// 地面に合わせる    
	float groundY = Map_GetGroundHeight(pos.x, pos.z, pos.y);
	m_position.y = groundY + OFFSET_Y;
	m_hp = 10;
	m_stateTimer = 0.0f;
	m_isActive = true; // ここで「生きている」状態にする
}

void Enemy_T2::Update(double elapsed_time) {
	if (!m_isActive) return;

	// 移動計算
	XMVECTOR vPos = XMLoadFloat3(&m_position);
	XMVECTOR vPlayerPos = XMLoadFloat3(&GetPlayer()->GetPosition());
	XMVECTOR vDir = XMVector3Normalize(vPlayerPos - vPos);

	XMFLOAT3 dir;
	XMStoreFloat3(&dir, vDir);
	m_angle = atan2f(dir.x, dir.z);

	XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + vDir * ENEMY_SPEED * (float)elapsed_time);

	if (m_hp <= 0) m_isActive = false;
}
