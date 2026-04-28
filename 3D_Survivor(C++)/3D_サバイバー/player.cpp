/*
	プレイヤーの制御：player.cpp

	2025/10/51	hibiki sakuma
*/

#include "player.h"
#include "player_camera.h"
#include "key_logger.h"
#include "pad_logger.h"
#include "map.h"
#include "bullet.h"
#include "light.h"
#include "camera.h"

using namespace DirectX;

// 定数宣言
static constexpr float PLAYER_RADIUS = 0.5f;
static constexpr float PLAYER_HEIGHT = 2.0f;
static constexpr float PLAYER_JUMP = 30.0f;
static constexpr float SKIN = 0.002f;
static constexpr float GROUND_Y = 0.7f;
static constexpr int   MAX_SWEEP = 3;
static constexpr float GROUND_FRICTION = 10.0f;
static constexpr float AIR_FRICTION = 5.0f;
static constexpr float GRAVITY = -98.0f;
// レベルアップ関係の定数
static constexpr float MAX_HP_UP = 30.0f;
static constexpr float MOVE_SPEED_MAG_UP = 0.2f; // 20%アップ
static constexpr float SHOTINTERVAL = 0.8; // 20%短縮

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

void Player::Update(double elapsed_time) {
    m_isGround = false;
    XMVECTOR position = XMLoadFloat3(&m_position);
    XMVECTOR velocity = XMLoadFloat3(&m_velocity);

    //	// 入力モードの取得
	InputMode mode;
	if (PadLogger_IsConnected()) {
		mode = InputMode::Controller;
	} else {
		mode = InputMode::KeyboardMouse;
	}


    // --- 入力処理 (元々のロジックをそのままコピー) ---
    bool isJumpTriggered = PadLogger_IsConnected() ? PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_A) : KeyLogger_IsTrigger(KK_SPACE);
    bool isShootTriggered = PadLogger_IsConnected() ? PadLogger_IsTrigger(0, SDL_CONTROLLER_BUTTON_B) : KeyLogger_IsTrigger(KK_F);

    if (isJumpTriggered && !m_isJump) {
        velocity += { 0.0f, PLAYER_JUMP, 0.0f };
        m_isJump = true;
    }

    velocity += XMVectorSet(0, GRAVITY, 0, 0) * (float)elapsed_time;

    // --- 移動処理 (m_moveSpeedMag を適用) ---
    XMVECTOR direction{};
    XMVECTOR cameraFront = XMLoadFloat3(&Player_Camera_GetFront()) * XMVECTOR { 1.0f, 0.0f, 1.0f };
    cameraFront = XMVector3Normalize(cameraFront);

    //	// プレイヤーの移動
	XMVECTOR front = XMLoadFloat3(&Player_Camera_GetFront()) * XMVECTOR { 1.0f, 0.0f, 1.0f };
	front = XMVector3Normalize(front);
	if (mode == InputMode::Controller) {
		// パッド移動：カメラの向きに合わせてスティック方向を計算
		XMFLOAT2 pad = PadLogger_GetLeftThumbStick(0);
		if (abs(pad.x) > 0.0f || abs(pad.y) > 0.0f) {
			XMVECTOR right = XMVector3Cross({ 0.0f, 1.0f, 0.0f }, front);
			direction = (front * pad.y) + (right * pad.x); // SDLのY軸反転を考慮
		}
	} else {
		// キーボード移動
		if (KeyLogger_IsPressed(KK_W)) {
			direction += front;
		}
		if (KeyLogger_IsPressed(KK_S)) {
			direction -= front;
		}
		if (KeyLogger_IsPressed(KK_D)) {
			direction += XMVector3Cross({ 0.0f,1.0f,0.0f }, front);
		}
		if (KeyLogger_IsPressed(KK_A)) {
			direction -= XMVector3Cross({ 0.0f,1.0f,0.0f }, front);
		}
	}

    if (XMVectorGetX(XMVector3LengthSq(direction)) > 0.0f) {
       direction = XMVector3Normalize(direction); // 向きたい方向ベクトル

		// 2つのベクトルのなす角は
		float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_front), direction));
		float angle = acosf(std::max(-1.0f, std::min(1.0f, dot))); // クランプしてエラー防止

		// 回転速度
		const float ROTATION_SPEED = XM_2PI * 4.0f * (float)elapsed_time;

		if (angle < ROTATION_SPEED) {
			front = direction;
		}
		else {
			// 向きたい方向が右回りか左回りかを判定→外積で求める
			XMMATRIX rote = XMMatrixIdentity();
			if (XMVectorGetY(XMVector3Cross(XMLoadFloat3(&m_front), direction)) < 0.0f) {
				rote = XMMatrixRotationY(-ROTATION_SPEED); // 回転速度が遅いと車みたいになる
			}
			else {
				rote = XMMatrixRotationY(ROTATION_SPEED);
			}

			front = XMVector3TransformNormal(XMLoadFloat3(&m_front), rote);
		}
		velocity += front * (float)(2500.0 / 50.0/*重さ*/ * elapsed_time); // 現在の方向ベクトル
		XMStoreFloat3(&m_front, front);

        velocity += XMLoadFloat3(&m_front) * (float)(2500.0 / 50.0 * elapsed_time * m_moveSpeedMag);
    }

    // --- 衝突判定 (Swept Collisionもそのままここに入れます) ---
    //	// 初期めり込みの解消
	for (int loop = 0; loop < 3; loop++) {
		bool anyHit = false;
		Capsule cap = GetCapsuleAt(position);

		for (int i = 0; i < Map_GetObjectsCount(); i++) {
			AABB box = Map_GetObject(i)->Aabb_collision;
			Hit hit = Collision_IsHitCapsulevsAABB(cap, box);

			if (!hit.isHit) continue;

			position += XMLoadFloat3(&hit.normal) * (hit.depth + SKIN);
			anyHit = true;

			if (hit.normal.y > GROUND_Y) {
				m_isGround = true;
				m_isJump = false;
				velocity = XMVectorSetY(velocity, 0.0f);
			}
		}
		if (!anyHit) break;
	}

	// Swept Collision（最大3回）
	XMVECTOR remaining = velocity * (float)elapsed_time;

	for (int iter = 0; iter < MAX_SWEEP; iter++) {

		if (XMVectorGetX(XMVector3LengthSq(remaining)) < 1e-6f) break;

		XMVECTOR oldPos = position;
		XMVECTOR newPos = position + remaining;

		Capsule oldCap = GetCapsuleAt(oldPos);
		Capsule newCap = GetCapsuleAt(newPos);

		float hitT = 1.0f;
		XMFLOAT3 hitNormal{};
		bool hitAny = false;

		for (int i = 0; i < Map_GetObjectsCount(); i++) {
			AABB box = Map_GetObject(i)->Aabb_collision;
			float t;
			XMFLOAT3 n;

			if (Collision_SweptCapsuleVsAABB(oldCap, newCap, box, t, n)) {
				if (t >= 0 && t < hitT) {
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

		// 衝突点まで移動＋SKIN押し出し
		position = oldPos + remaining * hitT + n * SKIN;

		// 速度を面に沿わせる（スライド）
		float vn = XMVectorGetX(XMVector3Dot(remaining, n));
		if (vn < 0.0f) remaining -= n * vn;

		// 接地
		if (hitNormal.y > GROUND_Y) {
			m_isGround = true;
			m_isJump = false;
			remaining = XMVectorSetY(remaining, 0.0f);
		}
	}
	velocity = remaining / (float)elapsed_time;
	
	// 抵抗…移動中の滑り
	if (m_isGround) {
		velocity += -velocity * (GROUND_FRICTION * (float)elapsed_time);
	} else {
		velocity += -velocity * (AIR_FRICTION * (float)elapsed_time);
	}

    // --- 結果を戻す ---
    XMStoreFloat3(&m_position, position);
    XMStoreFloat3(&m_velocity, velocity);

    // --- 射撃処理 (m_shotInterval を使用) ---
    if (isShootTriggered) {
        if (m_rapidTimer <= 0.0) {
            XMFLOAT3 b_vel;
            XMStoreFloat3(&b_vel, XMLoadFloat3(&m_front) * 10.0f);
            Bullet_Create(m_position, b_vel);
            m_rapidTimer = m_shotInterval; // ここを変数にする
        }
    } else {
        m_rapidTimer -= elapsed_time;
    }

	// 場外処理
	if (m_position.y < -1.0f) {
		m_position = { 0.0f,1.0f,0.0f };
		m_nowHP -= 10.0f; // ペナルティ
	}
}

void Player::Draw() {
    float angle = -atan2f(m_front.z, m_front.x) + XMConvertToRadians(270);
    XMMATRIX world = XMMatrixRotationY(angle) * XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

    // ResourceManager からモデルを取得して描画
    ModelDraw(Resouce_Manager_GetModelId(m_modelId), world);
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
		m_shotInterval *= SHOTINTERVAL;
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

    // 足元 (m_position を使う)
    c.start = {
        m_position.x,
        m_position.y + PLAYER_RADIUS - PLAYER_HEIGHT * 0.5f + 0.3f,
        m_position.z
    };

    // 頭
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
		XMVectorGetY(position) + PLAYER_RADIUS - PLAYER_HEIGHT * 0.5f + 0.3f,
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