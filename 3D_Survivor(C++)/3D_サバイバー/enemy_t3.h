/*
	敵(タイプ3)の制御：enemy_t3.h

	2026/01/11	hibiki sakuma
*/
#ifndef ENEMY_T3_H
#define ENEMY_T3_H

#include "enemy.h"
#include "texture.h"
#include "model.h"
//#include "route_search.h"
#include <cmath>
#include <DirectXMath.h>

class Enemy_T3 : public Enemy {
private:
	static MODEL* s_pModel; 
	double m_shotTimer = 0.0;
	float m_angle = 0.0f;
public:
	static void LoadModel();    // 起動時に1回だけ呼ぶ
	static void UnloadModel();  // 終了時に1回だけ呼ぶ

	void Activate(const DirectX::XMFLOAT3& pos) override;
	void Update(double elapsed_time) override;

	int GetTypeIndex() const override { return 3; }
	static MODEL* GetModel() { return s_pModel; }
	DirectX::XMMATRIX GetWorldMatrix() const {
		return DirectX::XMMatrixRotationY(m_angle + DirectX::XMConvertToRadians(180.0f)) * DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	}
	Sphere GetCollision() const {
		return { m_position, 1.25f };
	};
};

#endif // !ENEMY_T3_H