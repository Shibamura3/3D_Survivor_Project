/*
	敵全体の制御：enemymanager.h

	2026/01/10	hibiki sakuma
*/
#include "enemymanager.h"
#include "player.h"
#include "map.h"
#include "model.h"
#include "meshfield.h"
#include <vector>
#include <algorithm>
#include "achievementmanager.h"
#include <DirectXCollision.h> // 視錐台判定に必要
#include "player_camera.h"
using namespace DirectX;
// 定数宣言
// 敵数
static constexpr int MAX_T1 = 500;
static constexpr int MAX_T2 = 500;
static constexpr int MAX_T3 = 500;
static constexpr int TOTAL_MAX = MAX_T1 + MAX_T2 + MAX_T3;

// 1種類ごとのインスタンス描画上限
static constexpr int MAX_INSTANCES = 500;

// 出現数制御
static constexpr double COUNTUP_TIME = 20.0;
static constexpr int    START_ENEMY_LIMIT = 10;
static constexpr double ENEMY_LIMIT_UP = 5.0;

// スポーン位置
static constexpr float FLOOR_MIDDLE = 10.0f;
static constexpr float FLOOR_TOP = 20.0f;
static constexpr float CAMERA_FRONT_DISTANCE = 15.0f;
static constexpr float CAMERA_FAR_DISTANCE = 20.0f;
static constexpr float OFFSET_Y = 0.5f;

// デスポーン
static constexpr float DESPAWN_DISTANCE = 50.0f;

// 抽選率
static constexpr int SPAWN_T1_PERCENT = 30;
static constexpr int SPAWN_T2_PERCENT = 80; // 30 + 50

// マップ端マージン
static constexpr float MAP_MARGIN = 5.0f;

// 無効スポーン用Y
static constexpr float INVALID_SPAWN_Y = -999.0f;

// 内部変数（staticで隠蔽）
static Enemy_T1 g_PoolT1[MAX_T1];
static Enemy_T2 g_PoolT2[MAX_T2];
static Enemy_T3 g_PoolT3[MAX_T3];

static double g_GameTime = 0.0;
static int    g_CurrentMaxEnemies = START_ENEMY_LIMIT;

// 内部関数
namespace
{
    template <typename T, size_t N>
    void DeactivateAll(T(&pool)[N]) {
        for (auto& e : pool){
            e.Deactivate();
        }
    }

    BoundingFrustum CreateCameraFrustum() {
        XMMATRIX view = XMLoadFloat4x4(&Player_Camera_GetViewMatrix());
        XMMATRIX proj = XMLoadFloat4x4(&Player_Camera_GetPerspectiveMatrix());

        BoundingFrustum frustum(proj);
        XMMATRIX invView = XMMatrixInverse(nullptr, view);
        frustum.Transform(frustum, invView);

        return frustum;
    }

    template <typename T, size_t N>
    int UpdatePool(T(&pool)[N], double elapsed_time, const BoundingFrustum& frustum, const XMVECTOR& playerPosV, const char* achievementKey) {
        int activeCount = 0;

        for (auto& enemy : pool) {
            if (!enemy.IsActive()) {
                continue;
            }

            // デスポーン判定
            XMVECTOR posV = XMLoadFloat3(&enemy.GetPosition());
            float distSq = XMVectorGetX(XMVector3LengthSq(posV - playerPosV));

            if (distSq > (DESPAWN_DISTANCE * DESPAWN_DISTANCE)) {
                enemy.Deactivate();
                continue;
            }

            // 通常更新
            enemy.Update(elapsed_time);

            // 撃破判定
            if (enemy.GetHP() <= 0) {
                AchievementManager::Instance().OnNotify(achievementKey, 1);
                enemy.Deactivate();
                continue;
            }
            ++activeCount;
        }
        return activeCount;
    }

    template <typename T, size_t N>
    bool ActivateFirstInactive(T(&pool)[N], const XMFLOAT3& spawnPos) {
        for (auto& enemy : pool) {
            if (!enemy.IsActive()) {
                enemy.Activate(spawnPos);
                return true;
            }
        }
        return false;
    }

    template <typename T, size_t N>
    void DrawPool(T(&pool)[N], const BoundingFrustum& frustum, XMMATRIX(&matrices)[MAX_INSTANCES]) {
        int count = 0;
        MODEL* pModel = nullptr;

        for (auto& enemy : pool) {
            if (!enemy.IsActive()) {
                continue;
            }

            XMVECTOR pos = XMLoadFloat3(&enemy.GetPosition());
            if (frustum.Contains(pos) == DISJOINT) {
                continue;
            }

            if (!pModel) {
                pModel = enemy.GetModel();
            }

            if (count < MAX_INSTANCES) {
                matrices[count++] = XMMatrixTranspose(enemy.GetWorldMatrix());
            }
        }

        if (count > 0 && pModel) {
            ModelDrawInstanced(pModel, matrices, count);
        }
    }
}


void EnemyManager::Initialize() {
    Enemy_T1::LoadModel();
    Enemy_T2::LoadModel(); 
    Enemy_T3::LoadModel();

    DeactivateAll(g_PoolT1);
    DeactivateAll(g_PoolT2);
    DeactivateAll(g_PoolT3);

    g_GameTime = 0.0;
    g_CurrentMaxEnemies = START_ENEMY_LIMIT;
}

void EnemyManager::Finalize() {
    // モデルリリースはresource_managerで一括で行う
    // EnemyManagerでは使用した変数のみをNULL
    Enemy_T1::UnloadModel();
    Enemy_T2::UnloadModel();
    Enemy_T3::UnloadModel();
}

void EnemyManager::Update(double elapsed_time) {
    g_GameTime += elapsed_time;

    int baseCount = START_ENEMY_LIMIT + (int)(g_GameTime / COUNTUP_TIME * ENEMY_LIMIT_UP);

    float playerY = GetPlayer()->GetPosition().y;

    float heightFactor = 1.0f;

    if (playerY > FLOOR_TOP) {
        heightFactor = 2.0f; // 上：2倍
    } else if (playerY > FLOOR_MIDDLE) {
        heightFactor = 1.5f; // 中：1.5倍
    } else {
        heightFactor = 1.0f; // 下：変化なし
    }

    // 同時存在上限の更新
    g_CurrentMaxEnemies = (int)(baseCount * heightFactor);    
    if (g_CurrentMaxEnemies > TOTAL_MAX) g_CurrentMaxEnemies = TOTAL_MAX;

    // カメラの視錐台（見える空間）を作成
    BoundingFrustum frustum = CreateCameraFrustum();
    XMVECTOR playerPosV = XMLoadFloat3(&GetPlayer()->GetPosition());

    // 各プールの更新
    int activeCount = 0;
    activeCount += UpdatePool(g_PoolT1, elapsed_time, frustum, playerPosV, "ENEMY_KILLED_T1");
    activeCount += UpdatePool(g_PoolT2, elapsed_time, frustum, playerPosV, "ENEMY_KILLED_T2");
    activeCount += UpdatePool(g_PoolT3, elapsed_time, frustum, playerPosV, "ENEMY_KILLED_T3");

    // 足りない分だけ補充
    int need = g_CurrentMaxEnemies - activeCount;
    for (int i = 0; i < need; i++) {
        XMFLOAT3 spawnPos = CalculateSpawnPosition();
        if (spawnPos.y < 0.0f) continue;
        

        int dice = rand() % 100;
        if (dice < SPAWN_T1_PERCENT) {
            ActivateFirstInactive(g_PoolT1, spawnPos);
        } else if (dice < SPAWN_T2_PERCENT) {
            ActivateFirstInactive(g_PoolT2, spawnPos);
        } else {
            ActivateFirstInactive(g_PoolT3, spawnPos);
        }
    }
}

void EnemyManager::Draw() {
    static XMMATRIX matrices[MAX_INSTANCES];

    BoundingFrustum frustum = CreateCameraFrustum();

    DrawPool(g_PoolT1, frustum, matrices);
    DrawPool(g_PoolT2, frustum, matrices);
    DrawPool(g_PoolT3, frustum, matrices);
}

int EnemyManager::GetMaxCount() {
    return TOTAL_MAX;
}

Enemy* EnemyManager::GetEnemy(int index){
    if (index < MAX_T1) {
        return &g_PoolT1[index];
    }

    index -= MAX_T1;
    if (index < MAX_T2) {
        return &g_PoolT2[index];
    }

    index -= MAX_T2;
    if (index < MAX_T3) {
        return &g_PoolT3[index];
    }

    return nullptr;
}



DirectX::XMFLOAT3 EnemyManager::CalculateSpawnPosition() {
    XMFLOAT3 playerPos = GetPlayer()->GetPosition();
    XMFLOAT3 cameraFront = Player_Camera_GetFront();

    float baseAngle = atan2f(cameraFront.x, cameraFront.z);

    // 左右90度
    float offsetAngle = ((float)rand() / RAND_MAX - 0.5f) * XM_PI; // 左右90度
    float finalAngle = baseAngle + offsetAngle;

    // カメラの奥方向
    float dist = CAMERA_FRONT_DISTANCE + ((float)rand() / RAND_MAX * CAMERA_FAR_DISTANCE);

    DirectX::XMFLOAT3 pos{};
    pos.x = playerPos.x + sinf(finalAngle) * dist;
    pos.z = playerPos.z + cosf(finalAngle) * dist;
    pos.y = Map_GetGroundHeight(pos.x, pos.z, playerPos.y) + OFFSET_Y;

    // マップ境界チェック
    AABB mapBounds = MeshField_GetAABB(pos);
    float margin = 5.0f;
    if (pos.x < mapBounds.min.x + margin || pos.x > mapBounds.max.x - margin ||
        pos.z < mapBounds.min.z + margin || pos.z > mapBounds.max.z - margin)
    {
        pos.y = -999.0f; // 確実に画面外へ出す
        return pos;
    }

    return pos;
}

bool EnemyManager_GetNearestEnemy( const DirectX::XMFLOAT3& from, DirectX::XMFLOAT3& outPos) {
    float bestDistSq = FLT_MAX;
    bool found = false;

    for (int i = 0; i < EnemyManager::GetMaxCount(); i++) {
        Enemy* e = EnemyManager::GetEnemy(i);

        if (!e || !e->IsActive())
            continue;

        const XMFLOAT3& pos = e->GetPosition();

        float dx = pos.x - from.x;
        float dy = pos.y - from.y;
        float dz = pos.z - from.z;

        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            outPos = pos;
            found = true;
        }
    }

    return found;
}
