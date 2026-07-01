/*
	敵(タイプ１)の制御：enemy_t1.cpp

	2025/11/26	hibiki sakuma
*/

#include "enemy_t1.h"
#include "collision.h"
#include "player.h"
#include "cube.h"
#include "map.h"
#include "shader3d.h"
#include "resource_manager.h"
#include <cmath>
using namespace DirectX;
MODEL* Enemy_T1::s_pModel = nullptr;

// 定数宣言
static constexpr float SKIP_METER = 5.0f * 5.0f;
static constexpr float OFFSET_Y = 1.0f;
Capsule Enemy_T1::GetCapsule() const{
	Capsule c{};
	c.radius = 0.5f; // 敵の太さ
	c.start = m_position; // 足元
	c.end = m_position;
	c.start.y -= 0.50f; // 身長
	return c;
}

void Enemy_T1::LoadModel() {
	if (!s_pModel) s_pModel = Resouce_Manager_GetModelId(EnemyModel_T1);
}

void Enemy_T1::UnloadModel() {
	s_pModel = nullptr;
}

void Enemy_T1::Activate(const DirectX::XMFLOAT3& pos) {
	m_position = pos;
	
	// 地面に合わせる    
	float groundY = Map_GetGroundHeight(pos.x, pos.z, pos.y);
	m_position.y = groundY + OFFSET_Y;

	m_hp = 10;
	m_stateTimer = 0.0f;
	m_isActive = true; // ここで「生きている」状態にする
}

void Enemy_T1::Update(double elapsed_time) {
	if (!m_isActive) return;

	float groundY = Map_GetGroundHeight(m_position.x, m_position.z, m_position.y);
	m_position.y = groundY + OFFSET_Y;

	// 移動計算
	XMVECTOR vPos = XMLoadFloat3(&m_position);
	XMVECTOR vPlayerPos = XMLoadFloat3(&GetPlayer()->GetPosition());
	XMVECTOR vDir = XMVector3Normalize(XMVectorSetY(vPlayerPos - vPos, 0.0f));

	XMFLOAT3 dir;
	XMStoreFloat3(&dir, vDir);
	m_angle = atan2f(dir.x, dir.z);

	// 移動実行
	vPos += vDir * m_speed * (float)elapsed_time;
	XMStoreFloat3(&m_position, vPos);

	
		// 障害物との当たり判定
		Capsule myCap = GetCapsule();
		for (int i = 0; i < Map_GetObjectsCount(); i++) {
			auto obj = Map_GetObject(i);

			// 敵とマップ上の障害物の距離をチェック
			float dx = obj->Position.x - m_position.x;
			float dz = obj->Position.z - m_position.z;
			float distSq = dx * dx + dz * dz; // 平方根計算(sqrt)を避けるため2乗のまま比較

			// Xメートル以上離れていたらこの箱との判定はスキップする
			if (distSq > SKIP_METER) {
				continue;
			}

			// 近くにある障害物のみ衝突判定
			Hit hit = Collision_IsHitCapsulevsAABB(myCap, Map_GetObject(i)->Aabb_collision);
			if (hit.isHit) {
				vPos = XMLoadFloat3(&m_position);
				vPos += XMLoadFloat3(&hit.normal) * hit.depth;
				XMStoreFloat3(&m_position, vPos);
				myCap.start = m_position; // 次の判定用に更新
				myCap.end = m_position;
				myCap.end.y += 1.6f;
			}

		}

	if (m_position.y < 1.0f ) m_position.y = 1.0f;
	
	if (m_hp <= 0) m_isActive = false;

}