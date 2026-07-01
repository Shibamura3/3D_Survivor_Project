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

private: // 内部関数
    // フレーム初期化
    void BeginFrame();
    // ジャンプ入力
    void HandleJumpInput();
    // 重力適用
    void ApplyGravity(double elapsed_time);
    // 移動計算
    DirectX::XMVECTOR CalculateMoveDirection() const;
    // 向き補正（スムーズ回転）
    void UpdateFacing(const DirectX::XMVECTOR& direction, double elapsed_time);
    // 加速処理
    void ApplyMoveAcceleration(const DirectX::XMVECTOR& direction, double elapsed_time);
    // 移動＋衝突のまとめ
    void ResolveMovementAndCollision(double elapsed_time);
    // めり込み補正
    void ResolvePenetration(DirectX::XMVECTOR& position, DirectX::XMVECTOR& velocity);
    // スイープ処理
    void ResolveSweptCollision(DirectX::XMVECTOR& position, DirectX::XMVECTOR& velocity, double elapsed_time);
    // 接地処理
    void ResolveGroundContact(DirectX::XMVECTOR& position, DirectX::XMVECTOR& velocity);
    // 摩擦処理
    void ApplyFriction(double elapsed_time);
    // 射撃処理
    void UpdateShooting(double elapsed_time);
    // 場外処理
    void HandleOutOfBounds();

public:
    Player();
    ~Player();

    void Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, Model_ID modelId);
    void Finalize();
    void Update(double elapsed_time);
    void Draw();

    // ゲッター類
    bool IsGround() const { return m_isGround; }
    // 当たり判定
    Capsule GetCapsule() const;  // 衝突用
    Capsule GetCapsuleAt(const DirectX::XMVECTOR& position) const; // スイープ用

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
