/*
	敵(タイプ１)の制御：enemy_t1.h

	2025/11/26	hibiki sakuma
*/
#ifndef ENEMY_T1_H
#define ENEMY_T1_H

#include "enemy.h"
#include "texture.h"
#include "model.h"
//#include "route_search.h"
#include <cmath>
#include <DirectXMath.h>

class Enemy_T1 : public Enemy {
private:
	static MODEL* s_pModel; 
	float m_speed = 1.5f;
	float m_angle = 0.0f;
	// T1専用のカプセル情報を保持
	Capsule GetCapsule() const;
public:
	static void LoadModel();    // 起動時に1回だけ呼ぶ
	static void UnloadModel();  // 終了時に1回だけ呼ぶ

	void Activate(const DirectX::XMFLOAT3& pos) override;
	void Update(double elapsed_time) override;

	int GetTypeIndex() const override { return 1; }
	static MODEL* GetModel() { return s_pModel; }
	DirectX::XMMATRIX GetWorldMatrix() const {
		return DirectX::XMMatrixRotationY(m_angle + DirectX::XMConvertToRadians(180.0f)) * DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	}
	Sphere GetCollision() const {
		return { m_position, 1.25f };
	};
};

#endif // !ENEMY_T1_H
