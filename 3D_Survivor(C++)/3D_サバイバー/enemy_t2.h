/*
	敵(タイプ2)の制御：enemy_t2.h

	2026/01/11	hibiki sakuma
*/
#ifndef ENEMY_T2_H
#define ENEMY_T2_H

#include "enemy.h"
#include "texture.h"
#include "model.h"
//#include "route_search.h"]
#include <cmath>
#include <DirectXMath.h>

class Enemy_T2 : public Enemy {
private:
	static MODEL* s_pModel; 
	float m_angle = 0.0f;
public:
	static void LoadModel();    // 起動時に1回だけ呼ぶ
	static void UnloadModel();  // 終了時に1回だけ呼ぶ

	void Activate(const DirectX::XMFLOAT3& pos) override;
	void Update(double elapsed_time) override;

	int GetTypeIndex() const override { return 2; }
	static MODEL* GetModel() { return s_pModel; }
	DirectX::XMMATRIX GetWorldMatrix() const {
		return DirectX::XMMatrixRotationY(m_angle + DirectX::XMConvertToRadians(180.0f)) * DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	}
	Sphere GetCollision() const {
		return { m_position, 1.25f };
	};
};

#endif // !ENEMY_T2_H
