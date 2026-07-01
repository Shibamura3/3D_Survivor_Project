/*
	プレイヤーの制御：player.cpp

	2025/10/51	hibiki sakuma
*/

#include "player.h"
#include "player_camera.h"
#include "Key_logger.h"
#include "pad_logger.h"
#include "map.h"
#include "bullet.h"
#include "light.h"
#include "camera.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

// 定数宣言
constexpr float PLAYER_RADIUS = 0.5f;
static constexpr float PLAYER_HEIGHT = 2.0f;
static constexpr float PLAYER_JUMP = 25.0f;

static constexpr float SKIN = 0.002f;
static constexpr float GROUND_Y = 0.95f;
static constexpr int   MAX_SWEEP = 3;

// レベルアップ関係
static constexpr float MAX_HP_UP = 30.0f;
static constexpr float MOVE_SPEED_MAG_UP = 0.2f;
static constexpr float SHOT_INTERVAL_SCALE = 0.8f;

// 地面スナップ用
static constexpr float GROUND_SNAP_EPSILON = 0.0f;
static constexpr float CAPSULE_FOOT_ADJUST = 0.3f;
static constexpr float PLAYER_GROUND_OFFSET = PLAYER_HEIGHT * 0.5f - CAPSULE_FOOT_ADJUST;

// 地面判定用
static constexpr float FLOOR_NORMAL_Y = 0.95f;
static constexpr float GROUND_PROBE_DISTANCE = 0.08f;

// わずかに浮かせて刺さりを防ぐ
static constexpr float GROUND_CONTACT_BIAS = 0.01f;

// 本当に「落下中」とみなす閾値
static constexpr float LANDING_Y_EPSILON = 0.001f;

// プレイヤー移動
static constexpr float PLAYER_ACCELERATION = 500.0f;
static constexpr float GROUND_FRICTION = 6.0f;
static constexpr float AIR_FRICTION = 4.0f;
static constexpr float GRAVITY = -98.0f;
static constexpr float BASE_MAX_MOVE_SPEED_GROUND = 8.0f;
static constexpr float BASE_MAX_MOVE_SPEED_AIR = 6.0f;

// 場外ペナルティ
static constexpr float FALL_OUT_Y = -1.0f;
static constexpr float FALL_DAMAGE = 10.0f;
static constexpr float RESPAWN_Y = 1.0f;

// 射撃
static constexpr float BULLET_SPEED = 10.0f;

// 外部からは見えない、このファイルだけの静的変数
static Player* g_CurrentInstance = nullptr;

Player::Player() {
    g_CurrentInstance = this; // 生成されたときに自分を登録
}

Player::~Player() {
    if (g_CurrentInstance == this) {
        g_CurrentInstance = nullptr; // 破棄されたら解除
    }
}

// 実体（中身）の実装
Player* GetPlayer() {
    return g_CurrentInstance;
}

void Player::Initialize(const XMFLOAT3& position, const XMFLOAT3& front, Model_ID modelId) {
    m_position = position;
    m_velocity = { 0.0f, 0.0f, 0.0f };
    XMStoreFloat3(&m_front, XMVector3Normalize(XMLoadFloat3(&front)));

    m_modelId = modelId;
    m_isJump = false;
    m_isGround = false;

    // 初期ステータス
    m_maxHP = 100.0f;
    m_nowHP = m_maxHP;
    m_exp = 0.0f;
    m_nextExp = 10.0f;
    m_level = 1;

    m_moveSpeedMag = 1.0f;    // 初期倍率
    m_shotInterval = 0.5f;    // 初期間隔
    m_rapidTimer = 0.0;
}

void Player::Finalize() {
   
}

// プレイヤー更新
// 入力 → 物理 → 衝突 → 摩擦 の順に処理
void Player::Update(double elapsed_time) {

    BeginFrame();

    HandleJumpInput();
    ApplyGravity(elapsed_time);

    XMVECTOR direction = CalculateMoveDirection();
    UpdateFacing(direction, elapsed_time);
    ApplyMoveAcceleration(direction, elapsed_time);

    ResolveMovementAndCollision(elapsed_time);
    ApplyFriction(elapsed_time);

    UpdateShooting(elapsed_time);
    HandleOutOfBounds();

}

void Player::Draw() {
    float angle = -atan2f(m_front.z, m_front.x) + XMConvertToRadians(270);
    XMMATRIX world = XMMatrixRotationY(angle) * XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

    ModelDraw(Resouce_Manager_GetModelId(m_modelId), world);

}

// 内部関数
void Player::BeginFrame() {
    m_isGround = false;
}

void Player::HandleJumpInput() {
    bool isJumpTriggered =
        PadLogger_IsConnected()
        ? PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_A)
        : KeyLogger_IsTrigger(KK_SPACE);

    if (isJumpTriggered && !m_isJump) {
        XMVECTOR velocity = XMLoadFloat3(&m_velocity);
        velocity += XMVectorSet(0.0f, PLAYER_JUMP, 0.0f, 0.0f);
        XMStoreFloat3(&m_velocity, velocity);
        m_isJump = true;
    }
}

void Player::ApplyGravity(double elapsed_time) {
    XMVECTOR velocity = XMLoadFloat3(&m_velocity);
    velocity += XMVectorSet(0.0f, GRAVITY, 0.0f, 0.0f) * (float)elapsed_time;
    XMStoreFloat3(&m_velocity, velocity);
}

XMVECTOR Player::CalculateMoveDirection() const {
    XMVECTOR direction = XMVectorZero();

    XMVECTOR front = XMLoadFloat3(&Player_Camera_GetFront()) * XMVECTOR { 1.0f, 0.0f, 1.0f, 0.0f };
    front = XMVector3Normalize(front);

    if (PadLogger_IsConnected()) {
        XMFLOAT2 pad = PadLogger_GetLeftThumbStick(0);
        if (fabsf(pad.x) > 0.0f || fabsf(pad.y) > 0.0f) {
            XMVECTOR right = XMVector3Cross({ 0.0f,1.0f,0.0f,0.0f }, front);
            direction = (front * pad.y) + (right * pad.x);
        }
    } else {
        if (KeyLogger_IsPressed(KK_W)) direction += front;
        if (KeyLogger_IsPressed(KK_S)) direction -= front;
        if (KeyLogger_IsPressed(KK_D)) direction += XMVector3Cross({ 0.0f,1.0f,0.0f,0.0f }, front);
        if (KeyLogger_IsPressed(KK_A)) direction -= XMVector3Cross({ 0.0f,1.0f,0.0f,0.0f }, front);
    }

    if (XMVectorGetX(XMVector3LengthSq(direction)) > 0.0f) {
        direction = XMVector3Normalize(direction);
    }

    return direction;
}

void Player::UpdateFacing(const XMVECTOR& direction, double elapsed_time) {
    if (XMVectorGetX(XMVector3LengthSq(direction)) <= 0.0f)
        return;

    XMVECTOR currentFront = XMLoadFloat3(&m_front);

    float dot = XMVectorGetX(XMVector3Dot(currentFront, direction));
    dot = std::max(-1.0f, std::min(1.0f, dot));

    float angle = acosf(dot);
    const float ROTATION_SPEED = XM_2PI * 4.0f * (float)elapsed_time;

    if (angle < ROTATION_SPEED) {
        currentFront = direction;
    } else {
         XMMATRIX rot = XMMatrixIdentity();

        if (XMVectorGetY(XMVector3Cross(currentFront, direction)) < 0.0f) {
            rot = XMMatrixRotationY(-ROTATION_SPEED);
        } else {
            rot = XMMatrixRotationY(ROTATION_SPEED);
        }
        currentFront = XMVector3TransformNormal(currentFront, rot);
        currentFront = XMVector3Normalize(currentFront);
    }

    XMStoreFloat3(&m_front, currentFront);
}

void Player::ApplyMoveAcceleration(const XMVECTOR& direction, double elapsed_time) {
    if (XMVectorGetX(XMVector3LengthSq(direction)) <= 0.0f)
        return;

    XMVECTOR velocity = XMLoadFloat3(&m_velocity);

    // 進行方向ベクトルそのものを使う
    velocity += direction * (float)(PLAYER_ACCELERATION * elapsed_time * m_moveSpeedMag);

    // 水平方向だけ最大速度を制限
    float vx = XMVectorGetX(velocity);
    float vy = XMVectorGetY(velocity);
    float vz = XMVectorGetZ(velocity);

    float horizontalSpeedSq = vx * vx + vz * vz;

    float maxSpeed = m_isGround
        ? BASE_MAX_MOVE_SPEED_GROUND * m_moveSpeedMag
        : BASE_MAX_MOVE_SPEED_AIR * m_moveSpeedMag;

    if (horizontalSpeedSq > maxSpeed * maxSpeed) {
        float speed = sqrtf(horizontalSpeedSq);
        float scale = maxSpeed / speed;

        vx *= scale;
        vz *= scale;

    }

    velocity = XMVectorSet(vx, vy, vz, 0.0f);
    XMStoreFloat3(&m_velocity, velocity);

}

void Player::ResolveMovementAndCollision(double elapsed_time) {
    XMVECTOR position = XMLoadFloat3(&m_position);
    XMVECTOR velocity = XMLoadFloat3(&m_velocity);

    ResolvePenetration(position, velocity);
    ResolveSweptCollision(position, velocity, elapsed_time);
    ResolveGroundContact(position, velocity);

    XMStoreFloat3(&m_position, position);
    XMStoreFloat3(&m_velocity, velocity);
}

void Player::ResolvePenetration(XMVECTOR& position, XMVECTOR& velocity) {
    for (int loop = 0; loop < 3; loop++) {

        bool anyHit = false;
        Capsule cap = GetCapsuleAt(position);

        for (int i = 0; i < Map_GetObjectsCount(); i++) {
            AABB box = Map_GetObject(i)->Aabb_collision;

            Hit hit = Collision_IsHitCapsulevsAABB(cap, box);
            if (!hit.isHit) continue;

            // 床はここでは処理しない
            if (fabs(hit.normal.y) > GROUND_Y) {
                continue;
            }

            // 壁だけ押し出す
            position += XMLoadFloat3(&hit.normal) * (hit.depth + SKIN);
            anyHit = true;
        }

        if (!anyHit) break;
    }
}

void Player::ResolveSweptCollision(XMVECTOR& position, XMVECTOR& velocity, double elapsed_time) {
    XMVECTOR remaining = velocity * (float)elapsed_time;

    for (int iter = 0; iter < MAX_SWEEP; iter++) {

        if (XMVectorGetX(XMVector3LengthSq(remaining)) < 1e-6f)
            break;

        XMVECTOR oldPos = position;
        XMVECTOR newPos = position + remaining;

        Capsule oldCap = GetCapsuleAt(oldPos);
        Capsule newCap = GetCapsuleAt(newPos);

        float hitT = 1.0f;
        XMFLOAT3 hitNormal{};
        bool hitAny = false;

        for (int i = 0; i < Map_GetObjectsCount(); i++) {
            auto* obj = Map_GetObject(i);
            AABB box = obj->Aabb_collision;

            float t;
            XMFLOAT3 n;

            if (Collision_SweptCapsuleVsAABB(oldCap, newCap, box, t, n)) {

                float velocityY = XMVectorGetY(velocity);
                float playerFootY = XMVectorGetY(oldPos) - PLAYER_GROUND_OFFSET;
                float floorY = box.max.y;
                if (obj->isOneWay) {
                    if (n.y < -FLOOR_NORMAL_Y) continue;

                    if (n.y > FLOOR_NORMAL_Y) {
                        // 下から侵入 
                        if (playerFootY < floorY - 0.05f && velocityY >= 0.0f) {
                            continue;
                        }
                    }
                }
                if (t >= 0.0f && t < hitT) {
                    hitT = t;
                    hitNormal = n;
                    hitAny = true;
                }
            }
        }

        if (!hitAny) {
            position = newPos;
            break;
        }

        XMVECTOR n = XMLoadFloat3(&hitNormal);

        // 接触点まで移動
        position = oldPos + remaining * hitT + n * SKIN;

        float remainRate = 1.0f - hitT;
        remaining *= remainRate;

        // 床判定
        if (hitNormal.y > FLOOR_NORMAL_Y) {
            float velocityY = XMVectorGetY(velocity);

            // 下向きのときだけ乗る
            if (velocityY <= 0.0f) {
                // 接地状態
                m_isGround = true;
                m_isJump = false;

                // Y速度削除
                velocity = XMVectorSetY(velocity, 0.0f);

                // Y方向の移動を止める
                remaining = XMVectorSet(
                    XMVectorGetX(remaining),
                    0,
                    XMVectorGetZ(remaining),
                    0.0f
                );
            } else {
                // 上向き → すり抜ける
                continue;
            }
        } else {
            // 壁スライド
            float vn = XMVectorGetX(XMVector3Dot(remaining, n));

            if (vn < 0.0f) {
                remaining -= n * vn;
            }
        }
    }
    velocity = remaining / (float)elapsed_time;
}

void Player::ResolveGroundContact(DirectX::XMVECTOR& position, DirectX::XMVECTOR& velocity) {

    // 落下中または静止中のみ接地判定
    if (XMVectorGetY(velocity) > 0.0f) {
        return;
    }

    float px = XMVectorGetX(position);
    float py = XMVectorGetY(position);
    float pz = XMVectorGetZ(position);

    // カプセルの足先
    float footY = py - PLAYER_GROUND_OFFSET;

    bool foundGround = false;
    float bestGroundY = -FLT_MAX;

    for (int i = 0; i < Map_GetObjectsCount(); i++) {
        AABB box = Map_GetObject(i)->Aabb_collision;

        // XZに半径分の余裕を持って床上にいるか確認
        bool insideXZ =
            px >= box.min.x - PLAYER_RADIUS &&
            px <= box.max.x + PLAYER_RADIUS &&
            pz >= box.min.z - PLAYER_RADIUS &&
            pz <= box.max.z + PLAYER_RADIUS;

        if (!insideXZ) {
            continue;
        }

        float distToTop = footY - box.max.y;

        // 床のすぐ近くにいるときだけ接地させる
        if (XMVectorGetY(velocity) <= 0.0f &&
            distToTop >= -SKIN &&
            distToTop <= GROUND_PROBE_DISTANCE) {
            if (box.max.y > bestGroundY) {
                bestGroundY = box.max.y;
                foundGround = true;
            }
        }
    }

    if (foundGround) {
        float snappedY = bestGroundY + PLAYER_GROUND_OFFSET + GROUND_CONTACT_BIAS;

        position = XMVectorSet(
            XMVectorGetX(position),
            snappedY,
            XMVectorGetZ(position),
            0.0f
        );

        velocity = XMVectorSetY(velocity, 0.0f);
        m_isGround = true;
        m_isJump = false;
    }

}

void Player::ApplyFriction(double elapsed_time) {
    XMVECTOR velocity = XMLoadFloat3(&m_velocity);

    float vx = XMVectorGetX(velocity);
    float vy = XMVectorGetY(velocity);
    float vz = XMVectorGetZ(velocity);

    float friction = m_isGround ? GROUND_FRICTION : AIR_FRICTION;

    vx += -vx * (friction * (float)elapsed_time);
    vz += -vz * (friction * (float)elapsed_time);

    velocity = XMVectorSet(vx, vy, vz, 0.0f);
    XMStoreFloat3(&m_velocity, velocity);
}

void Player::UpdateShooting(double elapsed_time) {
    bool isShootTriggered =
        PadLogger_IsConnected()
        ? PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_B)
        : KeyLogger_IsTrigger(KK_F);

    if (!isShootTriggered) {
        m_rapidTimer -= elapsed_time;
        return;
    }

    if (m_rapidTimer > 0.0) {
        return;
    }

    XMFLOAT3 bulletVel;
    XMStoreFloat3(&bulletVel, XMLoadFloat3(&m_front) * BULLET_SPEED);
    Bullet_Create(m_position, bulletVel);

    m_rapidTimer = m_shotInterval;
}

void Player::HandleOutOfBounds() {
    if (m_position.y < FALL_OUT_Y) {
        m_position = { 0.0f, RESPAWN_Y, 0.0f };
        m_nowHP -= FALL_DAMAGE;
    }
}

// 3択適用の実装
void Player::ApplyUpgrade(UpgradeOption option) {
    switch (option) {
    case UpgradeOption::MaxHP:
        m_maxHP += MAX_HP_UP;
        break;
    case UpgradeOption::MoveSpeed:
        m_moveSpeedMag += MOVE_SPEED_MAG_UP;
        break;
    case UpgradeOption::ShotInterval:
		m_shotInterval *= SHOT_INTERVAL_SCALE;
        break;
    }
	m_nowHP = m_maxHP; // 回復
    m_isLevelUpPending = false; // 3択終了
}

void Player::AddExp(float exp) {
    m_exp += exp;
    if (m_exp >= m_nextExp) {
        m_isLevelUpPending = true; // 3択フラグを立てる
        m_level++;
        m_exp -= m_nextExp;
        m_nextExp += 10.0f;
    }
}

void Player::Damage(float damage){
    m_nowHP -= damage;
    if (m_nowHP <= 0.0f) {
        m_nowHP = 0.0f;
    }
}

Capsule Player::GetCapsule() const {
    Capsule c{};
    c.radius = PLAYER_RADIUS;

    c.start = {
        m_position.x,
        m_position.y + PLAYER_RADIUS - PLAYER_HEIGHT * 0.5f + CAPSULE_FOOT_ADJUST,
        m_position.z
    };

    c.end = {
        m_position.x,
        m_position.y + PLAYER_HEIGHT - PLAYER_RADIUS,
        m_position.z
    };


    return c;
}

Capsule Player::GetCapsuleAt(const DirectX::XMVECTOR& position) const{
    Capsule c{};
    c.radius = PLAYER_RADIUS;

    // 足元
    c.start = {
        XMVectorGetX(position),
        XMVectorGetY(position) + PLAYER_RADIUS - PLAYER_HEIGHT * 0.5f + CAPSULE_FOOT_ADJUST,
        XMVectorGetZ(position)
    };

	// 頭
    c.end = {
           XMVectorGetX(position),
           XMVectorGetY(position) + PLAYER_HEIGHT - PLAYER_RADIUS,
           XMVectorGetZ(position)
    };

    return c;

}