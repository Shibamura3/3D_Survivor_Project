/*
	敵の制御：enemy.h

	2025/11/26	hibiki sakuma
*/
#ifndef ENEMY_H
#define ENEMY_H

#include "collision.h"
#include <DirectXMath.h>

class Enemy {
protected:
	DirectX::XMFLOAT3 m_position{};
	bool m_isActive = false;
	int m_hp = 10;
	float m_stateTimer = 0.0f; // ステート内での経過時間

public:
    Enemy() : m_isActive(false) {}
    virtual ~Enemy() = default;

    // 初期化
    virtual void Activate(const DirectX::XMFLOAT3& pos) = 0; // 敵を出現させる（newの代わり）
    void Deactivate() { m_isActive = false; } // 敵を消す（deleteの代わり）

    virtual void Update(double elapsed_time) = 0;
    int GetHP() const { return m_hp; }
    // 共通の便利関数
    virtual int GetTypeIndex() const = 0;
    bool IsActive() const { return m_isActive; }
    virtual bool IsDestroy() const { return m_hp <= 0; }
    virtual Sphere GetCollision() const { return { m_position, 0.5f }; }
    virtual void Damage(int damage) { m_hp -= damage; }

    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
};

#endif // !ENEMY_H
