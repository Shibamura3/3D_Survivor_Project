/*
	敵の弾丸の管理：bullet_enemy.cpp

	2026/01/11	hibiki sakuma
*/

#include "bullet_enemy.h"
#include "model.h"
#include "resource_manager.h"
#include "trajectory3d.h"
using namespace DirectX;

// 定数宣言
static constexpr double TIME_LIMIT = 3.0;
static constexpr double MAX_LIMIT = 1.0; // 弾が画面に残っている時間

class  Bullet_Enemy {
	XMFLOAT3 m_position{};
	XMFLOAT3 m_prevposition{};
	XMFLOAT3 m_velocity{};
	double m_accumlatedTime = 0;
	bool m_isActive = false; // 弾のアクティブ判定
public:
	// デフォルトコンストラクタ 配列生成時
	Bullet_Enemy() : m_isActive(false) {}

	// 再利用（発射）するための関数
	void Activate(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity) {
		m_position = position;
		m_prevposition = position;
		m_velocity = velocity;
		m_accumlatedTime = 0;
		m_isActive = true; // 生存フラグを立てる
	}

	void Updata(double elapsed_time) {
		if (!m_isActive) return;
		m_accumlatedTime += elapsed_time;
		m_prevposition = m_position; // 直前の位置を保存
		XMVECTOR vPrev = XMLoadFloat3(&m_position); // 更新前の位置
		XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + XMLoadFloat3(&m_velocity) * elapsed_time); // 移動処理
		XMVECTOR vCurr = XMLoadFloat3(&m_position); // 更新後の位置
		if (m_isActive) { // 生きている間だけエフェクトを出す
			for (float t = 0.5f; t <= 1.0f; t += 0.5f) {
				XMFLOAT3 spawnPos;
				XMStoreFloat3(&spawnPos, XMVectorLerp(vPrev, vCurr, t));
				Trajectory3d_Create(spawnPos, { 1.0f,0.2f,0.2f,1.0f }, 0.5f, 0.50f);
			}
		}
		if (m_accumlatedTime >= 3.0) m_isActive = false; // 時間切れで非アクティブに

	}

	const XMFLOAT3& GetPosition() const { return m_position; }

	XMFLOAT3 GetFront() const {
		XMFLOAT3 front;
		XMStoreFloat3(&front, XMVector3Normalize(XMLoadFloat3(&m_velocity)));
		return front;
	}

	bool IsActive() const { return m_isActive; }
	void Deactivate() { m_isActive = false; }
	bool IsDestroy() const { return m_accumlatedTime >= TIME_LIMIT; }
	const XMFLOAT3& GetPrevPosition() const { return m_prevposition; }
};

static constexpr int MAX_BULLET = 256;

// 管理部分
static Bullet_Enemy g_EnemyBullets[MAX_BULLET]; // ポインタではなく実体配列

void Bullet_Enemy_Initialize() {
	for (int i = 0; i < MAX_BULLET; i++) {
		//delete g_Bullet[i];
		g_EnemyBullets[i].Deactivate(); // 全ての弾を非アクティブ化
	}
}

void Bullet_Enemy_Finalize() {
	for (int i = 0; i < MAX_BULLET; i++) {
		//delete g_pBullet[i];
		g_EnemyBullets[i].Deactivate(); // 全ての弾を非アクティブ化
	}
}

void Bullet_Enemy_Update(double elapsed_time) {
	for (int i = 0; i < MAX_BULLET; i++) {
		if (g_EnemyBullets[i].IsActive()) {
			g_EnemyBullets[i].Updata(elapsed_time);

			// 寿命が来たら無効化
			if (g_EnemyBullets[i].IsDestroy()) {
				g_EnemyBullets[i].Deactivate();
			}
		}
	}
}

void Bullet_Enemy_Draw() {
	XMMATRIX mtxWorld;
	for (int i = 0; i < MAX_BULLET; i++) {
		if (!g_EnemyBullets[i].IsActive()) continue; // 非アクティブに弾を無視
		XMVECTOR position = XMLoadFloat3(&g_EnemyBullets[i].GetPosition());
		mtxWorld = XMMatrixTranslationFromVector(position);
		ModelDraw(Resouce_Manager_GetModelId(Bullet_Model), mtxWorld);
	}
}


void Bullet_Enemy_Create(const XMFLOAT3& position, const XMFLOAT3& velocity) {
	for (int i = 0; i < MAX_BULLET; i++) {
		if (!g_EnemyBullets[i].IsActive()) {
			g_EnemyBullets[i].Activate(position, velocity);
			return; // createは１つのみ
		}
	}
}


int Bullet_Enemy_GetBulletCount() { return MAX_BULLET; }

const DirectX::XMFLOAT3& Bullet_Enemy_GetPosition(int index) {
	return g_EnemyBullets[index].GetPosition();
}

bool Bullet_Enemy_IsActive(int index) { return g_EnemyBullets[index].IsActive(); }

void Bullet_Enemy_Deactivate(int index) { g_EnemyBullets[index].Deactivate(); }

BulletEnemyRay Bullet_Enemy_GetRay(int index) {
	BulletEnemyRay ray;
	ray.start = g_EnemyBullets[index].GetPrevPosition();
	ray.end = g_EnemyBullets[index].GetPosition();
	return ray;
}


AABB Bullet_Enemy_GetAABB(int index)
{
	return Model_GetAABB(Resouce_Manager_GetModelId(Bullet_Model), g_EnemyBullets[index].GetPosition());
}

Sphere Bullet_Enemy_GetSphere(int index)
{
	return { g_EnemyBullets[index].GetPosition(), Resouce_Manager_GetModelId(Bullet_Model)->local_aabb.GetCenter().x };
}

