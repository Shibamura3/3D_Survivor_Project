/*
	プレイヤーの制御：player.h

	2025/10/51	hibiki sakuma
*/


#ifndef PLAYER_H
#define PLAYER_H

#include <DirectXMath.h>
#include "collision.h"
#include "resource_manager.h"

// アップグレードの選択肢
enum class UpgradeOption {
    MaxHP, // 最大HP上昇
    MoveSpeed, // 移動速度上昇
    ShotInterval // 弾の発射間隔短縮
};

class Player {
private:
    // --- 物理・移動関連 ---
    DirectX::XMFLOAT3 m_position{};
    DirectX::XMFLOAT3 m_velocity{};
    DirectX::XMFLOAT3 m_front{};
    bool m_isJump{};
    bool m_isGround{};

    // --- ステータス関連 ---
    float m_maxHP{};
    float m_nowHP{};
    float m_exp{};
    float m_nextExp{};
    int   m_level{};

    // --- 追加されたステータス倍率 ---
    float m_moveSpeedMag{};    // 移動速度倍率（初期1.0）
    float m_shotInterval{};    // 発射間隔（初期0.5）
    double m_rapidTimer{};     // 射撃間隔計算用
    bool m_isLevelUpPending{}; // 3択待ちフラグ

    // --- リソース ---
    Model_ID m_modelId{};

public:
    Player();
    ~Player();

    void Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, Model_ID modelId);
    void Finalize();
    void Update(double elapsed_time);
    void Draw();

    // ゲッター類
    // 当たり判定
    Capsule GetCapsule() const; 
    Capsule GetCapsuleAt(const DirectX::XMVECTOR& position) const;
    // 座標
    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
    const DirectX::XMFLOAT3& GetFront() const { return m_front; }

    // ステータス関連
    float GetHp() const { return m_nowHP; }
    float GetMaxHp() const { return m_maxHP; }
    float GetExp() const { return m_exp; }
    float GetNextExp() const { return m_nextExp; }
    int   GetLevel() const { return m_level; }
    bool  IsLevelUpPending() const { return m_isLevelUpPending; }

    // 加算減算処理
    void AddExp(float exp);
    void Damage(float damage);
    void ApplyUpgrade(UpgradeOption option); // 3択適用
};

// どこからでもプレイヤーの情報を確認できる
Player* GetPlayer();

#endif // !Player_H
